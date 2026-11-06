#include "generic_object.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
template<class Archive>
void GenericObject::serialize(Archive& /* ar */,
                              const unsigned int /* version */)
{
  // Nothing to do
}

SKELETON_IMPLEMENT(GenericObject);
#endif
