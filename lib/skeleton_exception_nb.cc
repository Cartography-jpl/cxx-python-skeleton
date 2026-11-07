// Python bindings for Exception (skeleton_exception.h). A C++ Exception
// thrown to python becomes a RuntimeError, with a copy of the C++ object
// as its cpp_exception attribute (cxx_python_rules_nb::register_exceptions).
#include "skeleton_nb_opaque.h"
#include "skeleton_exception.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_skeleton_exception(nb::module_& m)
{
  const auto doc = cxx_python_rules_nb::docs("Skeleton::Exception");
  nb::class_<Exception> c(m, "Exception", doc());
  c.def(nb::new_([](const std::string& W) {
          return std::make_shared<Exception>(W);
        }),
        "W"_a);
  c.def("what", &Exception::what, doc("what"));
  c.def("stacktrace", &Exception::stacktrace, doc("stacktrace"));
  c.def_static("capture_stacktrace",
               nb::overload_cast<bool>(&Exception::capture_stacktrace), "V"_a,
               doc("capture_stacktrace"));
  c.def_static("capture_stacktrace",
               nb::overload_cast<>(&Exception::capture_stacktrace),
               doc("capture_stacktrace"));
  cxx_python_rules_nb::printable(c);
  // std::source_location isn't bound; give its parts.
  c.def("source_file", [](const Exception& self) {
    return std::string(self.source_location().file_name());
  });
  c.def("source_line", [](const Exception& self) {
    return static_cast<int>(self.source_location().line());
  });
  c.def("source_function", [](const Exception& self) {
    return std::string(self.source_location().function_name());
  });
  cxx_python_rules_nb::register_type<Exception>();
}
