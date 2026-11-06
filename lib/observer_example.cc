#include "observer_example.h"
#include "observer_serialize_support.h"

using namespace Skeleton;

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
SKELETON_OBSERVER_SERIALIZE(ObservableExample);

template<class Archive>
void ObservableExample::serialize(Archive& ar, const unsigned int)
{
  ar& BOOST_SERIALIZATION_BASE_OBJECT_NVP(ObservableObservableExample);
  ar& SKELETON_NVP_(value);
}

template<class Archive>
void ObserverExample::serialize(Archive& ar, const unsigned int version)
{
  ar& BOOST_SERIALIZATION_BASE_OBJECT_NVP(ObserverObservableExample);
  ar& SKELETON_NVP_(name) & SKELETON_NVP_(observed) &
      SKELETON_NVP_(notify_count) & SKELETON_NVP_(add_count) &
      SKELETON_NVP_(remove_count) & SKELETON_NVP_(last_value);
  boost::serialization::split_member(ar, *this, version);
}

template<class Archive>
void ObserverExample::save(Archive&, const unsigned int) const
{
}

//-----------------------------------------------------------------------
// Register again after loading (the "register in load()" case). If the
// Observable in the archive already had us in its list this does
// nothing; if it didn't (we were only reachable through it by a
// weak_ptr that was dropped) this restores the connection.
//-----------------------------------------------------------------------

template<class Archive> void ObserverExample::load(Archive&, const unsigned int)
{
  if(observed_) {
    int add_count = add_count_;
    observed_->add_observer(*this);
    // Loading isn't a new add, keep the count from the archive.
    add_count_ = add_count;
  }
}

template<class Archive>
void CacheExample::serialize(Archive& ar, const unsigned int version)
{
  ar& BOOST_SERIALIZATION_BASE_OBJECT_NVP(CacheInvalidatedObserver);
  ar& SKELETON_NVP_(observed);
  boost::serialization::split_member(ar, *this, version);
}

template<class Archive>
void CacheExample::save(Archive&, const unsigned int) const
{
}

template<class Archive> void CacheExample::load(Archive&, const unsigned int)
{
  observed_->add_cache_invalidated_observer(*this);
}

SKELETON_IMPLEMENT(ObservableExample);
SKELETON_IMPLEMENT(ObserverExample);
SKELETON_IMPLEMENT(CacheExample);
#endif
