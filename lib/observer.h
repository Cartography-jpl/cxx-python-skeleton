#pragma once
#include "null_deleter.h"
#include "generic_object.h"
#include "skeleton_exception.h"
#include <memory>
#include <algorithm>
#include <vector>

namespace Skeleton {
template<class T> class Observable;
/****************************************************************/ /**
  Simple Mixin to be an Observer of another object of class T. We get
  notified when the other object is updated.

  This is a copy of the Observer in framework (lib/Support/observer.h),
  which is geocal's version plus CacheInvalidatedObserver, duplicate
  detection and serialization of the observer lists, so the skeleton
  tests the pattern both projects use.

  A note on lifetime of objects. When an Observer is destroyed,
  nothing special needs to be done. The Observable is automatically
  notified that the Object no longer exists, and shouldn't receive
  notify_update messages anymore. Likewise, nothing special happens
  when the Observable is destroyed, this Observer simply receives no
  more messages from that Observable since it never changes after
  it is dead.

  The Observer can be owned any way at all (stack, member, shared_ptr,
  python): the Observable only keeps a weak_ptr made from this_obj, a
  non-owning pointer with exactly the lifetime of the Observer.

  The relationship between Observer and Observable is m to n, a
  Observer can be attached to any number of Observables, and likewise
  an Observable can have any number of Observers attached.

  Thread safety: not thread safe. Register, notify and destroy
  observers on one thread, and don't destroy an observer while it is
  being notified (the Observable's weak_ptr doesn't keep it alive).
*******************************************************************/
template<class T> class Observer : public virtual GenericObject {
public:
  Observer() : this_obj(to_ptr(*this)) {}

  //-----------------------------------------------------------------------
  /// Not copyable or movable. this_obj points at this object, and the
  /// Observables we are registered with hold weak_ptrs made from it. A
  /// copy of this_obj would point at the original, so registering the
  /// copy would register the original (and the copy would never be
  /// notified). A derived class that needs copies writes its own copy
  /// constructor, calling our default constructor, and decides there
  /// whether the copy registers. (Deleting the copy also removes the
  /// implicit move, which would have the same problem.)
  //-----------------------------------------------------------------------

  Observer(const Observer&) = delete;
  Observer& operator=(const Observer&) = delete;
  ~Observer() override = default;

  //-----------------------------------------------------------------------
  /// Called when the Observed object is updated.
  //-----------------------------------------------------------------------

  virtual void notify_update(const T& /* Observed_object */) {}

  //-----------------------------------------------------------------------
  /// Called when an object is added to an Observable. Default is to
  /// do nothing.
  ///
  /// There is a version with and without the Observed_object. A normal
  /// Observer uses the first, CacheInvalidatedObserver-like classes that
  /// don't care about the type use the second.
  //-----------------------------------------------------------------------

  virtual void notify_add(T& /* Observed_object */) {}
  virtual void notify_add() {}

  //-----------------------------------------------------------------------
  /// Called when an object is removed from an Observable. Default is to
  /// do nothing.
  //-----------------------------------------------------------------------

  virtual void notify_remove(T& /* Observed_object */) {}
  virtual void notify_remove() {}
private:
  // Shared ptr that we can use to get weak_ptr to. This object has
  // exactly the same lifetime as the containing object.
  std::shared_ptr<Observer> this_obj;
  friend class Observable<T>;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
  template<class Archive>
  void save(Archive& ar, const unsigned int version) const;
  template<class Archive> void load(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  Special case observer where a cache is invalidated when an
  Observable changes. One of these can observe any number of
  Observables of different types.

  Thread safety: not thread safe. Register, notify and destroy
  observers on one thread, and don't destroy an observer while it is
  being notified (the Observable's weak_ptr doesn't keep it alive).
*******************************************************************/
class CacheInvalidatedObserver : public virtual GenericObject {
public:
  CacheInvalidatedObserver() : this_obj_(to_ptr(*this)) {}

  //-----------------------------------------------------------------------
  /// Not copyable or movable, for the same reason as Observer.
  //-----------------------------------------------------------------------

  CacheInvalidatedObserver(const CacheInvalidatedObserver&) = delete;
  CacheInvalidatedObserver& operator=(const CacheInvalidatedObserver&) = delete;
  ~CacheInvalidatedObserver() override = default;

  //-----------------------------------------------------------------------
  /// Called when the cache is invalidated.
  //-----------------------------------------------------------------------

  virtual void invalidate_cache() { cache_valid = false; }
  // The shared_ptr the Observable makes its weak_ptr from (read only:
  // resetting it would break the observer).
  const std::shared_ptr<CacheInvalidatedObserver>& this_obj()
  {
    return this_obj_;
  }

  // Python is happier with functions vs accessing cache_valid. So
  // return and allow setting of cache_valid
  virtual bool cache_valid_flag() const { return cache_valid; }
  virtual void cache_valid_flag(bool V) { cache_valid = V; }
protected:
  // mutable: a const accessor that fills the cache marks it valid.
  mutable bool cache_valid = false;
  // Shared ptr that we can use to get weak_ptr to. This object has
  // exactly the same lifetime as the containing object.
  std::shared_ptr<CacheInvalidatedObserver> this_obj_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
  template<class Archive>
  void save(Archive& ar, const unsigned int version) const;
  template<class Archive> void load(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  Mixin for a class that allows other classes to observe its
  state. When the object state has been updated, the class should
  call notify_update_do.

  In some cases, you might actually want the lifetime of the Observer
  to be controlled by the Observable. This would be for a class that
  has no other purpose than to observe another class, say something
  that writes out diagnostic messages whenever the object is updated.
  In those cases, you can use add_observer_and_keep_reference instead
  of just add_observer. This stashes a copy of the shared_ptr so that
  as long as the Observable exists the registered Observer will also.

  Serialization: we don't want to serialize every observer (say a
  logger), and we don't want to serialize none of them (we would lose
  the connections inside a larger object). So the observer lists are
  written with the two pass weak_ptr rule (weak_ptr_serialize_support.h):
  an observer is kept only if it is also written to the archive some
  other way.

  Thread safety: not thread safe. Register, notify and destroy
  observers on one thread, and don't destroy an observer while it is
  being notified (the Observable's weak_ptr doesn't keep it alive).
*******************************************************************/
template<class T> class Observable : public virtual GenericObject {
public:
  //-----------------------------------------------------------------------
  /// Not copyable or movable. A copy would notify the observers of the
  /// original, which never registered with it and got no notify_add,
  /// and would keep the original's ref_list observers alive too.
  //-----------------------------------------------------------------------

  Observable(const Observable&) = delete;
  Observable& operator=(const Observable&) = delete;
  ~Observable() override = default;

  //-----------------------------------------------------------------------
  /// Add an observer
  //-----------------------------------------------------------------------

  virtual void add_observer(Observer<T>& Obs) = 0;

  //-----------------------------------------------------------------------
  /// Add a CacheInvalidatedObserver
  //-----------------------------------------------------------------------

  void add_cache_invalidated_observer(CacheInvalidatedObserver& Obs)
  {
    auto p = points_to(Obs.this_obj().get());
    if(std::ranges::find_if(colist, p) == colist.end())
      colist.push_back(std::weak_ptr<CacheInvalidatedObserver>(Obs.this_obj()));
  }
  void remove_cache_invalidated_observer(CacheInvalidatedObserver& Obs)
  {
    std::erase_if(colist, points_to(Obs.this_obj().get()));
  }

  //-----------------------------------------------------------------------
  /// Add an observer and keep a reference to it. See the discussion in
  /// the class description for details.
  //-----------------------------------------------------------------------

  void add_observer_and_keep_reference(const std::shared_ptr<Observer<T>>& Obs)
  {
    if(!Obs)
      throw Exception("add_observer_and_keep_reference: Obs is null");
    ref_list.push_back(Obs);
    add_observer(*Obs);
  }

  //-----------------------------------------------------------------------
  /// Remove an observer
  //-----------------------------------------------------------------------

  virtual void remove_observer(Observer<T>& Obs) = 0;

  //-----------------------------------------------------------------------
  /// Remove all observers through calls to the remove_observer routine
  //-----------------------------------------------------------------------

  void clear_observers()
  {
    // Copy out shared_ptrs into a separate list since olist will be
    // changed by remove_observer as we go.
    std::vector<std::shared_ptr<Observer<T>>> obs_pointers;
    for(auto& w : olist)
      if(auto p = w.lock())
        obs_pointers.push_back(p);
    std::vector<std::shared_ptr<CacheInvalidatedObserver>> cobs_pointers;
    for(auto& w : colist)
      if(auto p = w.lock())
        cobs_pointers.push_back(p);
    for(auto& p : obs_pointers)
      remove_observer(*p);
    for(auto& p : cobs_pointers)
      remove_cache_invalidated_observer(*p);
  }

  //-----------------------------------------------------------------------
  /// Number of live observers and cache observers (for testing).
  //-----------------------------------------------------------------------

  int number_observer() const
  {
    return static_cast<int>(std::ranges::count_if(
        olist, [](const auto& w) { return !w.expired(); }));
  }
  int number_cache_observer() const
  {
    return static_cast<int>(std::ranges::count_if(
        colist, [](const auto& w) { return !w.expired(); }));
  }
protected:
  //-----------------------------------------------------------------------
  /// Function to call to notify Observers of a state change. The object
  /// should pass itself to this function, so it can be passed to the
  /// Observers.
  //-----------------------------------------------------------------------

  void notify_update_do(const T& Self) const
  {
    clean_dead_ptr();
    // Copy, an observer may add or remove observers when notified.
    auto ol = olist;
    for(auto& w : ol)
      if(auto p = w.lock())
        p->notify_update(Self);
    auto cl = colist;
    for(auto& w : cl)
      if(auto p = w.lock())
        p->invalidate_cache();
  }

  //-----------------------------------------------------------------------
  // Predicate: a weak_ptr in olist or colist that points to P (an
  // expired one never does).
  //-----------------------------------------------------------------------

  template<class U> static auto points_to(const U* P)
  {
    return [P](const std::weak_ptr<U>& W) {
      auto sp = W.lock();
      return sp && sp.get() == P;
    };
  }

  //-----------------------------------------------------------------------
  /// Add an observer. Adding one that is already there does nothing.
  //-----------------------------------------------------------------------

  void add_observer_do(Observer<T>& Obs, T& t)
  {
    auto p = points_to(Obs.this_obj.get());
    if(std::ranges::find_if(olist, p) == olist.end()) {
      olist.push_back(std::weak_ptr<Observer<T>>(Obs.this_obj));
      Obs.notify_add(t);
      Obs.notify_add();
    }
  }
  void add_observer_do(Observer<T>& Obs)
  {
    auto p = points_to(Obs.this_obj.get());
    if(std::ranges::find_if(olist, p) == olist.end()) {
      olist.push_back(std::weak_ptr<Observer<T>>(Obs.this_obj));
      Obs.notify_add();
    }
  }

  //-----------------------------------------------------------------------
  /// Remove an observer
  //-----------------------------------------------------------------------

  void remove_observer_do(Observer<T>& Obs, T& t)
  {
    std::erase_if(olist, points_to(Obs.this_obj.get()));
    Obs.notify_remove(t);
    Obs.notify_remove();
  }
  void remove_observer_do(Observer<T>& Obs)
  {
    std::erase_if(olist, points_to(Obs.this_obj.get()));
    Obs.notify_remove();
  }

  //-----------------------------------------------------------------------
  /// Remove any dead pointers.
  //-----------------------------------------------------------------------

  void clean_dead_ptr() const
  {
    std::erase_if(olist, [](const auto& w) { return w.expired(); });
    std::erase_if(colist, [](const auto& w) { return w.expired(); });
  }
  // mutable: notify_update_do (const) drops expired entries. std::vector
  // rather than the std::list of the original: Boost writes both the same
  // way, so archives are unchanged.
  mutable std::vector<std::weak_ptr<Observer<T>>> olist;
  mutable std::vector<std::weak_ptr<CacheInvalidatedObserver>> colist;
  std::vector<std::shared_ptr<Observer<T>>> ref_list;

  //-----------------------------------------------------------------------
  /// Used when loading (see observer_serialize_support.h). An observer
  /// that registers itself in its load() can be loaded while our list is
  /// being loaded, so it adds itself to olist before its entry from the
  /// archive is read. Keep the archive's order (so writing again gives
  /// the same archive) and add the ones registered during the load that
  /// aren't already there.
  //-----------------------------------------------------------------------

  template<class U>
  static void merge_loaded(std::vector<std::weak_ptr<U>>& Lst,
                           std::vector<std::weak_ptr<U>>& Loaded)
  {
    for(auto& w : Lst) {
      auto p = w.lock();
      if(!p || std::ranges::find_if(Loaded, points_to(p.get())) == Loaded.end())
        Loaded.push_back(w);
    }
    Lst.swap(Loaded);
  }
  void merge_loaded(
      std::vector<std::weak_ptr<Observer<T>>>& Loaded_olist,
      std::vector<std::weak_ptr<CacheInvalidatedObserver>>& Loaded_colist)
  {
    merge_loaded(olist, Loaded_olist);
    merge_loaded(colist, Loaded_colist);
  }
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
  template<class Archive>
  void save(Archive& ar, const unsigned int version) const;
  template<class Archive> void load(Archive& ar, const unsigned int version);
private:
  // Only T derives from Observable<T> (T is the observed class, which
  // passes itself to notify_update_do).
  Observable() = default;
  friend T;
};
}

//-----------------------------------------------------------------------
/// Typedefs and export keys for Observer<NAME> and Observable<NAME>
/// (ObserverNAME, ObservableNAME). Use in the header of NAME; the
/// matching SKELETON_OBSERVER_SERIALIZE goes in its .cc file
/// (observer_serialize_support.h).
//-----------------------------------------------------------------------

#define SKELETON_EXPORT_OBSERVER_KEY(NAME)                                     \
  namespace Skeleton {                                                         \
  using Observer##NAME = Observer<NAME>;                                       \
  using Observable##NAME = Observable<NAME>;                                   \
  }                                                                            \
  SKELETON_EXPORT_KEY(Observer##NAME);                                         \
  SKELETON_EXPORT_KEY(Observable##NAME);

SKELETON_EXPORT_KEY(CacheInvalidatedObserver);
