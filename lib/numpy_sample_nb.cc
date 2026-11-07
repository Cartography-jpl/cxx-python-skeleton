// Python bindings for NumpySample (numpy_sample.h).
#include "skeleton_nb_opaque.h"
#include "numpy_sample.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_numpy_sample(nb::module_& m)
{
  const auto doc = cxx_python_rules_nb::docs("Skeleton::NumpySample");
  nb::class_<NumpySample> c(m, "NumpySample", doc());
  c.def(nb::new_([]() { return std::make_shared<NumpySample>(); }));
  c.def(nb::new_([](const blitz::Array<double, 2>& Din) {
          return std::make_shared<NumpySample>(Din);
        }),
        "Din"_a);
  c.def("maybe_range", &NumpySample::maybe_range, "Return_empty"_a,
        doc("maybe_range"));
  c.def_prop_ro("data", &NumpySample::data, doc("data"));
  cxx_python_rules_nb::printable(c);
  cxx_python_rules_nb::register_type<NumpySample>();
}
