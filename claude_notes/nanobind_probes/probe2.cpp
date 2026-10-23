// Throwaway probe 2: workarounds for (a) shared_ptr<virtual base> downcast,
// (b) weak_ptr to Python-created objects.
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/trampoline.h>
#include <functional>
#include <map>
#include <memory>
#include <typeindex>

namespace nb = nanobind;

struct GenericObject { virtual ~GenericObject() = default; };
struct Base : public virtual GenericObject {
  virtual ~Base() = default;
  virtual int func(int v) const { return v + 1; }
};
struct Derived : public Base { int func(int v) const override { return v + 2; } };

// ---- (a) small type mapper: typeid(dynamic type) -> caster ----
using caster_t = std::function<nb::object(const std::shared_ptr<GenericObject>&)>;
std::map<std::type_index, caster_t>& registry() { static std::map<std::type_index, caster_t> r; return r; }
template<class T> void register_generic() {
  registry()[typeid(T)] = [](const std::shared_ptr<GenericObject>& p) {
    return nb::cast(std::dynamic_pointer_cast<T>(p)); };
}
nb::object to_python(const std::shared_ptr<GenericObject>& p) {
  if(!p) return nb::none();
  auto it = registry().find(typeid(*p));
  if(it == registry().end()) throw std::runtime_error("type not registered");
  return it->second(p);
}
std::shared_ptr<GenericObject> make_generic() { return std::make_shared<Derived>(); }

// ---- (b) weak_ptr holders ----
struct PyBase : Base {
  NB_TRAMPOLINE(Base, 1);
  int func(int v) const override { NB_OVERRIDE(func, v); }
};
struct Plain {  // no virtuals besides dtor, no trampoline
  virtual ~Plain() = default;
  int v = 7;
};
struct Holder {
  void set_weak_base(std::shared_ptr<Base> p) { wb = p; }
  bool base_expired() const { return wb.expired(); }
  int apply(int v) const { auto p = wb.lock(); return p ? p->func(v) : -999; }
  void set_weak_plain(std::shared_ptr<Plain> p) { wp = p; }
  bool plain_expired() const { return wp.expired(); }
  void set_strong_base(std::shared_ptr<Base> p) { sb = p; }
  std::weak_ptr<Base> wb; std::weak_ptr<Plain> wp; std::shared_ptr<Base> sb;
};

NB_MODULE(probe2, m) {
  nb::class_<GenericObject>(m, "GenericObject");
  nb::class_<Base, PyBase>(m, "Base").def(nb::init<>()).def("func", &Base::func);
  nb::class_<Derived, Base>(m, "Derived").def(nb::init<>());
  register_generic<Base>(); register_generic<Derived>();
  m.def("make_generic_raw", &make_generic);
  m.def("make_generic_mapped", []() { return to_python(make_generic()); });

  // Plain class, ordinary nb::init (Python-owned storage)
  nb::class_<Plain>(m, "Plain").def(nb::init<>());
  // Same class exposed with a shared_ptr factory constructor (C++-owned via shared_ptr)
  struct PlainShared : Plain {};
  nb::class_<PlainShared, Plain>(m, "PlainShared")
    .def(nb::new_([]() { return std::make_shared<PlainShared>(); }));
  // Base via shared_ptr factory (no Python subclassing in this path)
  m.def("make_base_shared", []() { return std::shared_ptr<Base>(std::make_shared<Derived>()); });

  nb::class_<Holder>(m, "Holder").def(nb::init<>())
    .def("set_weak_base", &Holder::set_weak_base).def("base_expired", &Holder::base_expired)
    .def("apply", &Holder::apply)
    .def("set_weak_plain", &Holder::set_weak_plain).def("plain_expired", &Holder::plain_expired)
    .def("set_strong_base", &Holder::set_strong_base);
}
