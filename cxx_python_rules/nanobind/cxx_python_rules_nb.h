#pragma once
// The nanobind rules library: include this (first) in every binding file.
// It replaced the SWIG rules (swig_rules/include, removed in phase S6).
//
// Needs CXX_PYTHON_RULES_NAMESPACE (the namespace of GenericObject and of
// serialize_function.h), which the project's configuration header defines
// (reached through generic_object.h), and CXX_PYTHON_RULES_NB_PACKAGE (the
// python package the extension module is in, e.g. "geocal_cxx"), which
// nanobind.am passes on the command line.
#include "generic_object.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/bind_vector.h>
#include <nanobind/trampoline.h>
#include "gc_shared_ptr.h"
#include "type_registry.h"
#include "blitz_array.h"
#include "std_array.h"
#include "iostream.h"
#include "serialize_function.h"
#include "director_hook.h"
#include <boost/archive/polymorphic_iarchive.hpp>
#include <boost/archive/polymorphic_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/string.hpp>
#include <exception>
#include <functional>
#include <map>
#include <string_view>
#include <string>
#include <vector>
#include <utility>

#ifndef CXX_PYTHON_RULES_NB_PACKAGE
#error                                                                         \
    "Define CXX_PYTHON_RULES_NB_PACKAGE, the python package of the extension module"
#endif

