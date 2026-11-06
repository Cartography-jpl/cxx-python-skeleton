%include "skeleton_common.i"

%{
#include "observer.h"
%}
%base_import(generic_object)

%skeleton_shared_ptr(Skeleton::CacheInvalidatedObserver);

namespace Skeleton {
%feature("director") CacheInvalidatedObserver;

template<class T> class Observable;
template<class T> class Observer : public GenericObject {
public:
  Observer();
  virtual ~Observer();
  virtual void notify_update(const T& Observed_object);
  virtual void notify_add(T& Observed_object);
  virtual void notify_remove(T& Observed_object);
};

class CacheInvalidatedObserver : public GenericObject {
public:
  CacheInvalidatedObserver();
  virtual void invalidate_cache();
  %python_attribute_with_set_virtual(cache_valid_flag, bool);
};

template<class T> class Observable : public GenericObject {
public:
  virtual ~Observable();
  void add_observer_and_keep_reference(boost::shared_ptr<Observer<T> >& Obs);
  void add_cache_invalidated_observer(CacheInvalidatedObserver& Obs);
  void remove_cache_invalidated_observer(CacheInvalidatedObserver& Obs);
  virtual void add_observer(Observer<T>& Obs) = 0;
  virtual void remove_observer(Observer<T>& Obs) = 0;
  void clear_observers();
  %python_attribute(number_observer, int);
  %python_attribute(number_cache_observer, int);
};
}

// List of things "import *" will include
%python_export("CacheInvalidatedObserver")
