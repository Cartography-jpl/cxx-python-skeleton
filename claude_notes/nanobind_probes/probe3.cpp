// Throwaway probe 3: enable_shared_from_this + weak_ptr in nanobind
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/trampoline.h>
#include <memory>
namespace nb = nanobind;
struct E : std::enable_shared_from_this<E> {
  virtual ~E() = default;
  virtual int func(int v) const { return v + 1; }
};
struct PyE : E { NB_TRAMPOLINE(E, 1); int func(int v) const override { NB_OVERRIDE(func, v); } };
struct Holder {
  void set_weak(std::shared_ptr<E> p) { w = p; }
  void set_strong(std::shared_ptr<E> p) { s = p; }
  bool expired() const { return w.expired(); }
  long use_count() const { return w.use_count(); }
  int apply(int v) const { auto p = w.lock(); return p ? p->func(v) : -999; }
  std::weak_ptr<E> w; std::shared_ptr<E> s;
};
NB_MODULE(probe3, m) {
  nb::class_<E, PyE>(m, "E").def(nb::init<>()).def("func", &E::func);
  m.def("make_cpp", []() { return std::make_shared<E>(); });
  nb::class_<Holder>(m, "Holder").def(nb::init<>())
    .def("set_weak", &Holder::set_weak).def("set_strong", &Holder::set_strong)
    .def("expired", &Holder::expired).def("use_count", &Holder::use_count).def("apply", &Holder::apply);
}
