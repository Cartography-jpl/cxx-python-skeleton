#pragma once
// Registry of the classes bound with nanobind, keyed by the C++ type.
//
// nanobind gives each class one bound base, and a virtual base can't be a
// nanobind base at all (claude_notes/implementation_plan.md §2.1). So for
// the GenericObject hierarchy, which has secondary and virtual bases
// everywhere, nanobind alone can't:
//  * return a shared_ptr<Base> as the most derived python type when the
//    most derived class reaches Base through a secondary or virtual base,
//  * accept a derived object for a parameter typed as such a base (or as
//    GenericObject).
// This registry does both: for each bound class B it has a function from
// GenericObject* to B* (a dynamic_cast, which handles virtual bases), and
// a function from a python object of type B to shared_ptr<GenericObject>.
// The shared_ptr caster (gc_shared_ptr.h) uses it. It is the nanobind
// version of SwigTypeMapperBase.
//
// The registry lives in a capsule in the builtins module, so every
// extension module of the project that uses these types (e.g. a
// downstream module) shares it. The key has the project's namespace in
// it: each project (geocal, framework) has its own registry, of its own
// Map type, even when they are loaded into one python process. Bump the
// version in the key if Entry or Map changes.
#include <nanobind/nanobind.h>
#include "generic_object.h"
#include <boost/preprocessor/stringize.hpp>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>

namespace CXX_PYTHON_RULES_NAMESPACE {
namespace nbreg {
namespace nb = nanobind;

struct Entry {
  // The bound type (for a trampoline alias, the class it overrides).
  const std::type_info* bound;
  // GenericObject* -> pointer to the bound type B (as void*), or null.
  void* (*from_generic)(GenericObject*);
  // python object of type B (or a python subclass) -> shared_ptr.
  std::shared_ptr<GenericObject> (*to_generic)(nb::handle);
};

// Hash for heterogeneous lookup: find() looks up the type name as a
// std::string_view, without making a std::string.
struct NameHash {
  using is_transparent = void;
  std::size_t operator()(std::string_view S) const noexcept
  {
    return std::hash<std::string_view>{}(S);
  }
};

// Keyed by type name, not type_info address: the same type_info can be
// duplicated across shared libraries.
using Map = std::unordered_map<std::string, Entry, NameHash, std::equal_to<>>;

inline constexpr const char* registry_key =
    "__cxx_python_rules_nb_type_registry_v2_" BOOST_PP_STRINGIZE(
        CXX_PYTHON_RULES_NAMESPACE) "__";

inline Map& registry()
{
  // A function-local static is initialized exactly once, even with
  // several threads.
  static Map* m = [] {
    nb::dict b = nb::borrow<nb::dict>(PyEval_GetBuiltins());
    if(b.contains(registry_key))
      return static_cast<Map*>(nb::cast<nb::capsule>(b[registry_key]).data());
    // Never freed: modules can't be unloaded, and the entries hold
    // function pointers into them.
    auto* r = new Map;
    b[registry_key] = nb::capsule(r);
    return r;
  }();
  return *m;
}

inline const Entry* find(const std::type_info& T)
{
  auto& r = registry();
  auto i = r.find(std::string_view(T.name()));
  return i == r.end() ? nullptr : &i->second;
}

//-----------------------------------------------------------------------
/// Register a bound class B (call after nb::class_<B>).
//-----------------------------------------------------------------------

template<class B> void register_type()
{
  static_assert(std::is_base_of_v<GenericObject, B>);
  registry()[typeid(B).name()] = Entry{
      &typeid(B), [](GenericObject* g) -> void* { return dynamic_cast<B*>(g); },
      [](nb::handle h) -> std::shared_ptr<GenericObject> {
        return nb::cast<std::shared_ptr<B>>(h);
      }};
}

//-----------------------------------------------------------------------
/// Register a C++ class that isn't bound but should be returned to
/// python as the bound class B (a trampoline class for B).
//-----------------------------------------------------------------------

template<class Alias, class B> void register_alias()
{
  registry()[typeid(Alias).name()] = registry().at(typeid(B).name());
}

//-----------------------------------------------------------------------
/// Python object -> shared_ptr<GenericObject>, or null if it isn't a
/// registered type.
//-----------------------------------------------------------------------

inline std::shared_ptr<GenericObject> to_generic(nb::handle h)
{
  if(!nb::inst_check(h))
    return nullptr;
  const Entry* e = find(nb::type_info(h.type()));
  if(!e)
    return nullptr;
  return e->to_generic(h);
}
}
}
