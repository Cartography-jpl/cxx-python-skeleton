// Python bindings for DirectorExample and the classes that use it
// (director_example.h). Python can subclass DirectorExample (the
// trampoline below), and python subclass objects can be serialized
// (CXX_PYTHON_RULES_NB_DIRECTOR_SERIALIZATION). See
// cxx_python_rules/lib/DirectorNotes.md.
#include "skeleton_nb_opaque.h"
#include "director_example.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

namespace {
class PyDirectorExample : public DirectorExample {
public:
  NB_TRAMPOLINE(DirectorExample);
  PyDirectorExample() = default;
  std::string desc() const override { NB_OVERRIDE_NAME("desc", desc); }
  int func(int v) const override { NB_OVERRIDE_NAME("func", func, v); }
};
}

CXX_PYTHON_RULES_NB_DIRECTOR_SERIALIZATION(PyDirectorExample, DirectorExample,
                                           DirectorExample)

void init_director_example(nb::module_& m)
{
  using DirectorVec = std::vector<std::shared_ptr<DirectorExample>>;
  nb::bind_vector<DirectorVec>(m, "Vector_DirectorExample");
  nb::bind_vector<std::vector<DirectorVec>>(m, "Vector_Vector_DirectorExample");
  nb::bind_vector<std::vector<std::vector<int>>>(m, "Vector_Vector_int");
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::DirectorExample");
    nb::class_<DirectorExample, PyDirectorExample> c(m, "DirectorExample",
                                                     doc());
    c.def(nb::init<int, std::string>(), "value_to_add"_a, "name"_a);
    // The virtual functions call the C++ base implementation directly when
    // self is a python subclass (the trampoline): a python override that
    // calls super() would otherwise go back through the trampoline and run
    // the override a second time.
    c.def(
        "desc",
        [](const DirectorExample& self) {
          if(dynamic_cast<const PyDirectorExample*>(&self))
            return self.DirectorExample::desc();
          return self.desc();
        },
        doc("desc"));
    c.def(
        "func",
        [](const DirectorExample& self, int v) {
          if(dynamic_cast<const PyDirectorExample*>(&self))
            return self.DirectorExample::func(v);
          return self.func(v);
        },
        "v"_a, doc("func"));
    c.def("value_to_add",
          nb::overload_cast<>(&DirectorExample::value_to_add, nb::const_),
          doc("value_to_add"));
    c.def("value_to_add",
          nb::overload_cast<int>(&DirectorExample::value_to_add), "v"_a,
          doc("value_to_add"));
    c.def("name", &DirectorExample::name, doc("name"));
    c.def("same_object", &DirectorExample::same_object, "obj2"_a,
          doc("same_object"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<DirectorExample>();
    cxx_python_rules_nb::register_alias<PyDirectorExample, DirectorExample>();
  }
  {
    const auto doc =
        cxx_python_rules_nb::docs("Skeleton::DirectorExampleNested");
    nb::class_<DirectorExampleNested, DirectorExample> c(
        m, "DirectorExampleNested", doc());
    c.def(nb::new_([](int value_to_add, const std::string& name,
                      const std::shared_ptr<DirectorExample>& Obj) {
            return std::make_shared<DirectorExampleNested>(value_to_add, name,
                                                           Obj);
          }),
          "value_to_add"_a, "name"_a, "Obj"_a.none());
    c.def_prop_ro("obj", &DirectorExampleNested::obj, doc("obj"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<DirectorExampleNested>();
  }
  {
    const auto doc =
        cxx_python_rules_nb::docs("Skeleton::DirectorExampleWeakPtr");
    nb::class_<DirectorExampleWeakPtr> c(m, "DirectorExampleWeakPtr", doc());
    c.def(nb::new_([](const std::shared_ptr<DirectorExample>& V) {
            return std::make_shared<DirectorExampleWeakPtr>(V);
          }),
          "V"_a.none());
    c.def("a", nb::overload_cast<>(&DirectorExampleWeakPtr::a, nb::const_),
          doc("a"));
    c.def("a",
          nb::overload_cast<const std::shared_ptr<DirectorExample>&>(
              &DirectorExampleWeakPtr::a),
          "V"_a.none(), doc("a"));
    c.def("expired", &DirectorExampleWeakPtr::expired, doc("expired"));
    c.def("use_count", &DirectorExampleWeakPtr::use_count, doc("use_count"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<DirectorExampleWeakPtr>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::DirectorExampleUser");
    nb::class_<DirectorExampleUser> c(m, "DirectorExampleUser", doc());
    c.def(nb::new_([]() { return std::make_shared<DirectorExampleUser>(); }));
    c.def("value_vec", &DirectorExampleUser::value_vec, doc("value_vec"));
    c.def("set_vec", &DirectorExampleUser::set_vec, "V"_a, doc("set_vec"));
    c.def("value_vec_vec", &DirectorExampleUser::value_vec_vec,
          doc("value_vec_vec"));
    c.def("set_vec_vec", &DirectorExampleUser::set_vec_vec, "V"_a,
          doc("set_vec_vec"));
    c.def("value", &DirectorExampleUser::value, doc("value"));
    c.def("value_vec_int", &DirectorExampleUser::value_vec_int,
          doc("value_vec_int"));
    c.def("set_vec_int", &DirectorExampleUser::set_vec_int, "V"_a,
          doc("set_vec_int"));
    c.def("value_vec_vec_int", &DirectorExampleUser::value_vec_vec_int,
          doc("value_vec_vec_int"));
    c.def("set_vec_vec_int", &DirectorExampleUser::set_vec_vec_int, "V"_a,
          doc("set_vec_vec_int"));
    c.def("set_value", &DirectorExampleUser::set_value, "V"_a.none(),
          doc("set_value"));
    c.def("apply_func",
          nb::overload_cast<int>(&DirectorExampleUser::apply_func, nb::const_),
          "v"_a, doc("apply_func"));
    c.def("apply_func",
          nb::overload_cast<int, int>(&DirectorExampleUser::apply_func,
                                      nb::const_),
          "v"_a, "ind"_a, doc("apply_func"));
    c.def("apply_func",
          nb::overload_cast<int, int, int>(&DirectorExampleUser::apply_func,
                                           nb::const_),
          "v"_a, "ind"_a, "ind2"_a, doc("apply_func"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<DirectorExampleUser>();
  }
}
