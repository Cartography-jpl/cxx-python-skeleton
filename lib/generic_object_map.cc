#include "generic_object_map.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
template<class Archive>
void GenericObjectMap::serialize(Archive& ar, const unsigned int)
{
  SKELETON_GENERIC_BASE(GenericObjectMap);
  using MapGenericObject =
      std::map<std::string, std::shared_ptr<GenericObject>>;
  ar& BOOST_SERIALIZATION_BASE_OBJECT_NVP(MapGenericObject);
}

SKELETON_IMPLEMENT(GenericObjectMap);
#endif
