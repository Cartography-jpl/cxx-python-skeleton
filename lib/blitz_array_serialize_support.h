#pragma once
#include <blitz/array.h>

// Boost serialization of blitz::Array<T, N>, for *implementing* the
// serialization of classes with blitz arrays (as opposed to *using* the
// serialization).
#include "skeleton_config.h"
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
#include <boost/serialization/split_free.hpp>
#include <string>

//-----------------------------------------------------------------------
/// The element types blitz::Array serialization supports, each with the
/// highest rank supported (ranks 1 to that). X(T, R) for each; R is 2 or
/// 5. blitz_array_serialize_support.cc instantiates save and load for
/// these, and serialize() below rejects anything else at compile time
/// (instead of a link error). To support another type, add it here.
//-----------------------------------------------------------------------

#define SKELETON_BLITZ_SERIALIZE_TYPES(X)                                      \
  X(double, 5)                                                                 \
  X(float, 5)                                                                  \
  X(int, 5)                                                                    \
  X(unsigned int, 5)                                                           \
  X(short int, 5)                                                              \
  X(unsigned short int, 5)                                                     \
  X(bool, 5)                                                                   \
  X(std::string, 2)

namespace Skeleton::detail {
template<class T> inline constexpr int blitz_serialize_max_rank = 0;
#define SKELETON_BLITZ_SERIALIZE_MAX_RANK(T, R)                                \
  template<> inline constexpr int blitz_serialize_max_rank<T> = R;
SKELETON_BLITZ_SERIALIZE_TYPES(SKELETON_BLITZ_SERIALIZE_MAX_RANK)
#undef SKELETON_BLITZ_SERIALIZE_MAX_RANK
}

namespace boost::serialization {
//-----------------------------------------------------------------------
/// Save and load a blitz::Array. The archive has the shape and the
/// values in row-major order: any storage order, stride or direction is
/// written that way, and a loaded array is always row-major with base 0.
/// A nonzero base can't be recorded, so saving one throws. Defined in
/// blitz_array_serialize_support.cc for the element types and ranks in
/// SKELETON_BLITZ_SERIALIZE_TYPES.
//-----------------------------------------------------------------------

template<class Archive, class T, int N>
void save(Archive& ar, const blitz::Array<T, N>& A, const unsigned version);
template<class Archive, class T, int N>
void load(Archive& ar, blitz::Array<T, N>& A, const unsigned version);

// What BOOST_SERIALIZATION_SPLIT_FREE does, for every T and N at once
// (the macro can't take a type with a comma in it, which is why there
// used to be a typedef and a macro line for each type and rank).
template<class Archive, class T, int N>
void serialize(Archive& ar, blitz::Array<T, N>& A, const unsigned version)
{
  static_assert(N >= 1 && N <= Skeleton::detail::blitz_serialize_max_rank<T>,
                "blitz::Array serialization: this element type and rank "
                "aren't in SKELETON_BLITZ_SERIALIZE_TYPES "
                "(blitz_array_serialize_support.h)");
  split_free(ar, A, version);
}
}

#endif
