// Python bindings for ValueSample and CxxPythonRulesSample
// (cxx_python_rules_sample.h): std::optional, boost::rational, std::array,
// properties, a virtual property python can override, and the pickle
// helpers.
#include "skeleton_nb_opaque.h"
#include "cxx_python_rules_sample.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

namespace {
class PyCxxPythonRulesSample : public CxxPythonRulesSample {
public:
  NB_TRAMPOLINE(CxxPythonRulesSample);
  double weight() const override { NB_OVERRIDE_NAME("_v_weight", weight); }
  void weight(const double& V) override
  {
    NB_OVERRIDE_NAME("_v_weight", weight, V);
  }
};
}

void init_cxx_python_rules_sample(nb::module_& m)
{
  {
    auto c = nb::bind_vector<std::vector<ValueSample>>(m, "Vector_ValueSample");
    cxx_python_rules_nb::pickle_vector(c);
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::ValueSample");
    nb::class_<ValueSample> c(m, "ValueSample", doc());
    c.def(nb::new_([](double V) { return std::make_shared<ValueSample>(V); }),
          "V"_a = 0);
    c.def_prop_ro("v", &ValueSample::v, doc("v"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_init(c, 1, {"v"});
    cxx_python_rules_nb::register_type<ValueSample>();
  }
  {
    using S = CxxPythonRulesSample;
    const auto doc =
        cxx_python_rules_nb::docs("Skeleton::CxxPythonRulesSample");
    nb::class_<S, PyCxxPythonRulesSample> c(m, "CxxPythonRulesSample", doc());
    c.def(nb::init<int, std::string>(), "Count"_a = 0, "Name"_a = "");
    c.def("optional_double",
          nb::overload_cast<>(&S::optional_double, nb::const_),
          doc("optional_double"));
    c.def("optional_double",
          nb::overload_cast<std::optional<double>>(&S::optional_double), "V"_a,
          doc("optional_double"));
    c.def("optional_double_ref", &S::optional_double_ref,
          doc("optional_double_ref"));
    c.def("optional_range_ref", &S::optional_range_ref,
          doc("optional_range_ref"));
    c.def("optional_range", &S::optional_range, "R"_a, doc("optional_range"));
    c.def("optional_range_size", &S::optional_range_size, "R"_a,
          doc("optional_range_size"));
    c.def("rational_value", &S::rational_value, "R"_a, doc("rational_value"));
    c.def("half", &S::half, doc("half"));
    c.def("sum_array", &S::sum_array, "A"_a, doc("sum_array"));
    c.def("first_element", &S::first_element, "A"_a, doc("first_element"));
    c.def("data_value", &S::data_value, "I"_a, "J"_a, doc("data_value"));
    c.def("weight_set", &S::weight_set, doc("weight_set"));
    c.def("weight_from_cpp", &S::weight_from_cpp, doc("weight_from_cpp"));
    c.def("values", &S::values, doc("values"));
    c.def("sum_values", &S::sum_values, "V"_a, doc("sum_values"));
    c.def("int_vector", &S::int_vector, doc("int_vector"));
    c.def("sum_int_vector", &S::sum_int_vector, "V"_a, doc("sum_int_vector"));
    c.def_prop_ro("count", &S::count, doc("count"));
    c.def_prop_ro("name", &S::name, doc("name"));
    c.def_prop_rw(
        "optional_int", nb::overload_cast<>(&S::optional_int, nb::const_),
        nb::overload_cast<const std::optional<int>&>(&S::optional_int),
        doc("optional_int"));
    // A std::array member: the setter takes anything numpy converts to
    // 3 numbers (RuntimeError for the wrong size, as with SWIG).
    c.def_prop_rw(
        "position_", [](const S& self) { return self.position_; },
        [](S& self, nb::handle v) {
          self.position_ = cxx_python_rules_nb::to_std_array<double, 3>(v);
        },
        doc("position_"));
    // Properties named differently from their accessors.
    c.def_prop_ro("count_squared", &S::get_count_squared,
                  doc("get_count_squared"));
    c.def_prop_rw("scale", nb::overload_cast<>(&S::get_scale, nb::const_),
                  nb::overload_cast<const double&>(&S::get_scale),
                  doc("get_scale"));
    c.def_prop_ro("counter", &S::counter, doc("counter"));
    // A writable numpy view of the C++ array (non-const reference).
    c.def_prop_ro("data", &S::data_ptr, doc("data_ptr"));
    // The weight property goes through the _v_weight methods, which a
    // python subclass overrides (the trampoline above). They call the C++
    // implementation directly for a python subclass, so an override
    // calling super() doesn't run twice.
    c.def("_v_weight", [](const S& self) {
      if(dynamic_cast<const PyCxxPythonRulesSample*>(&self))
        return self.S::weight();
      return self.weight();
    });
    c.def(
        "_v_weight",
        [](S& self, double V) {
          if(dynamic_cast<PyCxxPythonRulesSample*>(&self))
            self.S::weight(V);
          else
            self.weight(V);
        },
        "V"_a);
    c.def_prop_rw(
        "weight", [](nb::handle self) { return self.attr("_v_weight")(); },
        [](nb::handle self, nb::handle v) { self.attr("_v_weight")(v); },
        doc("weight"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_init(c, 1, {"count", "name"});
    cxx_python_rules_nb::register_type<S>();
    cxx_python_rules_nb::register_alias<PyCxxPythonRulesSample, S>();
  }
}
