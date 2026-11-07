#pragma once
// Bindings for one instance of Observer<T> and Observable<T> (observer.h),
// shared by the classes that use them (observer_example_nb.cc,
// multi_base_example_nb.cc). Python can subclass Observer<T>.
#include "skeleton_nb_opaque.h"
#include "observer.h"

namespace Skeleton {
// Hidden, like nanobind's own namespace: the trampoline holds nanobind
// objects, and a class with default visibility can't have hidden members.
namespace nb_observer __attribute__((visibility("hidden"))) {
  namespace nb = nanobind;
  using namespace nb::literals;

  //-----------------------------------------------------------------------
  /// Trampoline for Observer<T>. The notify functions pass the Observable
  /// itself to a python override, not a copy (CXX_PYTHON_RULES_NB_OVERRIDE_REF).
  //-----------------------------------------------------------------------

  template<class T> class PyObserver : public Observer<T> {
  public:
    NB_TRAMPOLINE(Observer<T>);
    void notify_update(const T& Observed_object) override
    {
      CXX_PYTHON_RULES_NB_OVERRIDE_REF("notify_update", notify_update,
                                       Observed_object);
    }
    void notify_add(T& Observed_object) override
    {
      CXX_PYTHON_RULES_NB_OVERRIDE_REF("notify_add", notify_add,
                                       Observed_object);
    }
    void notify_remove(T& Observed_object) override
    {
      CXX_PYTHON_RULES_NB_OVERRIDE_REF("notify_remove", notify_remove,
                                       Observed_object);
    }
  };

  //-----------------------------------------------------------------------
  /// Bind Observable<T> as Name; Doc_name is its C++ name for the
  /// docstrings (e.g. "Skeleton::Observable<Skeleton::CameraLike>").
  //-----------------------------------------------------------------------

  template<class T>
  void bind_observable(nb::module_ & m, const char* Name, const char* Doc_name)
  {
    using O = Observable<T>;
    const auto doc = cxx_python_rules_nb::docs(Doc_name);
    nb::class_<O> c(m, Name, doc());
    c.def("add_observer_and_keep_reference",
          &O::add_observer_and_keep_reference, "Obs"_a,
          doc("add_observer_and_keep_reference"));
    c.def("add_cache_invalidated_observer", &O::add_cache_invalidated_observer,
          "Obs"_a, doc("add_cache_invalidated_observer"));
    c.def("remove_cache_invalidated_observer",
          &O::remove_cache_invalidated_observer, "Obs"_a,
          doc("remove_cache_invalidated_observer"));
    c.def("add_observer", &O::add_observer, "Obs"_a, doc("add_observer"));
    c.def("remove_observer", &O::remove_observer, "Obs"_a,
          doc("remove_observer"));
    c.def("clear_observers", &O::clear_observers, doc("clear_observers"));
    c.def_prop_ro("number_observer", &O::number_observer,
                  doc("number_observer"));
    c.def_prop_ro("number_cache_observer", &O::number_cache_observer,
                  doc("number_cache_observer"));
    cxx_python_rules_nb::register_type<O>();
  }

  //-----------------------------------------------------------------------
  /// Bind Observer<T> as Name, with its trampoline.
  //-----------------------------------------------------------------------

  template<class T>
  void bind_observer(nb::module_ & m, const char* Name, const char* Doc_name)
  {
    using O = Observer<T>;
    using Py = PyObserver<T>;
    const auto doc = cxx_python_rules_nb::docs(Doc_name);
    nb::class_<O, Py> c(m, Name, doc());
    c.def(nb::init<>());
    // Call the C++ implementation directly for a python subclass, so an
    // override calling super() doesn't run twice (see observer_nb.cc).
    c.def(
        "notify_update",
        [](O& self, const T& Observed_object) {
          if(dynamic_cast<Py*>(&self))
            return self.O::notify_update(Observed_object);
          return self.notify_update(Observed_object);
        },
        "Observed_object"_a, doc("notify_update"));
    c.def(
        "notify_add",
        [](O& self, T& Observed_object) {
          if(dynamic_cast<Py*>(&self))
            return self.O::notify_add(Observed_object);
          return self.notify_add(Observed_object);
        },
        "Observed_object"_a, doc("notify_add"));
    c.def(
        "notify_remove",
        [](O& self, T& Observed_object) {
          if(dynamic_cast<Py*>(&self))
            return self.O::notify_remove(Observed_object);
          return self.notify_remove(Observed_object);
        },
        "Observed_object"_a, doc("notify_remove"));
    cxx_python_rules_nb::register_type<O>();
    cxx_python_rules_nb::register_alias<Py, O>();
  }
}
}