namespace cxx_python_rules_nb {
namespace nb = nanobind;
using GenericObject = CXX_PYTHON_RULES_NAMESPACE::GenericObject;
using CXX_PYTHON_RULES_NAMESPACE::nbblitz::to_std_array;
using CXX_PYTHON_RULES_NAMESPACE::nbreg::register_alias;
using CXX_PYTHON_RULES_NAMESPACE::nbreg::register_type;

//-----------------------------------------------------------------------
/// The python helper module (cxx_python_rules/_nanobind_support.py, installed
/// as <package>._nanobind_support).
//-----------------------------------------------------------------------

inline nb::module_& support()
{
  static nb::module_* m = new nb::module_(
      nb::module_::import_(CXX_PYTHON_RULES_NB_PACKAGE "._nanobind_support"));
  return *m;
}

//-----------------------------------------------------------------------
/// Docstrings, filled in from the doxygen documentation by the generated
/// docstrings file (doxy2nanobind.py). Returns "" if there is none
/// (nanobind crashes on a null docstring).
//-----------------------------------------------------------------------

std::map<std::string, const char*, std::less<>>& docstring_table();
inline const char* doc(std::string_view Name)
{
  auto& t = docstring_table();
  auto i = t.find(Name);
  if(i != t.end())
    return i->second;
  // A template instance, e.g. Ns::Observer<Ns::Foo>::notify_update is
  // documented as Ns::Observer::notify_update.
  std::string s;
  int depth = 0;
  for(char c : Name) {
    if(c == '<')
      ++depth;
    else if(c == '>')
      --depth;
    else if(depth == 0)
      s += c;
  }
  i = t.find(s);
  return i == t.end() ? "" : i->second;
}

//-----------------------------------------------------------------------
/// The docstrings of one class: doc() for the class, doc("f") for its
/// member f. Make one per bound class:
///
///   const auto doc = cxx_python_rules_nb::docs("Ns::Foo");
///   nb::class_<Ns::Foo> c(m, "Foo", doc());
///   c.def("f", &Ns::Foo::f, doc("f"));
///
/// A member declared in a base class is documented under the base's name;
/// use cxx_python_rules_nb::doc("Ns::Base::f") for it.
//-----------------------------------------------------------------------

class docs {
public:
  explicit docs(std::string Class) : cls(std::move(Class)) {}
  const char* operator()() const { return doc(cls); }
  const char* operator()(std::string_view Member) const
  {
    std::string name = cls;
    name.append("::").append(Member);
    return doc(name);
  }
private:
  std::string cls;
};

//-----------------------------------------------------------------------
/// Classes the rules need: blitz::Range (for std::optional<blitz::Range>)
/// and boost::rational<int> ("rational_int"), as swig_std.i and
/// swig_rational.i did.
//-----------------------------------------------------------------------

void bind_common_types(nb::module_& m);

//-----------------------------------------------------------------------
/// Set up the module: exception translation (register_exceptions) and
/// the hook for python subclasses in archives. Call first in NB_MODULE.
//-----------------------------------------------------------------------

void init_module(nb::module_& m);

//-----------------------------------------------------------------------
/// Exception translation, the same as swig_exception.i:
///  * a python exception raised in an override (nb::python_error) comes
///    back unchanged (nanobind does this);
///  * TypeErrorT (a library exception type, so the C++ library doesn't
///    depend on python) -> TypeError;
///  * nb::cast_error (e.g. a python override returned the wrong type) ->
///    TypeError;
///  * any other std::exception -> RuntimeError with what(). (nanobind's
///    default maps some std exceptions to ValueError/IndexError; we keep
///    RuntimeError, as SWIG did.)
///  * ExceptionT (the library's exception base, bound to python)
///    additionally gets attributes cpp_exception (a copy of the C++
///    exception, e.g. cpp_exception.stacktrace()) and cpp_source (where it
///    was thrown) on the python exception.
///  * anything else -> RuntimeError("unknown C++ exception").
//-----------------------------------------------------------------------

inline void set_error(PyObject* Type, const char* Msg)
{
  PyErr_SetString(Type, Msg);
}

template<class ExceptionT, class TypeErrorT> void register_exceptions()
{
  // Translators are tried newest first, so register the catch all first.
  nb::register_exception_translator([](const std::exception_ptr& p, void*) {
    try {
      std::rethrow_exception(p);
    } catch(nb::python_error& e) {
      e.restore();
    } catch(nb::builtin_exception&) {
      throw;
    } catch(const std::bad_cast& e) {
      // Includes nb::cast_error (an alias of std::bad_cast)
      set_error(PyExc_TypeError, e.what());
    } catch(const TypeErrorT& e) {
      set_error(PyExc_TypeError, e.what());
    } catch(const ExceptionT& e) {
      set_error(PyExc_RuntimeError, e.what());
      // Take the exception just set (a new reference, already
      // normalized), add the attributes, and set it again (which steals
      // the reference). Python >= 3.12.
      PyObject* value = PyErr_GetRaisedException();
      try {
        // A copy of the C++ exception (it only holds the stack trace as
        // addresses, symbol lookup happens if python calls stacktrace()),
        // and where it was thrown.
        nb::handle v(value);
        v.attr("cpp_exception") = nb::cast(ExceptionT(e));
        auto& l = e.source_location();
        v.attr("cpp_source") = std::string(l.file_name()) + ":" +
                               std::to_string(l.line()) + " " +
                               l.function_name();
      } catch(...) { // NOLINT(bugprone-empty-catch)
        // Deliberate: if adding the attributes fails, keep the RuntimeError
        // without them rather than replace the error being reported.
      }
      PyErr_SetRaisedException(value);
    } catch(const std::exception& e) {
      set_error(PyExc_RuntimeError, e.what());
    } catch(...) {
      set_error(PyExc_RuntimeError, "unknown C++ exception");
    }
  });
}

//-----------------------------------------------------------------------
/// Pickling with boost serialization (%pickle_serialization): __reduce__
/// gives (_new_from_serialization, (serialize_write_binary_string(self),)).
/// Works for python subclasses of a class with director serialization
/// too (the python class and __dict__ are in the archive).
//-----------------------------------------------------------------------

inline nb::bytes serialize_self(nb::handle self)
{
  auto obj = nb::cast<std::shared_ptr<GenericObject>>(self);
  std::string d =
      CXX_PYTHON_RULES_NAMESPACE::serialize_write_binary_string(obj);
  return nb::bytes(d.data(), d.size());
}

template<class C> void pickle_serialization(C& cls)
{
  cls.def("__reduce__", [](nb::handle self) {
    return nb::make_tuple(support().attr("_new_from_serialization"),
                          nb::make_tuple(serialize_self(self)));
  });
}

//-----------------------------------------------------------------------
/// %pickle_serialization_dir: also records the directory, and loads
/// from there (for objects with relative file names).
//-----------------------------------------------------------------------

template<class C> void pickle_serialization_dir(C& cls)
{
  cls.def("__reduce__", [](nb::handle self) {
    nb::object cwd = nb::module_::import_("os").attr("getcwd")();
    return nb::make_tuple(support().attr("_new_from_serialization_dir"),
                          nb::make_tuple(cwd, serialize_self(self)));
  });
}

//-----------------------------------------------------------------------
/// %pickle_init(VER, attributes...): pickled as the constructor
/// arguments, read from the given attributes.
//-----------------------------------------------------------------------

template<class C>
void pickle_init(C& cls, int Version, std::vector<std::string> Attr)
{
  cls.def_static("pickle_format_version", [Version]() { return Version; });
  cls.def("__reduce__", [Version, Attr = std::move(Attr)](nb::handle self) {
    nb::list args;
    args.append(self.type());
    args.append(Version);
    for(const auto& a : Attr)
      args.append(self.attr(a.c_str()));
    return nb::make_tuple(support().attr("_new_from_init"), nb::tuple(args));
  });
}

//-----------------------------------------------------------------------
/// %pickle_vector, for a bind_vector type.
//-----------------------------------------------------------------------

template<class C> void pickle_vector(C& cls)
{
  cls.def_static("pickle_format_version", []() { return 1; });
  cls.def("__reduce__", [](nb::handle self) {
    return nb::make_tuple(support().attr("_new_vector"),
                          nb::make_tuple(self.type(), 1, nb::list(self)));
  });
}

//-----------------------------------------------------------------------
/// print_to_string as __str__ (swig_print.i).
//-----------------------------------------------------------------------

template<class C> void printable(C& cls)
{
  using T = C::Type;
  cls.def("print_to_string", [](const T& s) { return s.print_to_string(); });
  cls.def("__str__", [](const T& s) { return s.print_to_string(); });
}

//-----------------------------------------------------------------------
/// Director serialization (director_serialization.i): a python subclass
/// of a C++ class, inside a boost archive. The C++ part is written as
/// the C++ base class, plus "python_object", a pickle of the python
/// class and __dict__ (in the same form SWIG wrote, so archives written
/// with SWIG still load). Reading it makes the python object first, and
/// the C++ object in place (the python object owns it); director_hook
/// then gives the archive's shared_ptr ownership of the python object.
//-----------------------------------------------------------------------

std::map<GenericObject*, nb::object>& pending_director();
std::shared_ptr<GenericObject>
director_hook_func(const std::shared_ptr<GenericObject>& V);

inline std::string director_dumps(nb::handle Self)
{
  nb::bytes b = nb::cast<nb::bytes>(support().attr("director_dumps")(Self));
  return std::string(b.c_str(), b.size());
}

// Make the python object for the C++ object at D (not yet constructed).
// Construct must placement-new the C++ object at D.
template<class Tramp, class Construct>
void director_load(const std::string& Python_object, Tramp* D,
                   Construct Construct_func)
{
  nb::tuple cs = nb::cast<nb::tuple>(support().attr("director_loads")(
      nb::bytes(Python_object.data(), Python_object.size())));
  nb::object inst = nb::inst_take_ownership(cs[0], static_cast<void*>(D));
  Construct_func(D);
  nb::getattr(inst, "__dict__").attr("update")(cs[1]);
  pending_director()[static_cast<GenericObject*>(D)] = inst;
}
}

