// Throwaway probe 7: forked shared_ptr caster with GeoCal-like virtual GenericObject : esft
#include <nanobind/nanobind.h>
#include "gc_shared_ptr.h"          // fork of nanobind/stl/shared_ptr.h
#include <nanobind/trampoline.h>
#include <memory>
namespace nb = nanobind;
struct GenericObject : std::enable_shared_from_this<GenericObject> { virtual ~GenericObject() = default; };
template<class T> struct Printable : public virtual GenericObject {};
template<class T> struct Observable { int n = 0; };
struct WithParameter : public virtual GenericObject { virtual int psize() const { return 2; } };
struct Camera : public Printable<Camera>, public Observable<Camera>, public WithParameter {
  virtual int number_band() const { return 1; }
};
struct QuaternionCamera : Camera { int number_band() const override { return 3; } };
struct PyCamera : Camera { NB_TRAMPOLINE(Camera, 1); int number_band() const override { NB_OVERRIDE(number_band); } };
struct Holder {
  void set_weak(std::shared_ptr<Camera> p) { w = p; }
  void set_strong(std::shared_ptr<Camera> p) { s = p; }
  bool expired() const { return w.expired(); }
  int apply() const { auto p = w.lock(); return p ? p->number_band() : -999; }
  std::weak_ptr<Camera> w; std::shared_ptr<Camera> s;
};
NB_MODULE(probe8, m) {
  nb::class_<GenericObject>(m, "GenericObject");
  nb::class_<Camera, GenericObject, PyCamera>(m, "Camera").def(nb::init<>()).def("number_band", &Camera::number_band)
    .def("psize", [](const Camera& c) { return c.psize(); });
  nb::class_<QuaternionCamera, Camera>(m, "QuaternionCamera").def(nb::init<>());
  m.def("make_generic", []() { return std::shared_ptr<GenericObject>(std::make_shared<QuaternionCamera>()); });
  m.def("make_withparam", []() { return std::shared_ptr<WithParameter>(std::make_shared<QuaternionCamera>()); });
  m.def("make_camera", []() { return std::shared_ptr<Camera>(std::make_shared<QuaternionCamera>()); });
  m.def("identity_generic", [](std::shared_ptr<GenericObject> g) { return g; });
  nb::class_<Holder>(m, "Holder").def(nb::init<>())
    .def("set_weak", &Holder::set_weak).def("set_strong", &Holder::set_strong)
    .def("expired", &Holder::expired).def("apply", &Holder::apply);
}
