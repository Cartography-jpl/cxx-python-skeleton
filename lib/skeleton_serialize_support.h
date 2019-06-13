#ifndef SKELETON_SERIALIZE_SUPPORT_H
#define SKELETON_SERIALIZE_SUPPORT_H
// This contains various support routines for *implementing* the boost
// serialization (as opposed to *using*  the serialization
#include "skeleton_config.h"
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
// At boost 1.64, the header file to include for boost::array changed
// names. Pretty annoying, but select the right file to include
#ifdef HAVE_BOOST_SERIALIZATION_BOOST_ARRAY_HPP
#include <boost/serialization/boost_array.hpp>
#else
#include <boost/serialization/array.hpp>
#endif
#include <boost/archive/polymorphic_iarchive.hpp>
#include <boost/archive/polymorphic_oarchive.hpp>

// Do this a lot, so give a shortcut for this
#define SKELETON_NVP(x) BOOST_SERIALIZATION_NVP(x)
#define SKELETON_NVP_(x) boost::serialization::make_nvp(BOOST_PP_STRINGIZE(x), x ## _)
#define SKELETON_NVP2(x, y) boost::serialization::make_nvp(x, y)
#define SKELETON_IMPLEMENT(NAME) \
BOOST_CLASS_EXPORT_IMPLEMENT(Skeleton::NAME); \
template void NAME::serialize(boost::archive::polymorphic_oarchive& ar, \
				    const unsigned int version); \
template void NAME::serialize(boost::archive::polymorphic_iarchive& ar, \
				    const unsigned int version);
#define SKELETON_SPLIT_IMPLEMENT(NAME) \
BOOST_CLASS_EXPORT_IMPLEMENT(Skeleton::NAME); \
template void NAME::save(boost::archive::polymorphic_oarchive& ar, \
				    const unsigned int version) const; \
template void NAME::load(boost::archive::polymorphic_iarchive& ar, \
				    const unsigned int version);

#define SKELETON_BASE(NAME,BASE) boost::serialization::void_cast_register<Skeleton::NAME, Skeleton::BASE>();
#define SKELETON_GENERIC_BASE(NAME) SKELETON_BASE(NAME, GenericObject);

using boost::archive::polymorphic_iarchive;
using boost::archive::polymorphic_oarchive;

#endif

#include "blitz_array_serialize_support.h"
#endif
