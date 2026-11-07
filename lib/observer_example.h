#ifndef OBSERVER_EXAMPLE_H
#define OBSERVER_EXAMPLE_H
#include "observer.h"
#include "printable.h"
#include "skeleton_exception.h"
#include <memory>
#include <string>
#include <utility>

namespace Skeleton {
/****************************************************************/ /**
  An Observable with one value. Setting the value notifies the
  observers, like a geocal Camera or Orbit notifying an
  ImageGroundConnection, or a framework StateVector.
*******************************************************************/
class ObservableExample : public Observable<ObservableExample>,
                          public Printable<ObservableExample> {
public:
  explicit ObservableExample(double V = 0) : value_(V) {}
  ~ObservableExample() override = default;
  void add_observer(Observer<ObservableExample>& Obs) override
  {
    add_observer_do(Obs, *this);
  }
  void remove_observer(Observer<ObservableExample>& Obs) override
  {
    remove_observer_do(Obs, *this);
  }

  //-----------------------------------------------------------------------
  /// The value. Setting it notifies the observers.
  //-----------------------------------------------------------------------

  double value() const { return value_; }
  void value(double V)
  {
    value_ = V;
    notify_update_do(*this);
  }
  virtual void print(std::ostream& Os) const
  {
    Os << "ObservableExample " << value_;
  }
private:
  double value_ = 0;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};
}

SKELETON_EXPORT_OBSERVER_KEY(ObservableExample);

namespace Skeleton {
/****************************************************************/ /**
  An Observer of ObservableExample that counts its notifications.

  If constructed with an ObservableExample, it registers itself with
  it in the constructor, and again when it is loaded from an archive
  (the two places framework and geocal register, see
  claude_notes/implementation_plan.md §2.4). Registering twice does
  nothing, so it doesn't matter if the Observable's observer list in
  the archive already had it.
*******************************************************************/
class ObserverExample : public Observer<ObservableExample>,
                        public Printable<ObserverExample> {
public:
  explicit ObserverExample(std::string Name = "") : name_(std::move(Name)) {}
  ObserverExample(std::string Name, std::shared_ptr<ObservableExample> Obj)
    : name_(std::move(Name)), observed_(std::move(Obj))
  {
    if(observed_)
      observed_->add_observer(*this);
  }
  ~ObserverExample() override = default;
  void notify_update(const ObservableExample& Obj) override
  {
    ++notify_count_;
    last_value_ = Obj.value();
  }
  void notify_add(ObservableExample&) override { ++add_count_; }
  void notify_remove(ObservableExample&) override { ++remove_count_; }
  const std::string& name() const { return name_; }

  //-----------------------------------------------------------------------
  /// Number of notify_update calls, and the value at the last one.
  //-----------------------------------------------------------------------

  int notify_count() const { return notify_count_; }
  double last_value() const { return last_value_; }

  //-----------------------------------------------------------------------
  /// Number of notify_add and notify_remove calls.
  //-----------------------------------------------------------------------

  int add_count() const { return add_count_; }
  int remove_count() const { return remove_count_; }

  //-----------------------------------------------------------------------
  /// The ObservableExample given to the constructor, if any.
  //-----------------------------------------------------------------------

  const std::shared_ptr<ObservableExample>& observed() const
  {
    return observed_;
  }
  virtual void print(std::ostream& Os) const
  {
    Os << "ObserverExample " << name_;
  }
private:
  std::string name_;
  std::shared_ptr<ObservableExample> observed_;
  int notify_count_ = 0;
  int add_count_ = 0;
  int remove_count_ = 0;
  double last_value_ = 0;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
  template<class Archive>
  void save(Archive& ar, const unsigned int version) const;
  template<class Archive> void load(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  A framework style cache: registers with an ObservableExample in the
  constructor (and on load), and recomputes its value only after the
  Observable changed.
*******************************************************************/
class CacheExample : public CacheInvalidatedObserver,
                     public Printable<CacheExample> {
public:
  explicit CacheExample(std::shared_ptr<ObservableExample> Obj)
    : observed_(std::move(Obj))
  {
    if(!observed_)
      throw Exception("CacheExample: Obj is null");
    observed_->add_cache_invalidated_observer(*this);
  }
  ~CacheExample() override = default;

  //-----------------------------------------------------------------------
  /// Twice the observed value, recomputed only when the cache is invalid.
  //-----------------------------------------------------------------------

  double value() const
  {
    if(!cache_valid) {
      cached_ = 2 * observed_->value();
      ++recompute_count_;
      const_cast<CacheExample*>(this)->cache_valid = true;
    }
    return cached_;
  }
  int recompute_count() const { return recompute_count_; }
  const std::shared_ptr<ObservableExample>& observed() const
  {
    return observed_;
  }
  virtual void print(std::ostream& Os) const { Os << "CacheExample"; }
private:
  CacheExample() = default;
  std::shared_ptr<ObservableExample> observed_;
  mutable double cached_ = 0;
  mutable int recompute_count_ = 0;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
  template<class Archive>
  void save(Archive& ar, const unsigned int version) const;
  template<class Archive> void load(Archive& ar, const unsigned int version);
};
}

SKELETON_EXPORT_KEY(ObservableExample);
SKELETON_EXPORT_KEY(ObserverExample);
SKELETON_EXPORT_KEY(CacheExample);
#endif
