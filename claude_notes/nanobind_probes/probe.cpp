// Throwaway probe: does nanobind handle the GeoCal/skeleton class patterns?
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/trampoline.h>
#include <memory>
#include <sstream>
#include <vector>
#include <string>

namespace nb = nanobind;

// ---- GeoCal-style mixins: GenericObject (virtual base) + Printable CRTP ----
struct GenericObject {
  virtual ~GenericObject() = default;
};
template<class T> struct Printable : public virtual GenericObject {
  std::string print_to_string() const {
    std::ostringstream os; static_cast<const T*>(this)->print(os); return os.str();
  }
};

// ---- GeoCal-style multiple inheritance: Camera : Observable<Camera>, WithParameter ----
template<class T> struct Observable {
  int observer_count() const { return n_; }
  void add_observer_count() { ++n_; }
  int n_ = 0;
};
struct WithParameter {
  virtual ~WithParameter() = default;
  virtual std::vector<double> parameter() const { return {1.0, 2.0}; }
  int parameter_size() const { return (int) parameter().size(); }
};

struct Camera : public Observable<Camera>, public WithParameter, public Printable<Camera> {
  virtual ~Camera() = default;
  virtual int number_band() const { return 1; }
  virtual void print(std::ostream& os) const { os << "Camera"; }
};
struct QuaternionCamera : public Camera {
  int number_band() const override { return 3; }
  void print(std::ostream& os) const override { os << "QuaternionCamera"; }
};

// Functions with base-class static types (downcast tests)
std::shared_ptr<Camera> make_camera_as_base() { return std::make_shared<QuaternionCamera>(); }
std::shared_ptr<GenericObject> make_generic() { return std::make_shared<QuaternionCamera>(); }
std::shared_ptr<WithParameter> make_withparam() { return std::make_shared<QuaternionCamera>(); }
int use_withparam(const WithParameter& w) { return w.parameter_size(); }
int use_observable(Observable<Camera>& o) { o.add_observer_count(); return o.observer_count(); }

// ---- Director-style class held by C++ via shared_ptr and weak_ptr ----
struct Director : public Printable<Director> {
  virtual ~Director() = default;
  virtual int func(int v) const { return v + 1; }
  void print(std::ostream& os) const { os << "Director"; }
};
struct PyDirector : Director {
  NB_TRAMPOLINE(Director, 1);
  int func(int v) const override { NB_OVERRIDE(func, v); }
};
struct DirectorUser {
  void set(std::shared_ptr<Director> d) { d_ = d; }
  int apply(int v) const { return d_->func(v); }
  void set_weak(std::shared_ptr<Director> d) { w_ = d; }
  bool weak_expired() const { return w_.expired(); }
  int apply_weak(int v) const { auto p = w_.lock(); return p ? p->func(v) : -999; }
  std::shared_ptr<Director> get() const { return d_; }
  std::shared_ptr<Director> d_;
  std::weak_ptr<Director> w_;
};

// Same, but with enable_shared_from_this
struct DirectorEsft : public Printable<DirectorEsft>, public std::enable_shared_from_this<DirectorEsft> {
  virtual ~DirectorEsft() = default;
  virtual int func(int v) const { return v + 1; }
  void print(std::ostream& os) const { os << "DirectorEsft"; }
};
struct PyDirectorEsft : DirectorEsft {
  NB_TRAMPOLINE(DirectorEsft, 1);
  int func(int v) const override { NB_OVERRIDE(func, v); }
};
struct DirectorEsftUser {
  void set_weak(std::shared_ptr<DirectorEsft> d) { w_ = d; }
  bool weak_expired() const { return w_.expired(); }
  int apply_weak(int v) const { auto p = w_.lock(); return p ? p->func(v) : -999; }
  std::weak_ptr<DirectorEsft> w_;
};

NB_MODULE(probe, m) {
  // Mixins: bind GenericObject so shared_ptr<GenericObject> can be returned
  nb::class_<GenericObject>(m, "GenericObject");
  nb::class_<WithParameter>(m, "WithParameter")
    .def("parameter_size", &WithParameter::parameter_size);
  // Camera: one bound base only (WithParameter). Observable + Printable methods
  // bound directly on Camera via lambdas / base member pointers.
  nb::class_<Camera, WithParameter>(m, "Camera")
    .def(nb::init<>())
    .def("number_band", &Camera::number_band)
    .def("observer_count", [](const Camera& c) { return c.observer_count(); })
    .def("add_observer_count", &Observable<Camera>::add_observer_count)   // base member pointer
    .def("__str__", &Camera::print_to_string);
  nb::class_<QuaternionCamera, Camera>(m, "QuaternionCamera").def(nb::init<>());
  m.def("make_camera_as_base", &make_camera_as_base);
  m.def("make_generic", &make_generic);
  m.def("make_withparam", &make_withparam);
  m.def("use_withparam", &use_withparam);
  m.def("use_observable", [](Camera& c) { return use_observable(c); });

  nb::class_<Director, PyDirector>(m, "Director")
    .def(nb::init<>())
    .def("func", &Director::func)
    .def("__str__", &Director::print_to_string);
  nb::class_<DirectorUser>(m, "DirectorUser")
    .def(nb::init<>())
    .def("set", &DirectorUser::set).def("apply", &DirectorUser::apply)
    .def("get", &DirectorUser::get)
    .def("set_weak", &DirectorUser::set_weak).def("weak_expired", &DirectorUser::weak_expired)
    .def("apply_weak", &DirectorUser::apply_weak);
  nb::class_<DirectorEsft, PyDirectorEsft>(m, "DirectorEsft")
    .def(nb::init<>()).def("func", &DirectorEsft::func);
  nb::class_<DirectorEsftUser>(m, "DirectorEsftUser")
    .def(nb::init<>())
    .def("set_weak", &DirectorEsftUser::set_weak).def("weak_expired", &DirectorEsftUser::weak_expired)
    .def("apply_weak", &DirectorEsftUser::apply_weak);
}
