// Python bindings for ArrayTypemapSample (array_typemap_sample.h), which
// tests numpy <-> blitz::Array (cxx_python_rules/nanobind/blitz_array.h).
// Python can subclass it and override transform.
#include "skeleton_nb_opaque.h"
#include "array_typemap_sample.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

namespace {
class PyArrayTypemapSample : public ArrayTypemapSample {
public:
  NB_TRAMPOLINE(ArrayTypemapSample);
  blitz::Array<double, 1>
  transform(const blitz::Array<double, 1>& A) const override
  {
    NB_OVERRIDE_NAME("transform", transform, A);
  }
};
}

void init_array_typemap_sample(nb::module_& m)
{
  using Sample = ArrayTypemapSample;
  const auto doc = cxx_python_rules_nb::docs("Skeleton::ArrayTypemapSample");
  nb::class_<Sample, PyArrayTypemapSample> c(m, "ArrayTypemapSample", doc());
  c.def(nb::init<>());
  c.def("echo_double_1", &Sample::echo_double_1, "A"_a, doc("echo_double_1"));
  c.def("echo_double_2", &Sample::echo_double_2, "A"_a, doc("echo_double_2"));
  c.def("echo_double_3", &Sample::echo_double_3, "A"_a, doc("echo_double_3"));
  c.def("echo_float_2", &Sample::echo_float_2, "A"_a, doc("echo_float_2"));
  c.def("echo_int_1", &Sample::echo_int_1, "A"_a, doc("echo_int_1"));
  c.def("echo_int_2", &Sample::echo_int_2, "A"_a, doc("echo_int_2"));
  c.def("echo_int_3", &Sample::echo_int_3, "A"_a, doc("echo_int_3"));
  c.def("echo_uint_2", &Sample::echo_uint_2, "A"_a, doc("echo_uint_2"));
  c.def("echo_bool_1", &Sample::echo_bool_1, "A"_a, doc("echo_bool_1"));
  c.def("echo_bool_2", &Sample::echo_bool_2, "A"_a, doc("echo_bool_2"));
  c.def("echo_short_2", &Sample::echo_short_2, "A"_a, doc("echo_short_2"));
  c.def("echo_ushort_2", &Sample::echo_ushort_2, "A"_a, doc("echo_ushort_2"));
  c.def("echo_char_2", &Sample::echo_char_2, "A"_a, doc("echo_char_2"));
  c.def("echo_uchar_2", &Sample::echo_uchar_2, "A"_a, doc("echo_uchar_2"));
  c.def("sum_by_value", &Sample::sum_by_value, "A"_a, doc("sum_by_value"));
  c.def("sum_double_2", &Sample::sum_double_2, "A"_a, doc("sum_double_2"));
  c.def("element", &Sample::element, "A"_a, "I"_a, "J"_a, doc("element"));
  c.def("set_value", &Sample::set_value, "A"_a, "V"_a, doc("set_value"));
  // C++ gets a copy of the array (SWIG's FORCE_COPY), so its write isn't
  // seen by python.
  c.def(
      "set_value_copy",
      [](const Sample& self, const blitz::Array<double, 2>& A, double V) {
        self.set_value_copy(A.copy(), V);
      },
      "A"_a, "V"_a, doc("set_value_copy"));
  // The output arguments are returned as a tuple.
  c.def(
      "split",
      [](const Sample& self, const blitz::Array<double, 2>& A) {
        blitz::Array<double, 1> row0, col0;
        self.split(A, row0, col0);
        return nb::make_tuple(row0, col0);
      },
      "A"_a, doc("split"));
  c.def(
      "scaled",
      [](const Sample& self, const blitz::Array<double, 2>& A, double S) {
        blitz::Array<double, 2> res;
        double total = 0;
        self.scaled(A, S, res, total);
        return nb::make_tuple(res, total);
      },
      "A"_a, "S"_a, doc("scaled"));
  c.def("echo_vector", &Sample::echo_vector, "V"_a, doc("echo_vector"));
  c.def("vector_size", &Sample::vector_size, "V"_a, doc("vector_size"));
  c.def("stored_ref", &Sample::stored_ref, doc("stored_ref"));
  c.def("stored_value", &Sample::stored_value, "I"_a, "J"_a,
        doc("stored_value"));
  // Calls the C++ implementation directly for a python subclass, so an
  // override calling super() doesn't run twice (see observer_nb.cc).
  c.def(
      "transform",
      [](const Sample& self, const blitz::Array<double, 1>& A) {
        if(dynamic_cast<const PyArrayTypemapSample*>(&self))
          return self.Sample::transform(A);
        return self.transform(A);
      },
      "A"_a, doc("transform"));
  c.def("apply_transform", &Sample::apply_transform, "A"_a,
        doc("apply_transform"));
  c.def_prop_ro("stored", &Sample::stored, doc("stored"));
  cxx_python_rules_nb::printable(c);
  cxx_python_rules_nb::register_type<Sample>();
  cxx_python_rules_nb::register_alias<PyArrayTypemapSample, Sample>();
}
