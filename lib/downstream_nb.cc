// A second extension module, skeleton_cxx._skeleton_downstream, that uses
// the classes bound in the main module (_bindings) without binding them
// again: the pattern of geocal -> ECOSTRESS and framework -> the instrument
// repositories (claude_notes/implementation_plan.md S6.1 item 10). Both
// modules must use the same nanobind version and build flags; nanobind
// shares its type information between them, and the cxx_python_rules type
// registry is shared through a capsule in builtins (type_registry.h).
#include "cxx_python_rules_nb.h"
#include "generic_object_sample.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace Skeleton {
// A class only the downstream module knows, derived from a class bound in
// the main module.
class DownstreamB : public GenericObjectB {
public:
  DownstreamB(std::shared_ptr<GenericObjectA> A, int Extra)
    : GenericObjectB(std::move(A)), extra_(Extra)
  {
  }
  int extra() const { return extra_; }
private:
  int extra_ = 0;
};
}

NB_MODULE(_skeleton_downstream, m)
{
  using namespace Skeleton;
  // The main module has to be loaded first, for its types.
  nb::module_::import_(CXX_PYTHON_RULES_NB_PACKAGE "._bindings");
  m.def(
      "make_a", [](int V) { return std::make_shared<GenericObjectA>(V); },
      "V"_a);
  m.def(
      "a_value",
      [](const std::shared_ptr<GenericObjectA>& A) { return A->v(); }, "A"_a);
  // Returned as GenericObject, python gets the most derived type (the
  // registry knows the main module's classes).
  m.def(
      "as_generic", [](const std::shared_ptr<GenericObject>& G) { return G; },
      "G"_a.none());
  nb::class_<DownstreamB, GenericObjectB>(m, "DownstreamB")
      .def(nb::new_([](const std::shared_ptr<GenericObjectA>& A, int Extra) {
             return std::make_shared<DownstreamB>(A, Extra);
           }),
           "A"_a, "Extra"_a)
      .def_prop_ro("extra", &DownstreamB::extra);
  cxx_python_rules_nb::register_type<DownstreamB>();
}
