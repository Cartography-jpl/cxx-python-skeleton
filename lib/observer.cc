#include "observer.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
// Same as the Observer<T> serialization (observer_serialize_support.h):
// only a placeholder is written, but the object is marked as serialized
// so weak_ptr to it are kept. cache_valid isn't written, a loaded object
// starts with an invalid cache.
template<class Archive>
void CacheInvalidatedObserver::serialize(Archive& ar,
                                         const unsigned int version)
{
  SKELETON_GENERIC_BASE(CacheInvalidatedObserver);
  std::string p = "empty";
  ar& SKELETON_NVP2("placeholder", p);
  boost::serialization::split_member(ar, *this, version);
}

template<class Archive>
void CacheInvalidatedObserver::save(Archive&, const unsigned int) const
{
  add_ptr_serialized_reference(this);
}

template<class Archive>
void CacheInvalidatedObserver::load(Archive&, const unsigned int)
{
}

SKELETON_IMPLEMENT(CacheInvalidatedObserver);
#endif
