#include <blitz/array.h>
#include "unit_test_support.h"
#include "skeleton_serialize_support.h"
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
  BOOST_CHECK_MATRIX_CLOSE(d, dr);
#endif
}

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
// Save and load, XML and binary, for every element type and rank with
// serialization support. The golden archive test only covers load.
template<class T, int D> void round_trip()
{
  blitz::TinyVector<int, D> shp;
  for(int i = 0; i < D; ++i)
    shp(i) = (i == 0 ? 3 : 2);
  blitz::Array<T, D> a(shp);
  for(int i = 0; i < (int) a.size(); ++i)
    a.data()[i] = static_cast<T>(i % 3);
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

template<class T> void round_trip_all_rank()
{
  round_trip<T, 1>();
  round_trip<T, 2>();
  round_trip<T, 3>();
  round_trip<T, 4>();
  round_trip<T, 5>();
}
#endif

BOOST_AUTO_TEST_CASE(all_types)
{
  if(!have_serialize_supported())
    return;
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
  round_trip_all_rank<double>();
  round_trip_all_rank<float>();
  round_trip_all_rank<int>();
  round_trip_all_rank<unsigned int>();
  round_trip_all_rank<short int>();
  round_trip_all_rank<unsigned short int>();
  round_trip_all_rank<bool>();
#endif
}

BOOST_AUTO_TEST_SUITE_END()
  
