#include "blitz_array_serialize_support.h"
#include "skeleton_serialize_support.h"
#include "skeleton_exception.h"

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
#include <boost/serialization/array.hpp>
#include <array>
#include <string>

namespace {
//-----------------------------------------------------------------------
// Names of the shape elements in the archive, which differ by rank (the
// format of the one function per rank this replaced).
//-----------------------------------------------------------------------

template<int N>
constexpr std::array<const char*, static_cast<std::size_t>(N)> shape_names()
{
  if constexpr(N == 1)
    return {"size"};
  else if constexpr(N == 2)
    return {"rows", "cols"};
  else if constexpr(N == 3)
    return {"rows", "cols", "depth"};
  else if constexpr(N == 4)
    return {"shp0", "shp1", "shp2", "shp3"};
  else {
    static_assert(N == 5, "blitz serialization supports ranks 1 to 5");
    return {"shp0", "shp1", "shp2", "shp3", "shp4"};
  }
}

//-----------------------------------------------------------------------
// The archive holds the shape and then the values in row-major order.
// Return A itself if it is already stored that way (row-major,
// ascending, contiguous), otherwise a row-major copy. A column-major,
// reversed or strided array used to be written in memory order (or read
// out of bounds, for a reversed one). The base isn't in the archive, so
// an array with a nonzero base is an error rather than silently losing
// it.
//-----------------------------------------------------------------------

template<class T, int N>
blitz::Array<T, N> row_major(const blitz::Array<T, N>& A)
{
  for(int i = 0; i < N; ++i)
    if(A.lbound(i) != 0)
      throw Skeleton::Exception(
          "Can't serialize a blitz::Array with a nonzero base (lbound({}) = "
          "{}): the archive format doesn't record the base",
          i, A.lbound(i));
  bool direct = A.isStorageContiguous();
  for(int i = 0; i < N && direct; ++i)
    direct = A.isRankStoredAscending(i) && A.ordering(i) == N - 1 - i;
  if(direct)
    return A;
  blitz::Array<T, N> res(A.shape());
  res = A;
  return res;
}
}

template<class Archive, class T, int N>
void boost::serialization::save(Archive& ar, const blitz::Array<T, N>& A,
                                const unsigned /* version */)
{
  const blitz::Array<T, N> a = row_major(A);
  constexpr auto names = shape_names<N>();
  for(int i = 0; i < N; ++i) {
    int extent = a.extent(i);
    ar << SKELETON_NVP2(names[static_cast<std::size_t>(i)], extent);
  }
  if(a.size() > 0)
    ar << SKELETON_NVP2("data",
                        boost::serialization::make_array(a.data(), a.size()));
}

//-----------------------------------------------------------------------
// Read the values into a new row-major array with base 0 and make A
// refer to it. Resizing A instead would keep A's storage order and base,
// so loading into a column-major array scrambled the values.
//-----------------------------------------------------------------------

template<class Archive, class T, int N>
void boost::serialization::load(Archive& ar, blitz::Array<T, N>& A,
                                const unsigned /* version */)
{
  constexpr auto names = shape_names<N>();
  blitz::TinyVector<int, N> shape;
  for(int i = 0; i < N; ++i)
    ar >> SKELETON_NVP2(names[static_cast<std::size_t>(i)],
                        shape(static_cast<unsigned>(i)));
  blitz::Array<T, N> res(shape);
  if(res.size() > 0)
    ar >> SKELETON_NVP2(
              "data", boost::serialization::make_array(res.data(), res.size()));
  A.reference(res);
}

#define SKELETON_BLITZ_SERIALIZE_INSTANTIATE(T, N)                             \
  template void boost::serialization::save(                                    \
      boost::archive::polymorphic_oarchive& ar, const blitz::Array<T, N>& A,   \
      const unsigned version);                                                 \
  template void boost::serialization::load(                                    \
      boost::archive::polymorphic_iarchive& ar, blitz::Array<T, N>& A,         \
      const unsigned version);
// Ranks 1 to R, for R = 2 or 5 (SKELETON_BLITZ_SERIALIZE_TYPES).
#define SKELETON_BLITZ_SERIALIZE_INSTANTIATE_2(T)                              \
  SKELETON_BLITZ_SERIALIZE_INSTANTIATE(T, 1)                                   \
  SKELETON_BLITZ_SERIALIZE_INSTANTIATE(T, 2)
#define SKELETON_BLITZ_SERIALIZE_INSTANTIATE_5(T)                              \
  SKELETON_BLITZ_SERIALIZE_INSTANTIATE_2(T)                                    \
  SKELETON_BLITZ_SERIALIZE_INSTANTIATE(T, 3)                                   \
  SKELETON_BLITZ_SERIALIZE_INSTANTIATE(T, 4)                                   \
  SKELETON_BLITZ_SERIALIZE_INSTANTIATE(T, 5)
#define SKELETON_BLITZ_SERIALIZE_INSTANTIATE_TYPE(T, R)                        \
  SKELETON_BLITZ_SERIALIZE_INSTANTIATE_##R(T)

SKELETON_BLITZ_SERIALIZE_TYPES(SKELETON_BLITZ_SERIALIZE_INSTANTIATE_TYPE)
#endif
