#include "multi_base_example.h"
#include "observer_serialize_support.h"
#include <boost/core/demangle.hpp>
#include <typeinfo>

using namespace Skeleton;

std::string MultiBaseExample::type_name(const std::shared_ptr<GenericObject>& G)
{
  if(!G)
    return "None";
  const GenericObject& g = *G;
  return boost::core::demangle(typeid(g).name());
}

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
SKELETON_OBSERVER_SERIALIZE(CameraLike);

template<class Archive>
void CameraLike::serialize(Archive& ar, const unsigned int)
{
  SKELETON_GENERIC_BASE(CameraLike);
  ar& BOOST_SERIALIZATION_BASE_OBJECT_NVP(ObservableCameraLike) &
      BOOST_SERIALIZATION_BASE_OBJECT_NVP(WithParameterLike);
}

template<class Archive>
void SimpleCameraLike::serialize(Archive& ar, const unsigned int)
{
  ar& BOOST_SERIALIZATION_BASE_OBJECT_NVP(CameraLike) &
      SKELETON_NVP_(focal_length) & SKELETON_NVP_(number_band);
}

SKELETON_IMPLEMENT(CameraLike);
SKELETON_IMPLEMENT(SimpleCameraLike);
#endif
