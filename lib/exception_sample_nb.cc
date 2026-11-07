// Python bindings for ExceptionSample (exception_sample.h), which python
// can subclass.
#include "skeleton_nb_opaque.h"
#include "exception_sample.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

namespace {
class PyExceptionSample : public ExceptionSample {
public:
  NB_TRAMPOLINE(ExceptionSample);
  int value() const override { NB_OVERRIDE_NAME("value", value); }
};
}

void init_exception_sample(nb::module_& m)
{
  const auto doc = cxx_python_rules_nb::docs("Skeleton::ExceptionSample");
  nb::class_<ExceptionSample, PyExceptionSample> c(m, "ExceptionSample", doc());
  c.def(nb::init<>());
  c.def("throw_skeleton_exception", &ExceptionSample::throw_skeleton_exception,
        "Msg"_a, doc("throw_skeleton_exception"));
  c.def("throw_std_exception", &ExceptionSample::throw_std_exception, "Msg"_a,
        doc("throw_std_exception"));
  c.def("throw_type_error", &ExceptionSample::throw_type_error, "Msg"_a,
        doc("throw_type_error"));
  c.def("throw_unknown", &ExceptionSample::throw_unknown, doc("throw_unknown"));
  c.def("checked_value", &ExceptionSample::checked_value, "V"_a,
        doc("checked_value"));
  // Calls the C++ implementation directly for a python subclass, so an
  // override calling super() doesn't run twice (see observer_nb.cc).
  c.def(
      "value",
      [](const ExceptionSample& self) {
        if(dynamic_cast<const PyExceptionSample*>(&self))
          return self.ExceptionSample::value();
        return self.value();
      },
      doc("value"));
  c.def("value_error_message", &ExceptionSample::value_error_message,
        doc("value_error_message"));
  c.def("value_rethrow_copy", &ExceptionSample::value_rethrow_copy,
        doc("value_rethrow_copy"));
  c.def_static("caught_exception", &ExceptionSample::caught_exception, "Msg"_a,
               doc("caught_exception"));
  cxx_python_rules_nb::printable(c);
  cxx_python_rules_nb::register_type<ExceptionSample>();
  cxx_python_rules_nb::register_alias<PyExceptionSample, ExceptionSample>();
}
