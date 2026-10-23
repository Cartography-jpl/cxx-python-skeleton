// Throwaway probe 4: enable_shared_from_this on a (virtual) GenericObject root
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/trampoline.h>
#include <memory>
namespace nb = nanobind;
struct GenericObject : std::enable_shared_from_this<GenericObject> { virtual ~GenericObject() = default; };
#ifdef VIRTUAL_BASE
struct Base : public virtual GenericObject {
#else
struct Base : public GenericObject {
#endif
  virtual ~Base() = default;
  virtual int func(int v) const { return v + 1; }
};
struct Derived : Base { int func(int v) const override { return v + 2; } };
struct PyBase : Base { NB_TRAMPOLINE(Base, 1); int func(int v) const override { NB_OVERRIDE(func, v); } };
struct Holder {
  void set_weak(std::shared_ptr<Base> p) { w = p; }
  void set_strong(std::shared_ptr<Base> p) { s = p; }
  bool expired() const { return w.expired(); }
  int apply(int v) const { auto p = w.lock(); return p ? p->func(v) : -999; }
  std::weak_ptr<Base> w; std::shared_ptr<Base> s;
};
NB_MODULE(MODNAME, m) {
  nb::class_<Base, PyBase>(m, "Base").def(nb::init<>()).def("func", &Base::func);
  nb::class_<Derived, Base>(m, "Derived").def(nb::init<>());
  m.def("make_cpp", []() { return std::shared_ptr<Base>(std::make_shared<Derived>()); });
  nb::class_<Holder>(m, "Holder").def(nb::init<>())
    .def("set_weak", &Holder::set_weak).def("set_strong", &Holder::set_strong)
    .def("expired", &Holder::expired).def("apply", &Holder::apply);
}
