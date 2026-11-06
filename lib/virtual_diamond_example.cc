#include "virtual_diamond_example.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
template<class Archive>
void NestedLike::serialize(Archive& ar, const unsigned int)
{
  ar& BOOST_SERIALIZATION_BASE_OBJECT_NVP(WithParameterLike);
}

template<class Archive> void RootLike::serialize(Archive&, const unsigned int)
{
  SKELETON_GENERIC_BASE(RootLike);
}

template<class Archive> void Diamond::serialize(Archive& ar, const unsigned int)
{
  SKELETON_GENERIC_BASE(Diamond);
  ar& BOOST_SERIALIZATION_BASE_OBJECT_NVP(RootLike) &
      BOOST_SERIALIZATION_BASE_OBJECT_NVP(NestedLike) & SKELETON_NVP_(name) &
      SKELETON_NVP2("parameter", p_);
}

SKELETON_IMPLEMENT(NestedLike);
SKELETON_IMPLEMENT(RootLike);
SKELETON_IMPLEMENT(Diamond);
#endif
