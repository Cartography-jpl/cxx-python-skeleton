#include "unit_test_support.h"
#include "blitz_mdspan.h"

using namespace Skeleton;

BOOST_FIXTURE_TEST_SUITE(blitz_mdspan, GlobalFixture)

BOOST_AUTO_TEST_CASE(contiguous)
{
  blitz::Array<double, 2> a(2, 3);
  a = 1, 2, 3, 4, 5, 6;
  auto m = to_mdspan(a);
  BOOST_CHECK_EQUAL(m.extent(0), 2);
  BOOST_CHECK_EQUAL(m.extent(1), 3);
  BOOST_CHECK_EQUAL(m.stride(0), 3);
  BOOST_CHECK_EQUAL(m.stride(1), 1);
  BOOST_CHECK_EQUAL(m(1, 2), 6);
  m(0, 1) = 20;
  BOOST_CHECK_EQUAL(a(0, 1), 20);
  blitz::Array<double, 2> b = to_blitz(m);
  BOOST_CHECK(b.data() == a.data());
  b(1, 0) = 40;
  BOOST_CHECK_EQUAL(a(1, 0), 40);
  // layout_right mdspan converts too
  std::array<double, 6> d{1, 2, 3, 4, 5, 6};
  md::mdspan<double, md::dextents<std::ptrdiff_t, 2>> r(d.data(), 2, 3);
  blitz::Array<double, 2> c = to_blitz(strided_mdspan<double, 2>(r));
  BOOST_CHECK_EQUAL(c(1, 1), 5);
}

BOOST_AUTO_TEST_CASE(strided_slice)
{
  blitz::Array<double, 2> a(3, 4);
  for(int i = 0; i < 3; ++i)
    for(int j = 0; j < 4; ++j)
      a(i, j) = 10 * i + j;
  blitz::Array<double, 2> s = a(blitz::Range(0, 2, 2), blitz::Range(1, 3, 2));
  auto m = to_mdspan(s);
  BOOST_CHECK_EQUAL(m.stride(0), 8);
  BOOST_CHECK_EQUAL(m.stride(1), 2);
  BOOST_CHECK_EQUAL(m(1, 1), 23);
  blitz::Array<double, 2> b = to_blitz(m);
  BOOST_CHECK_EQUAL(b(1, 0), 21);
  b(0, 1) = -1;
  BOOST_CHECK_EQUAL(a(0, 3), -1);
}

BOOST_AUTO_TEST_CASE(column_major)
{
  blitz::Array<double, 2> a(2, 3, blitz::ColumnMajorArray<2>());
  for(int i = 0; i < 2; ++i)
    for(int j = 0; j < 3; ++j)
      a(i, j) = 10 * i + j;
  auto m = to_mdspan(a);
  BOOST_CHECK_EQUAL(m.stride(0), 1);
  BOOST_CHECK_EQUAL(m.stride(1), 2);
  BOOST_CHECK_EQUAL(m(1, 2), 12);
  // layout_left mdspan
  std::array<double, 6> d{0, 10, 1, 11, 2, 12};
  md::mdspan<double, md::dextents<std::ptrdiff_t, 2>, md::layout_left> l(
      d.data(), 2, 3);
  blitz::Array<double, 2> b = to_blitz(strided_mdspan<double, 2>(l));
  BOOST_CHECK_EQUAL(b(1, 2), 12);
  BOOST_CHECK_EQUAL(b.stride(0), 1);
  b(0, 0) = 5;
  BOOST_CHECK_EQUAL(d[0], 5);
}

BOOST_AUTO_TEST_CASE(owner)
{
  // The owner is kept alive by the blitz array and its copies.
  auto data = std::make_shared<std::vector<double>>(6, 1.0);
  std::weak_ptr<std::vector<double>> w = data;
  blitz::Array<double, 2> copy;
  {
    strided_mdspan<double, 2> m(
        data->data(), typename strided_mdspan<double, 2>::mapping_type(
                          md::dextents<std::ptrdiff_t, 2>(2, 3),
                          std::array<std::ptrdiff_t, 2>{3, 1}));
    blitz::Array<double, 2> b = to_blitz(m, data);
    data.reset();
    BOOST_CHECK(!w.expired());
    copy.reference(b);
  }
  BOOST_CHECK(!w.expired());
  BOOST_CHECK_EQUAL(copy(1, 2), 1.0);
  copy.free();
  BOOST_CHECK(w.expired());
}

BOOST_AUTO_TEST_CASE(negative_stride)
{
  blitz::Array<double, 1> a(4);
  a = 1, 2, 3, 4;
  a.reverseSelf(0);
  BOOST_CHECK_THROW(to_mdspan(a), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
