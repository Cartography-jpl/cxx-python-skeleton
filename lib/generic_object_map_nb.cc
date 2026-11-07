// Python bindings for GenericObjectMap (generic_object_map.h), with the
// python mapping protocol (m["a"], "a" in m) and attribute access (m.a).
#include "skeleton_nb_opaque.h"
#include "generic_object_map.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_generic_object_map(nb::module_& m)
{
  const auto doc = cxx_python_rules_nb::docs("Skeleton::GenericObjectMap");
  nb::class_<GenericObjectMap> c(m, "GenericObjectMap", doc());
  c.def(nb::new_([]() { return std::make_shared<GenericObjectMap>(); }));
  c.def("keys", &GenericObjectMap::keys, doc("keys"));
  c.def("set_generic", &GenericObjectMap::set_generic, "Key"_a, "V"_a.none(),
        doc("set_generic"));
  c.def("get_generic", &GenericObjectMap::get_generic, "Key"_a,
        doc("get_generic"));
  cxx_python_rules_nb::printable(c);
  cxx_python_rules_nb::pickle_serialization(c);
  c.def("__contains__", &GenericObjectMap::contains);
  c.def("__getitem__", &GenericObjectMap::get_generic);
  c.def("__setitem__", &GenericObjectMap::set_generic);
  c.def("__getattr__",
        [](const GenericObjectMap& self, const std::string& key) {
          auto v = self.get_generic(key);
          if(!v)
            throw nb::attribute_error(key.c_str());
          return v;
        });
  cxx_python_rules_nb::register_type<GenericObjectMap>();
}
