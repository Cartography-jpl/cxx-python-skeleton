// Python bindings for OverloadExample (overload_example.h): overloads
// that differ only in the type of their argument.
#include "skeleton_nb_opaque.h"
#include "overload_example.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_overload_example(nb::module_& m)
{
  const auto doc = cxx_python_rules_nb::docs("Skeleton::OverloadExample");
  nb::class_<OverloadExample> c(m, "OverloadExample", doc());
  c.def(nb::new_([]() { return std::make_shared<OverloadExample>(); }));
  c.def_static("which",
               nb::overload_cast<const blitz::Array<double, 2>&>(
                   &OverloadExample::which),
               "A"_a, doc("which"));
  c.def_static("which",
               nb::overload_cast<const std::shared_ptr<NumpySample>&>(
                   &OverloadExample::which),
               "S"_a.none(), doc("which"));
  c.def_static("which",
               nb::overload_cast<const std::string&>(&OverloadExample::which),
               "S"_a, doc("which"));
  cxx_python_rules_nb::printable(c);
  cxx_python_rules_nb::register_type<OverloadExample>();
}
