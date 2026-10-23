// Throwaway probe: GeoCal/framework Observer pattern + enable_shared_from_this.
// Compares the current to_ptr(*this) self pointer with an aliasing-constructor
// version that does not claim enable_shared_from_this's weak_this.
#include <cstdio>
#include <list>
#include <memory>

struct null_deleter { void operator()(const void*) const {} };
struct GenericObject : std::enable_shared_from_this<GenericObject> { virtual ~GenericObject() = default; };

template<class T> struct Observable;
template<class T> struct Observer : public virtual GenericObject {
  Observer() {
#ifdef USE_TO_PTR
    this_obj = std::shared_ptr<Observer>(this, null_deleter());          // current code (to_ptr)
#else
    this_obj = std::shared_ptr<Observer>(                                // proposed: aliasing ctor,
      std::shared_ptr<void>(static_cast<void*>(this), null_deleter()), this); // doesn't touch weak_this
#endif
  }
  virtual void notify_update(const T&) {}
  std::shared_ptr<Observer> this_obj;
};
template<class T> struct Observable : public virtual GenericObject {
  void add_observer(Observer<T>& o) { olist.push_back(o.this_obj); }
  int notify(const T& self) {
    int n = 0; olist.remove_if([](auto& w) { return w.expired(); });
    for(auto& w : olist) if(auto p = w.lock()) { p->notify_update(self); ++n; }
    return n;
  }
  std::list<std::weak_ptr<Observer<T>>> olist;
};
struct Camera : public Observable<Camera> {};
struct Igc : public Observer<Camera> {
  explicit Igc(Camera& c) { c.add_observer(*this); }   // registration in the constructor, as in GeoCal
  void notify_update(const Camera&) override { ++count; }
  int count = 0;
};

int main() {
  Camera cam;
  // 1. shared_ptr-owned observer: does shared_from_this() give the real owner?
  auto igc = std::make_shared<Igc>(cam);
  auto sft = igc->shared_from_this();
  bool real_owner = !sft.owner_before(igc) && !igc.owner_before(sft);   // same control block
  printf("1. make_shared observer: shared_from_this is the real owner: %s (use_count=%ld)\n",
         real_owner ? "yes" : "NO (non-owning self pointer captured weak_this)", igc.use_count());
  printf("   notify reaches it: %d\n", cam.notify(cam));
  // 2. observer on the stack (constructor registration) still works
  { Igc stack_igc(cam); printf("2. stack observer notified: %d (expect 2)\n", cam.notify(cam)); }
  printf("   after stack observer destroyed: %d (expect 1)\n", cam.notify(cam));
  // 3. shared_ptr-owned observer destroyed -> dropped
  igc.reset(); sft.reset();
  printf("3. after shared_ptr observer destroyed: %d (expect 0)\n", cam.notify(cam));
  // 4. observer as a by-value member of another object
  struct Holder { Camera c; Igc i{c}; } h;
  printf("4. member observer notified: %d (expect 1)\n", h.c.notify(h.c));
  return 0;
}
