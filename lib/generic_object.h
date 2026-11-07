#pragma once
// The project configuration (CXX_PYTHON_RULES_NAMESPACE,
// SKELETON_HAVE_BOOST_SERIALIZATION); the cxx_python_rules headers reach
// it through this header.
#include "skeleton_config.h"
#include <memory>
#include <string>

namespace boost::serialization {
class access;
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
  A base class that everything exposed to python can be cast to.

  It derives from std::enable_shared_from_this, so the python bindings
  can find the shared_ptr that already owns an object: without that,
  nanobind makes a new control block for every python to C++
  conversion, and a weak_ptr to an object passed in from python expires
  at once. It is a virtual
  base of most classes, so use std::dynamic_pointer_cast on the result
  of shared_from_this().

  Non-owning pointers to a GenericObject must be made with to_ptr /
  const_pointer (null_deleter.h), which leave weak_this alone.

  Thread safety: as for a standard library type: const member functions
  can be called from several threads at once; anything else needs
  exclusive access.
*******************************************************************/
// Copy without move on purpose (see the protected members).
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class GenericObject : public std::enable_shared_from_this<GenericObject> {
public:
  GenericObject() = default;
  // Have a virtual member function, which forces RTTI information to
  // be available.
  virtual ~GenericObject() = default;
protected:
  // Copying only as part of a derived class, so a derived object can't
  // be sliced through a GenericObject& (C++ Core Guidelines C.67).
  // Copying doesn't copy the shared_from_this owner (the copy is a new
  // object). No move operations: this is a virtual base, where a move
  // assignment can run more than once (gcc's -Wvirtual-move-assign), so a
  // move copies, as it always has here.
  GenericObject(const GenericObject&) = default;
  GenericObject& operator=(const GenericObject&) = default;
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

}

SKELETON_EXPORT_KEY(GenericObject);
