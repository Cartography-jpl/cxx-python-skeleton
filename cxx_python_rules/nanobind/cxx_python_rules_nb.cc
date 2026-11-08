// Compiled once into the python extension module (not into the C++
// library). See cxx_python_rules_nb.h.
#include "cxx_python_rules_nb.h"
#include <blitz/range.h>
#include <boost/rational.hpp>

namespace cxx_python_rules_nb {
std::map<GenericObject*, nb::object>& pending_director()
{
  // Per thread: a python subclass object read from an archive is made
  // (director_load) and claimed (director_hook_func) on the same thread,
  // during one read. Never freed, like the module objects: freeing would
  // release python references after python may be gone.
  thread_local auto* m = new std::map<GenericObject*, nb::object>;
  return *m;
}

std::shared_ptr<GenericObject>
director_hook_func(const std::shared_ptr<GenericObject>& V)
{
  auto& p = pending_director();
  auto i = p.find(V.get());
  if(i == p.end())
    return nullptr;
  nb::object inst = std::move(i->second);
  p.erase(i);
  // A shared_ptr that owns a reference to the python object (which owns
  // the C++ object).
  return nb::cast<std::shared_ptr<GenericObject>>(inst);
}

void bind_common_types(nb::module_& m)
{
  nb::class_<blitz::Range>(m, "Range")
      .def(nb::init<int, int>(), nb::arg("first"), nb::arg("last"))
      .def(nb::init<int, int, int>(), nb::arg("first"), nb::arg("last"),
           nb::arg("stride"))
      .def("first", [](const blitz::Range& r) { return r.first(); })
      .def("last", [](const blitz::Range& r) { return r.last(); })
      .def("stride", [](const blitz::Range& r) { return r.stride(); })
      .def("length", [](const blitz::Range& r) { return r.length(); });
  using R = boost::rational<int>;
  nb::class_<R>(m, "rational_int")
      .def(nb::init<int, int>(), nb::arg("numerator"),
           nb::arg("denominator") = 1)
      .def_prop_ro("numerator", [](const R& r) { return r.numerator(); })
      .def_prop_ro("denominator", [](const R& r) { return r.denominator(); })
      .def("__float__",
           [](const R& r) { return boost::rational_cast<double>(r); })
      .def("__repr__", [](const R& r) {
        return "rational_int(" + std::to_string(r.numerator()) + ", " +
               std::to_string(r.denominator()) + ")";
      });
}

void init_module(nb::module_& m)
{
  CXX_PYTHON_RULES_NAMESPACE::director_hook() = director_hook_func;
  init_statics();
  // For tests: is the class registered with register_type (or
  // register_alias)? A bound GenericObject class that isn't would come
  // back from C++ as the wrong python type.
  m.def("_is_registered", [](nb::handle T) {
    return nb::type_check(T) &&
           CXX_PYTHON_RULES_NAMESPACE::nbreg::find(nb::type_info(T)) != nullptr;
  });
}
}
