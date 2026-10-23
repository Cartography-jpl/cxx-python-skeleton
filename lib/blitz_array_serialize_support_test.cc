#include <blitz/array.h>
#include "unit_test_support.h"
#include "skeleton_serialize_support.h"
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
#include <boost/archive/polymorphic_xml_iarchive.hpp>
#include <boost/archive/polymorphic_xml_oarchive.hpp>
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

BOOST_AUTO_TEST_SUITE_END()
  
