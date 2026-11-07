#include "generic_object_map.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
template<class Archive>
void GenericObjectMap::serialize(Archive& ar, const unsigned int)
{
  SKELETON_GENERIC_BASE(GenericObjectMap);
  // Written under the element name the class used when it derived from
  // the map (BOOST_SERIALIZATION_BASE_OBJECT_NVP(MapGenericObject)), so
  // archives are unchanged.
  ar& SKELETON_NVP2("MapGenericObject", map_);
}

SKELETON_IMPLEMENT(GenericObjectMap);
#endif
