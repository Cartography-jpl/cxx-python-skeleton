#ifndef SKELETON_GENERIC_OBJECT_H
#define SKELETON_GENERIC_OBJECT_H
// These get defined in skeleton_config.h. Undef them to stop warnings
// if we change these.
#undef PACKAGE
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION
#include "skeleton_config.h"
#include <string>

namespace boost {
namespace serialization {
class access;
}
}

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
#include <boost/serialization/export.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#define SKELETON_EXPORT_KEY(NAME) BOOST_CLASS_EXPORT_KEY(Skeleton::NAME)
#define SKELETON_EXPORT_KEY2(NAME, KEY)                                        \
  BOOST_CLASS_EXPORT_KEY2(Skeleton::NAME, KEY)
#define SKELETON_SPLIT_MEMBER() BOOST_SERIALIZATION_SPLIT_MEMBER()
#define SKELETON_DONT_TRACK(NAME)                                              \
  BOOST_CLASS_TRACKING(Skeleton::NAME, boost::serialization::track_never)
#define SKELETON_CLASS_VERSION(NAME, N) BOOST_CLASS_VERSION(Skeleton::NAME, N)
#else
#define SKELETON_EXPORT_KEY(NAME)       /* Noop */
#define SKELETON_EXPORT_KEY2(NAME, KEY) /* Noop */
#define SKELETON_SPLIT_MEMBER()         /* Noop */
#define SKELETON_DONT_TRACK(NAME)       /* Noop */
#define SKELETON_CLASS_VERSION(NAME, N) /* Noop */
#endif

namespace Skeleton {
/****************************************************************/ /**
  For use with SWIG, it is useful to have a base class that
  everything can be cast to. This class doesn't provide any
  functionality, other than allowing casts.
*******************************************************************/
class GenericObject {
public:
  // Have a virtual member function, which forces RTTI information to
  // be available.
  virtual ~GenericObject() = default;
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

}

SKELETON_EXPORT_KEY(GenericObject);

#endif
