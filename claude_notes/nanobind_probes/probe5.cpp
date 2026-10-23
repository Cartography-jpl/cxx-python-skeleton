// Throwaway probe 5: shared_ptr<GenericObject> return, non-virtual root, mixins without GenericObject
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <memory>
#include <vector>
namespace nb = nanobind;
struct GenericObject : std::enable_shared_from_this<GenericObject> { virtual ~GenericObject() = default; };
template<class T> struct Observable { int n = 0; void add() { ++n; } };   // mixin, no GenericObject
struct WithParameter { virtual ~WithParameter() = default; int psize() const { return 2; } };  // mixin
struct Camera : public GenericObject, public Observable<Camera>, public WithParameter {
  virtual int number_band() const { return 1; }
};
struct QuaternionCamera : Camera { int number_band() const override { return 3; } };
NB_MODULE(probe5, m) {
  nb::class_<GenericObject>(m, "GenericObject");
  nb::class_<Camera, GenericObject>(m, "Camera").def("number_band", &Camera::number_band)
    .def("psize", [](const Camera& c) { return c.psize(); })
    .def("add", [](Camera& c) { c.add(); return c.n; });
  nb::class_<QuaternionCamera, Camera>(m, "QuaternionCamera").def(nb::init<>());
  m.def("make_generic", []() { return std::shared_ptr<GenericObject>(std::make_shared<QuaternionCamera>()); });
  m.def("generic_list", []() { return std::vector<std::shared_ptr<GenericObject>>{std::make_shared<QuaternionCamera>(), std::make_shared<Camera>()}; });
}
