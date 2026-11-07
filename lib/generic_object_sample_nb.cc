// Python bindings for GenericObjectA, B, B2 and C (generic_object_sample.h),
// small classes for testing serialization and pickling.
#include "skeleton_nb_opaque.h"
#include "generic_object_sample.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_generic_object_sample(nb::module_& m)
{
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::GenericObjectA");
    nb::class_<GenericObjectA> c(m, "GenericObjectA", doc());
    c.def(nb::new_([](int V) { return std::make_shared<GenericObjectA>(V); }),
          "V"_a);
    c.def_prop_rw("v", nb::overload_cast<>(&GenericObjectA::v, nb::const_),
                  nb::overload_cast<int>(&GenericObjectA::v), doc("v"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<GenericObjectA>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::GenericObjectB");
    nb::class_<GenericObjectB> c(m, "GenericObjectB", doc());
    c.def(nb::new_([](const std::shared_ptr<GenericObjectA>& V) {
            return std::make_shared<GenericObjectB>(V);
          }),
          "V"_a.none());
    c.def_prop_rw("a", nb::overload_cast<>(&GenericObjectB::a, nb::const_),
                  nb::overload_cast<const std::shared_ptr<GenericObjectA>&>(
                      &GenericObjectB::a),
                  doc("a"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<GenericObjectB>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::GenericObjectB2");
    nb::class_<GenericObjectB2> c(m, "GenericObjectB2", doc());
    c.def(nb::new_([](const GenericObjectA& V) {
            return std::make_shared<GenericObjectB2>(V);
          }),
          "V"_a);
    c.def_prop_rw("a", nb::overload_cast<>(&GenericObjectB2::a, nb::const_),
                  nb::overload_cast<const GenericObjectA&>(&GenericObjectB2::a),
                  doc("a"));
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<GenericObjectB2>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::GenericObjectC");
    nb::class_<GenericObjectC> c(m, "GenericObjectC", doc());
    c.def(nb::new_([](const std::shared_ptr<GenericObjectA>& V) {
            return std::make_shared<GenericObjectC>(V);
          }),
          "V"_a.none());
    c.def_prop_rw("a", nb::overload_cast<>(&GenericObjectC::a, nb::const_),
                  nb::overload_cast<const std::shared_ptr<GenericObjectA>&>(
                      &GenericObjectC::a),
                  doc("a"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<GenericObjectC>();
  }
  m.def("generic_object_list", &generic_object_list,
        cxx_python_rules_nb::doc("Skeleton::generic_object_list"));
  m.def("generic_object_index", &generic_object_index, "I"_a,
        cxx_python_rules_nb::doc("Skeleton::generic_object_index"));
  nb::bind_vector<std::vector<std::shared_ptr<GenericObject>>>(
      m, "Vector_GenericObject");
}
