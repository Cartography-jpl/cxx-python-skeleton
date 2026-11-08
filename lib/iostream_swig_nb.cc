// Python bindings for IostreamSwig (iostream_swig.h): C++ functions that
// take a std::ostream / std::istream get a python file object.
#include "skeleton_nb_opaque.h"
#include "iostream_swig.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_iostream_swig(nb::module_& m)
{
  const auto doc = cxx_python_rules_nb::docs("Skeleton::IostreamSwig");
  nb::class_<IostreamSwig> c(m, "IostreamSwig", doc());
  c.def(nb::new_([](const std::string& To_write, int Word_to_read) {
          return std::make_shared<IostreamSwig>(To_write, Word_to_read);
        }),
        "To_write"_a = "hi there", "Word_to_read"_a = 2);
  c.def(
      "write",
      [](const IostreamSwig& self, nb::object Os) {
        cxx_python_rules_nb::py_ostream os(std::move(Os));
        self.write(os);
        os.flush(); // so an error in the last write comes out here
      },
      "Os"_a, doc("write"));
  c.def(
      "read",
      [](IostreamSwig& self, nb::object In) {
        cxx_python_rules_nb::py_istream is(std::move(In));
        return self.read(is);
      },
      "In"_a, doc("read"));
  cxx_python_rules_nb::printable(c);
  cxx_python_rules_nb::register_type<IostreamSwig>();
}
