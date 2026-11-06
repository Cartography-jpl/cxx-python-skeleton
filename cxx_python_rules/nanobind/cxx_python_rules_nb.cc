// Compiled once into the python extension module (not into the C++
// library). See cxx_python_rules_nb.h.
#include "cxx_python_rules_nb.h"
#include <blitz/range.h>
#include <boost/rational.hpp>

namespace cxx_python_rules_nb {
std::map<GenericObject*, nb::object>& pending_director()
{
  static auto* m = new std::map<GenericObject*, nb::object>;
  return *m;
}

std::shared_ptr<GenericObject>
director_hook_func(const std::shared_ptr<GenericObject>& V)
{
  auto& p = pending_director();
  auto i = p.find(V.get());
  if(i == p.end())
    return nullptr;
  nb::object inst = std::move(i->second);
  p.erase(i);
  // A shared_ptr that owns a reference to the python object (which owns
  // the C++ object).
  return nb::cast<std::shared_ptr<GenericObject>>(inst);
}

void bind_common_types(nb::module_& m)
{
  nb::class_<blitz::Range>(m, "Range")
      .def(nb::init<int, int>(), nb::arg("first"), nb::arg("last"))
      .def(nb::init<int, int, int>(), nb::arg("first"), nb::arg("last"),
           nb::arg("stride"))
      .def("first", [](const blitz::Range& r) { return r.first(); })
      .def("last", [](const blitz::Range& r) { return r.last(); })
      .def("stride", [](const blitz::Range& r) { return r.stride(); })
      .def("length", [](const blitz::Range& r) { return r.length(); });
  using R = boost::rational<int>;
  nb::class_<R>(m, "rational_int")
      .def(nb::init<int, int>(), nb::arg("numerator"),
           nb::arg("denominator") = 1)
      .def_prop_ro("numerator", [](const R& r) { return r.numerator(); })
      .def_prop_ro("denominator", [](const R& r) { return r.denominator(); })
      .def("__float__",
           [](const R& r) { return boost::rational_cast<double>(r); })
      .def("__repr__", [](const R& r) {
        return "rational_int(" + std::to_string(r.numerator()) + ", " +
               std::to_string(r.denominator()) + ")";
      });
}

void init_module(nb::module_&)
{
  CXX_PYTHON_RULES_NAMESPACE::director_hook() = director_hook_func;
}
}
