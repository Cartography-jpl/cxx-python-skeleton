#include "unit_test_support.h"
#include "array_sample.h"

using namespace Skeleton;

BOOST_FIXTURE_TEST_SUITE(column_major_sample, GlobalFixture)

BOOST_AUTO_TEST_CASE(layout)
{
  ColumnMajorSample s(2, 4);
  BOOST_CHECK_EQUAL(ColumnMajorSample::layout(s.data()), "column_major");
  BOOST_CHECK_EQUAL(s.data()(1, 3), 13);
  BOOST_CHECK_EQUAL(s.data().data()[1], 10);
  blitz::Array<double, 2> v = s.strided_view();
  BOOST_CHECK_EQUAL(ColumnMajorSample::layout(v), "strided");
  BOOST_CHECK_EQUAL(v.cols(), 2);
  BOOST_CHECK_EQUAL(v(1, 1), 12);
  BOOST_CHECK(v.data() == s.data().data());
  blitz::Array<double, 2> r(2, 3);
  r = 1, 2, 3, 4, 5, 6;
  BOOST_CHECK_EQUAL(ColumnMajorSample::layout(r), "row_major");
  s.data(r);
  BOOST_CHECK_EQUAL(ColumnMajorSample::layout(s.data()), "column_major");
  BOOST_CHECK_EQUAL(s.data()(1, 2), 6);
  BOOST_CHECK_EQUAL(ColumnMajorSample::sum(s.data()), 21);
  ColumnMajorSample::scale_in_place(r, 2);
  BOOST_CHECK_EQUAL(r(1, 2), 12);
}

BOOST_AUTO_TEST_SUITE_END()
