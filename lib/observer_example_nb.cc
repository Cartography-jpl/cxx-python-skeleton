// Python bindings for ObservableExample, ObserverExample and CacheExample
// (observer_example.h), with Observer<ObservableExample> and
// Observable<ObservableExample> (observer_nb.h).
#include "observer_nb.h"
#include "observer_example.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_observer_example(nb::module_& m)
{
  nb_observer::bind_observable<ObservableExample>(
      m, "ObservableObservableExample",
      "Skeleton::Observable<Skeleton::ObservableExample>");
  nb_observer::bind_observer<ObservableExample>(
      m, "ObserverObservableExample",
      "Skeleton::Observer<Skeleton::ObservableExample>");
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::ObservableExample");
    nb::class_<ObservableExample, Observable<ObservableExample>> c(
        m, "ObservableExample", doc());
    c.def(nb::new_(
              [](double V) { return std::make_shared<ObservableExample>(V); }),
          "V"_a = 0);
    c.def("add_observer", &ObservableExample::add_observer, "Obs"_a,
          doc("add_observer"));
    c.def("remove_observer", &ObservableExample::remove_observer, "Obs"_a,
          doc("remove_observer"));
    c.def_prop_rw(
        "value", nb::overload_cast<>(&ObservableExample::value, nb::const_),
        nb::overload_cast<double>(&ObservableExample::value), doc("value"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<ObservableExample>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::ObserverExample");
    nb::class_<ObserverExample, Observer<ObservableExample>> c(
        m, "ObserverExample", doc());
    c.def(nb::new_([](const std::string& Name) {
            return std::make_shared<ObserverExample>(Name);
          }),
          "Name"_a = "");
    c.def(nb::new_([](const std::string& Name,
                      const std::shared_ptr<ObservableExample>& Obj) {
            return std::make_shared<ObserverExample>(Name, Obj);
          }),
          "Name"_a, "Obj"_a.none());
    c.def("notify_update", &ObserverExample::notify_update, "Obj"_a,
          doc("notify_update"));
    c.def_prop_ro("name", &ObserverExample::name, doc("name"));
    c.def_prop_ro("notify_count", &ObserverExample::notify_count,
                  doc("notify_count"));
    c.def_prop_ro("last_value", &ObserverExample::last_value,
                  doc("last_value"));
    c.def_prop_ro("add_count", &ObserverExample::add_count, doc("add_count"));
    c.def_prop_ro("remove_count", &ObserverExample::remove_count,
                  doc("remove_count"));
    c.def_prop_ro("observed", &ObserverExample::observed, doc("observed"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<ObserverExample>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::CacheExample");
    nb::class_<CacheExample, CacheInvalidatedObserver> c(m, "CacheExample",
                                                         doc());
    c.def(nb::new_([](const std::shared_ptr<ObservableExample>& Obj) {
            return std::make_shared<CacheExample>(Obj);
          }),
          "Obj"_a);
    c.def_prop_ro("value", &CacheExample::value, doc("value"));
    c.def_prop_ro("recompute_count", &CacheExample::recompute_count,
                  doc("recompute_count"));
    c.def_prop_ro("observed", &CacheExample::observed, doc("observed"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<CacheExample>();
  }
}
