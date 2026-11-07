// Python bindings for the virtual diamond (virtual_diamond_example.h):
// NestedLike, RootLike, Diamond and VirtualDiamondExample. Virtual bases
// are never nanobind bases (cxx_python_rules/nanobind/README.md); their
// methods are bound on each class, and the type registry converts a
// Diamond for a parameter typed as any of its bases.
#include "skeleton_nb_opaque.h"
#include "virtual_diamond_example.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

namespace {
// The parameter property through the _v_parameter methods (which a python
// subclass can override), for a class C derived from WithParameterLike.
template<class C> void bind_parameter(nb::class_<C>& c, const char* Doc)
{
  c.def("_v_parameter", nb::overload_cast<>(&C::parameter, nb::const_));
  c.def("_v_parameter",
        nb::overload_cast<const blitz::Array<double, 1>&>(&C::parameter),
        "Parm"_a);
  c.def_prop_rw(
      "parameter", [](nb::handle self) { return self.attr("_v_parameter")(); },
      [](nb::handle self, nb::handle v) { self.attr("_v_parameter")(v); }, Doc);
}
}

void init_virtual_diamond_example(nb::module_& m)
{
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::NestedLike");
    nb::class_<NestedLike> c(m, "NestedLike", doc());
    c.def("nested_name", &NestedLike::nested_name, doc("nested_name"));
    c.def("parameter_sum", &NestedLike::parameter_sum, doc("parameter_sum"));
    bind_parameter(c, doc("parameter"));
    cxx_python_rules_nb::register_type<NestedLike>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::RootLike");
    nb::class_<RootLike> c(m, "RootLike", doc());
    c.def("root_name", &RootLike::root_name, doc("root_name"));
    cxx_python_rules_nb::register_type<RootLike>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::Diamond");
    nb::class_<Diamond> c(m, "Diamond", doc());
    c.def(nb::new_([](const std::string& Name, double P0, double P1) {
            return std::make_shared<Diamond>(Name, P0, P1);
          }),
          "Name"_a, "P0"_a, "P1"_a);
    c.def("root_name", &Diamond::root_name, doc("root_name"));
    c.def("nested_name", &Diamond::nested_name, doc("nested_name"));
    c.def("parameter_sum", &Diamond::parameter_sum, doc("parameter_sum"));
    c.def_prop_ro("name", &Diamond::name, doc("name"));
    bind_parameter(c, doc("parameter"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<Diamond>();
  }
  {
    const auto doc =
        cxx_python_rules_nb::docs("Skeleton::VirtualDiamondExample");
    nb::class_<VirtualDiamondExample> c(m, "VirtualDiamondExample", doc());
    c.def(nb::new_([](const std::shared_ptr<Diamond>& D) {
            return std::make_shared<VirtualDiamondExample>(D);
          }),
          "D"_a.none());
    c.def("as_root", &VirtualDiamondExample::as_root, doc("as_root"));
    c.def("as_nested", &VirtualDiamondExample::as_nested, doc("as_nested"));
    c.def("as_with_parameter", &VirtualDiamondExample::as_with_parameter,
          doc("as_with_parameter"));
    c.def("as_generic", &VirtualDiamondExample::as_generic, doc("as_generic"));
    c.def_static("root_name", &VirtualDiamondExample::root_name, "R"_a,
                 doc("root_name"));
    c.def_static("nested_name", &VirtualDiamondExample::nested_name, "N"_a,
                 doc("nested_name"));
    c.def_static("parameter_sum", &VirtualDiamondExample::parameter_sum, "P"_a,
                 doc("parameter_sum"));
    c.def_prop_ro("diamond", &VirtualDiamondExample::diamond, doc("diamond"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::register_type<VirtualDiamondExample>();
  }
}
