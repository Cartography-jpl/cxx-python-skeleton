#include "with_parameter_like.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
template<class Archive>
void WithParameterLike::serialize(Archive&, const unsigned int)
{
  SKELETON_GENERIC_BASE(WithParameterLike);
}

SKELETON_IMPLEMENT(WithParameterLike);
#endif