//-----------------------------------------------------------------------
/// NB_OVERRIDE_NAME for a virtual function whose one argument is a
/// reference to a bound class (e.g. Observer<T>::notify_update(const T&)).
/// NB_OVERRIDE_NAME passes a C++ reference to python with nanobind's
/// default policy for calls into python, which *copies* the object: the
/// python override got a copy (for an Observable, with a copy of its
/// observer list), not the object, and a class that can't be copied
/// aborts. This passes the object itself (rv_policy::reference, the
/// existing python object if it has one), as SWIG did. As with any
/// reference, python must not keep it after the call returns unless the
/// object is owned elsewhere.
//-----------------------------------------------------------------------

#define CXX_PYTHON_RULES_NB_OVERRIDE_REF(name, func, arg)                      \
  using nb_ret_type = decltype(NBBase::func(arg));                             \
  constexpr uint64_t nb_hash = nanobind::detail::str_hash(name);               \
  nanobind::detail::ticket nb_ticket(nb_trampoline, name, nb_hash, false);     \
  if(nb_ticket.key.is_valid()) {                                               \
    return nanobind::cast<nb_ret_type>(                                        \
        nb_trampoline.base().attr(nb_ticket.key)(                              \
            nanobind::cast(&(arg), nanobind::rv_policy::reference)));          \
  } else                                                                       \
    return NBBase::func(arg)

