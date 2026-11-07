// Python bindings for WithParameterLike (with_parameter_like.h).
#include "skeleton_nb_opaque.h"
#include "with_parameter_like.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_with_parameter_like(nb::module_& m)
{
  const auto doc = cxx_python_rules_nb::docs("Skeleton::WithParameterLike");
  nb::class_<WithParameterLike> c(m, "WithParameterLike", doc());
  c.def("parameter_sum", &WithParameterLike::parameter_sum,
        doc("parameter_sum"));
  // The property goes through the _v_parameter methods, which a python
  // subclass can override (the SWIG %python_attribute_virtual pattern).
  c.def("_v_parameter",
        nb::overload_cast<>(&WithParameterLike::parameter, nb::const_));
  c.def("_v_parameter",
        nb::overload_cast<const blitz::Array<double, 1>&>(
            &WithParameterLike::parameter),
        "Parm"_a);
  c.def_prop_rw(
      "parameter", [](nb::handle self) { return self.attr("_v_parameter")(); },
      [](nb::handle self, nb::handle v) { self.attr("_v_parameter")(v); },
      doc("parameter"));
  cxx_python_rules_nb::register_type<WithParameterLike>();
}
