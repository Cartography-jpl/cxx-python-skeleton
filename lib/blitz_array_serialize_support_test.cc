#include <blitz/array.h>
#include "unit_test_support.h"
#include "skeleton_serialize_support.h"
#include "skeleton_exception.h"
#include <string>
#include <type_traits>
#include <utility>
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
#include <boost/archive/polymorphic_xml_iarchive.hpp>
#include <boost/archive/polymorphic_xml_oarchive.hpp>
#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/archive/polymorphic_binary_oarchive.hpp>
#endif

using namespace Skeleton;
BOOST_FIXTURE_TEST_SUITE(blitz_array_serialize_support, GlobalFixture)

BOOST_AUTO_TEST_CASE(serialization)
{
  if(!have_serialize_supported())
    return;
  blitz::Array<double, 1> d(3);
  d = 1, 2, 3;
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
  std::ostringstream os;
  {
    boost::archive::polymorphic_xml_oarchive oa(os);
    oa << SKELETON_NVP(d);
  }
  if(false)
    std::cerr << os.str();

  std::istringstream is(os.str());
  boost::archive::polymorphic_xml_iarchive ia(is);
  blitz::Array<double, 1> dr;
  ia >> SKELETON_NVP(dr);
  BOOST_CHECK_EQUAL(d.rows(), dr.rows());
  SKELETON_CHECK_MATRIX_CLOSE(d, dr);
#endif
}

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
// Save and load, XML and binary, for every element type and rank with
// serialization support. The golden archive test only covers load.
template<class T, int D> void round_trip()
{
  blitz::TinyVector<int, D> shp;
  shp = 2;
  shp(0) = 3;
  blitz::Array<T, D> a(shp);
  for(std::size_t i = 0; i < a.size(); ++i) {
    if constexpr(std::is_same_v<T, std::string>)
      a.data()[i] = std::to_string(i % 3);
    else
      a.data()[i] = static_cast<T>(i % 3);
  }
  for(bool binary : {false, true}) {
    std::stringstream ss;
    if(binary) {
      boost::archive::polymorphic_binary_oarchive oa(ss);
      oa << SKELETON_NVP(a);
    } else {
      boost::archive::polymorphic_xml_oarchive oa(ss);
      oa << SKELETON_NVP(a);
    }
    blitz::Array<T, D> ar;
    if(binary) {
      boost::archive::polymorphic_binary_iarchive ia(ss);
      ia >> SKELETON_NVP2("a", ar);
    } else {
      boost::archive::polymorphic_xml_iarchive ia(ss);
      ia >> SKELETON_NVP2("a", ar);
    }
    BOOST_CHECK(blitz::all(ar.shape() == a.shape()));
    BOOST_CHECK(blitz::all(ar == a));
  }
  // An empty array
  blitz::Array<T, D> e;
  std::stringstream ss;
  {
    boost::archive::polymorphic_xml_oarchive oa(ss);
    oa << SKELETON_NVP(e);
  }
  blitz::Array<T, D> er(shp);
  boost::archive::polymorphic_xml_iarchive ia(ss);
  ia >> SKELETON_NVP2("e", er);
  BOOST_CHECK_EQUAL(er.size(), 0u);
}

// Ranks 1 to R.
template<class T, int R> void round_trip_all_rank()
{
  []<int... I>(std::integer_sequence<int, I...>) {
    (round_trip<T, I + 1>(), ...);
  }(std::make_integer_sequence<int, R>());
}
#endif

BOOST_AUTO_TEST_CASE(all_types)
{
  if(!have_serialize_supported())
    return;
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
  // Every type and rank in the list: one without its instantiation in
  // blitz_array_serialize_support.cc fails to link this test.
#define ROUND_TRIP(T, R) round_trip_all_rank<T, R>();
  SKELETON_BLITZ_SERIALIZE_TYPES(ROUND_TRIP)
#undef ROUND_TRIP
#endif
}

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
// Save A and load it into Res (which may have any storage order or
// shape beforehand), through an XML or a binary archive.
template<class T, int D>
void save_load(const blitz::Array<T, D>& A, blitz::Array<T, D>& Res,
               bool Binary)
{
  std::stringstream ss;
  if(Binary) {
    boost::archive::polymorphic_binary_oarchive oa(ss);
    oa << SKELETON_NVP2("a", A);
  } else {
    boost::archive::polymorphic_xml_oarchive oa(ss);
    oa << SKELETON_NVP2("a", A);
  }
  if(Binary) {
    boost::archive::polymorphic_binary_iarchive ia(ss);
    ia >> SKELETON_NVP2("a", Res);
  } else {
    boost::archive::polymorphic_xml_iarchive ia(ss);
    ia >> SKELETON_NVP2("a", Res);
  }
}