//-----------------------------------------------------------------------
/// Boost serialization of the trampoline TRAMP for BASE (NAME is BASE
/// without the namespace). Written as SWIG did: class name
/// "SwigDirector_" NAME, the base part as element NAME, so archives
/// written with SWIG load. TRAMP needs a public default constructor. Use
/// at namespace scope in a .cc file.
//-----------------------------------------------------------------------

// (TRAMP and BASE are type names, so they can't be in parentheses.)
// NOLINTBEGIN(bugprone-macro-parentheses)
#define CXX_PYTHON_RULES_NB_DIRECTOR_SERIALIZATION(TRAMP, BASE, NAME)          \
  namespace boost {                                                            \
  namespace serialization {                                                    \
  template<class Archive>                                                      \
  void serialize(Archive& ar, TRAMP& D, const unsigned int)                    \
  {                                                                            \
    ar& boost::serialization::make_nvp(                                        \
        BOOST_PP_STRINGIZE(NAME), boost::serialization::base_object<BASE>(D)); \
  }                                                                            \
  template<class Archive>                                                      \
  void save_construct_data(Archive& ar, const TRAMP* d, const unsigned int)    \
  {                                                                            \
    std::string python_object =                                                \
        cxx_python_rules_nb::director_dumps(d->nb_trampoline.base());          \
    ar& BOOST_SERIALIZATION_NVP(python_object);                                \
  }                                                                            \
  template<class Archive>                                                      \
  void load_construct_data(Archive& ar, TRAMP* d, const unsigned int)          \
  {                                                                            \
    std::string python_object;                                                 \
    ar& BOOST_SERIALIZATION_NVP(python_object);                                \
    cxx_python_rules_nb::director_load(python_object, d,                       \
                                       [](TRAMP* p) { ::new(p) TRAMP(); });    \
  }                                                                            \
  }                                                                            \
  }                                                                            \
  BOOST_CLASS_EXPORT_KEY2(TRAMP, "SwigDirector_" BOOST_PP_STRINGIZE(NAME));    \
  BOOST_CLASS_EXPORT_IMPLEMENT(TRAMP);                                         \
  template void boost::serialization::serialize(                               \
      boost::archive::polymorphic_oarchive&, TRAMP&, const unsigned int);      \
  template void boost::serialization::serialize(                               \
      boost::archive::polymorphic_iarchive&, TRAMP&, const unsigned int);      \
  template void boost::serialization::save_construct_data(                     \
      boost::archive::polymorphic_oarchive&, const TRAMP*,                     \
      const unsigned int);                                                     \
  template void boost::serialization::load_construct_data(                     \
      boost::archive::polymorphic_iarchive&, TRAMP*, const unsigned int);
// NOLINTEND(bugprone-macro-parentheses)
