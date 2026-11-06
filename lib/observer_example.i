%include "skeleton_common.i"

%{
#include "observer_example.h"
%}
%base_import(generic_object)
%base_import(observer)

%skeleton_shared_ptr(Skeleton::ObservableExample);
%skeleton_shared_ptr(Skeleton::ObserverExample);
%skeleton_shared_ptr(Skeleton::CacheExample);
namespace Skeleton {
  class ObservableExample;
}
%skeleton_shared_ptr(Skeleton::Observable<Skeleton::ObservableExample>);
%skeleton_shared_ptr(Skeleton::Observer<Skeleton::ObservableExample>);

namespace Skeleton {
// A director on a template instance (framework has 6 of these), so a
// python class can be an Observer.
%feature("director") Observer<ObservableExample>;
%template(ObservableObservableExample) Skeleton::Observable<Skeleton::ObservableExample>;
%template(ObserverObservableExample) Skeleton::Observer<Skeleton::ObservableExample>;

class ObservableExample : public Observable<ObservableExample> {
public:
  ObservableExample(double V = 0);
  virtual void add_observer(Observer<ObservableExample>& Obs);
  virtual void remove_observer(Observer<ObservableExample>& Obs);
  %python_attribute_with_set(value, double);
  std::string print_to_string() const;
  %pickle_serialization();
};

class ObserverExample : public Observer<ObservableExample> {
public:
  ObserverExample(const std::string& Name = "");
  ObserverExample(const std::string& Name,
                  const boost::shared_ptr<ObservableExample>& Obj);
  virtual void notify_update(const ObservableExample& Obj);
  %python_attribute(name, std::string);
  %python_attribute(notify_count, int);
  %python_attribute(last_value, double);
  %python_attribute(add_count, int);
  %python_attribute(remove_count, int);
  %python_attribute(observed, boost::shared_ptr<ObservableExample>);
  std::string print_to_string() const;
  %pickle_serialization();
};

class CacheExample : public CacheInvalidatedObserver {
public:
  CacheExample(const boost::shared_ptr<ObservableExample>& Obj);
  %python_attribute(value, double);
  %python_attribute(recompute_count, int);
  %python_attribute(observed, boost::shared_ptr<ObservableExample>);
  std::string print_to_string() const;
  %pickle_serialization();
};
}

// List of things "import *" will include
%python_export("ObservableExample", "ObserverExample", "CacheExample",
               "ObservableObservableExample", "ObserverObservableExample")
