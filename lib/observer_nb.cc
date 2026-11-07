// Python bindings for CacheInvalidatedObserver (observer.h), which python
// can subclass. Observer<T> and Observable<T> are bound for each T with
// that class (observer_example_nb.cc, multi_base_example_nb.cc).
#include "skeleton_nb_opaque.h"
#include "observer.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

namespace {
class PyCacheInvalidatedObserver : public CacheInvalidatedObserver {
public:
  NB_TRAMPOLINE(CacheInvalidatedObserver);
  void invalidate_cache() override
  {
    NB_OVERRIDE_NAME("invalidate_cache", invalidate_cache);
  }
  bool cache_valid_flag() const override
  {
    NB_OVERRIDE_NAME("_v_cache_valid_flag", cache_valid_flag);
  }
  void cache_valid_flag(bool V) override
  {
    NB_OVERRIDE_NAME("_v_cache_valid_flag", cache_valid_flag, V);
  }
};
}

void init_observer(nb::module_& m)
{
  const auto doc =
      cxx_python_rules_nb::docs("Skeleton::CacheInvalidatedObserver");
  nb::class_<CacheInvalidatedObserver, PyCacheInvalidatedObserver> c(
      m, "CacheInvalidatedObserver", doc());
  c.def(nb::init<>());
  // The virtual functions call the C++ base implementation directly when
  // self is a python subclass (the trampoline): a python override that
  // calls super() would otherwise go back through the trampoline and run
  // the override a second time.
  c.def(
      "invalidate_cache",
      [](CacheInvalidatedObserver& self) {
        if(dynamic_cast<PyCacheInvalidatedObserver*>(&self))
          return self.CacheInvalidatedObserver::invalidate_cache();
        return self.invalidate_cache();
      },
      doc("invalidate_cache"));
  c.def("_v_cache_valid_flag", [](const CacheInvalidatedObserver& self) {
    if(dynamic_cast<const PyCacheInvalidatedObserver*>(&self))
      return self.CacheInvalidatedObserver::cache_valid_flag();
    return self.cache_valid_flag();
  });
  c.def(
      "_v_cache_valid_flag",
      [](CacheInvalidatedObserver& self, bool V) {
        if(dynamic_cast<PyCacheInvalidatedObserver*>(&self))
          self.CacheInvalidatedObserver::cache_valid_flag(V);
        else
          self.cache_valid_flag(V);
      },
      "V"_a);
  c.def_prop_rw(
      "cache_valid_flag",
      [](nb::handle self) { return self.attr("_v_cache_valid_flag")(); },
      [](nb::handle self, nb::handle v) {
        self.attr("_v_cache_valid_flag")(v);
      },
      doc("cache_valid_flag"));
  cxx_python_rules_nb::register_type<CacheInvalidatedObserver>();
  cxx_python_rules_nb::register_alias<PyCacheInvalidatedObserver,
                                      CacheInvalidatedObserver>();
}
