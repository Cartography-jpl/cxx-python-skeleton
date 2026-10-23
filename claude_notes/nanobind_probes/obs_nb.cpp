// Throwaway probe: Observer pattern (aliasing self pointer) through nanobind + forked caster
#include <nanobind/nanobind.h>
#include "gc_shared_ptr.h"
#include <nanobind/trampoline.h>
#include <list>
#include <memory>
namespace nb = nanobind;
struct null_deleter { void operator()(const void*) const {} };
struct GenericObject : std::enable_shared_from_this<GenericObject> { virtual ~GenericObject() = default; };
template<class T> struct Observer : public virtual GenericObject {
  Observer() { this_obj = std::shared_ptr<Observer>(std::shared_ptr<void>(static_cast<void*>(this), null_deleter()), this); }
  virtual void notify_update(const T&) {}
  std::shared_ptr<Observer> this_obj;
};
template<class T> struct Observable : public virtual GenericObject {
  void add_observer(Observer<T>& o) { olist.push_back(o.this_obj); }
  void add_observer_and_keep_reference(std::shared_ptr<Observer<T>> o) { ref_list.push_back(o); add_observer(*o); }
  int notify_do(const T& self) { int n = 0; olist.remove_if([](auto& w){ return w.expired(); });
    for(auto& w : olist) if(auto p = w.lock()) { p->notify_update(self); ++n; } return n; }
  std::list<std::weak_ptr<Observer<T>>> olist; std::list<std::shared_ptr<Observer<T>>> ref_list;
};
struct Camera : public Observable<Camera> { int notify() { return notify_do(*this); } };
struct CameraObserver : public Observer<Camera> {};
struct PyCameraObserver : CameraObserver { NB_TRAMPOLINE(CameraObserver, 1);
  void notify_update(const Camera& c) override { NB_OVERRIDE(notify_update, c); } };
struct Igc : public Observer<Camera> {
  explicit Igc(std::shared_ptr<Camera> c) : cam(c) { c->add_observer(*this); }   // ctor registration
  void notify_update(const Camera&) override { ++count; }
  std::shared_ptr<Camera> cam; int count = 0;
};
NB_MODULE(obs_nb, m) {
  nb::class_<Camera>(m, "Camera").def(nb::init<>()).def("notify", &Camera::notify)
    .def("add_observer", [](Camera& c, CameraObserver& o) { c.add_observer(o); })
    .def("add_observer_and_keep_reference", [](Camera& c, std::shared_ptr<CameraObserver> o) { c.add_observer_and_keep_reference(o); });
  nb::class_<CameraObserver, PyCameraObserver>(m, "CameraObserver").def(nb::init<>());
  nb::class_<Igc>(m, "Igc").def(nb::init<std::shared_ptr<Camera>>()).def_ro("count", &Igc::count);
  m.def("owner_is_real", [](std::shared_ptr<Igc> i) {   // does shared_from_this() share the caster's control block?
    auto s = i->shared_from_this(); return !s.owner_before(i) && !i.owner_before(s); });
}