// 2 x 3 array with element (i, j) = 10 * i + j, in the given storage.
template<class S> blitz::Array<double, 2> test_matrix(const S& Storage)
{
  blitz::Array<double, 2> a(2, 3, Storage);
  for(int i = 0; i < 2; ++i)
    for(int j = 0; j < 3; ++j)
      a(i, j) = 10 * i + j;
  return a;
}
#endif

// Any storage order is written in row-major order (a column-major array
// written in memory order would come back scrambled), and a loaded array
// is always row-major.
BOOST_AUTO_TEST_CASE(column_major)
{
  if(!have_serialize_supported())
    return;
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
  blitz::Array<double, 2> a = test_matrix(blitz::ColumnMajorArray<2>());
  for(bool binary : {false, true}) {
    blitz::Array<double, 2> r;
    save_load(a, r, binary);
    BOOST_CHECK(blitz::all(r == a));
    BOOST_CHECK_EQUAL(r.ordering(0), 1);
  }
  // Rank 3
  blitz::Array<int, 3> a3(2, 3, 4, blitz::ColumnMajorArray<3>());
  for(int i = 0; i < 2; ++i)
    for(int j = 0; j < 3; ++j)
      for(int k = 0; k < 4; ++k)
        a3(i, j, k) = 100 * i + 10 * j + k;
  for(bool binary : {false, true}) {
    blitz::Array<int, 3> r;
    save_load(a3, r, binary);
    BOOST_CHECK(blitz::all(r == a3));
  }
#endif
}

// A strided view (every other column) is written as its values.
BOOST_AUTO_TEST_CASE(strided)
{
  if(!have_serialize_supported())
    return;
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
  blitz::Array<double, 2> a(3, 6);
  for(int i = 0; i < 3; ++i)
    for(int j = 0; j < 6; ++j)
      a(i, j) = 10 * i + j;
  blitz::Array<double, 2> v =
      a(blitz::Range::all(), blitz::Range(0, blitz::toEnd, 2));
  for(bool binary : {false, true}) {
    blitz::Array<double, 2> r;
    save_load(v, r, binary);
    BOOST_CHECK_EQUAL(r.rows(), 3);
    BOOST_CHECK_EQUAL(r.cols(), 3);
    BOOST_CHECK(blitz::all(r == v));
  }
#endif
}

// The archive doesn't record the base, so a nonzero base is an error,
// not silently dropped.
BOOST_AUTO_TEST_CASE(nonzero_base)
{
  if(!have_serialize_supported())
    return;
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
  blitz::Array<double, 1> a(blitz::Range(1, 3));
  a = 1, 2, 3;
  blitz::Array<double, 1> r;
  BOOST_CHECK_THROW(save_load(a, r, false), Exception);
#endif
}

// A reversed array (negative stride) passed the contiguity check, but
// data() is its last element in memory, so save read past the block.
BOOST_AUTO_TEST_CASE(reversed)
{
  if(!have_serialize_supported())
    return;
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
  blitz::Array<double, 1> a(3);
  a = 1, 2, 3;
  blitz::Array<double, 1> rev = a.reverse(0);
  for(bool binary : {false, true}) {
    blitz::Array<double, 1> r;
    save_load(rev, r, binary);
    BOOST_CHECK(blitz::all(r == rev));
  }
#endif
}

// Loading into a column-major target kept its storage order and
// scrambled the values.
BOOST_AUTO_TEST_CASE(load_into_column_major)
{
  if(!have_serialize_supported())
    return;
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
  blitz::Array<double, 2> a = test_matrix(blitz::GeneralArrayStorage<2>());
  for(bool binary : {false, true}) {
    blitz::Array<double, 2> r(1, 1, blitz::ColumnMajorArray<2>());
    save_load(a, r, binary);
    BOOST_CHECK(blitz::all(r == a));
  }
#endif
}

BOOST_AUTO_TEST_SUITE_END()
