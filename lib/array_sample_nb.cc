// Python bindings for ArraySample and ColumnMajorSample (array_sample.h):
// std::array, std::optional and blitz::Array members, column-major and
// strided arrays.
#include "skeleton_nb_opaque.h"
#include "array_sample.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_array_sample(nb::module_& m)
{
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::ArraySample");
    nb::class_<ArraySample> c(m, "ArraySample", doc());
    c.def(nb::new_([]() { return std::make_shared<ArraySample>(); }));
    c.def("position", &ArraySample::position, doc("position"));
    c.def("position_ref", &ArraySample::position_ref, doc("position_ref"));
    // A std::array member: the setter takes anything numpy converts to
    // 3 numbers (RuntimeError for the wrong size, as with SWIG).
    c.def_prop_rw(
        "position_", [](const ArraySample& self) { return self.position_; },
        [](ArraySample& self, nb::handle v) {
          self.position_ = cxx_python_rules_nb::to_std_array<double, 3>(v);
        },
        doc("position_"));
    c.def_prop_rw(
        "matrix", nb::overload_cast<>(&ArraySample::matrix, nb::const_),
        nb::overload_cast<const blitz::Array<double, 2>&>(&ArraySample::matrix),
        doc("matrix"));
    c.def_prop_rw(
        "count", nb::overload_cast<>(&ArraySample::count, nb::const_),
        nb::overload_cast<const std::optional<int>&>(&ArraySample::count),
        doc("count"));
    c.def_prop_rw(
        "scale", nb::overload_cast<>(&ArraySample::scale, nb::const_),
        nb::overload_cast<const std::optional<double>&>(&ArraySample::scale),
        doc("scale"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<ArraySample>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::ColumnMajorSample");
    nb::class_<ColumnMajorSample> c(m, "ColumnMajorSample", doc());
    c.def(nb::new_([](int Rows, int Cols) {
            return std::make_shared<ColumnMajorSample>(Rows, Cols);
          }),
          "Rows"_a = 2, "Cols"_a = 3);
    c.def("strided_view", &ColumnMajorSample::strided_view,
          doc("strided_view"));
    c.def_static("layout", &ColumnMajorSample::layout, "A"_a, doc("layout"));
    c.def_static("data_address", &ColumnMajorSample::data_address, "A"_a,
                 doc("data_address"));
    c.def_static("sum", &ColumnMajorSample::sum, "A"_a, doc("sum"));
    c.def_static("scale_in_place", &ColumnMajorSample::scale_in_place, "A"_a,
                 "V"_a, doc("scale_in_place"));
    c.def_prop_rw("data",
                  nb::overload_cast<>(&ColumnMajorSample::data, nb::const_),
                  nb::overload_cast<const blitz::Array<double, 2>&>(
                      &ColumnMajorSample::data),
                  doc("data"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::register_type<ColumnMajorSample>();
  }
}
