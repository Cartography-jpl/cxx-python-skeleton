#include "array_sample.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
template<class Archive>
void ArraySample::serialize(Archive& ar, const unsigned int)
{
  SKELETON_GENERIC_BASE(ArraySample);
  ar & SKELETON_NVP_(position) & SKELETON_NVP_(matrix)
    & SKELETON_NVP_(count) & SKELETON_NVP_(scale);
}

SKELETON_IMPLEMENT(ArraySample);
#endif
