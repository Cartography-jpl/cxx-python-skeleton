#include "unit_test_support.h"
#include "raster_sample.h"
#include "skeleton_exception.h"
#include <vector>

using namespace Skeleton;

BOOST_FIXTURE_TEST_SUITE(raster_sample, GlobalFixture)

namespace {
// value 1000 * line + sample, the same as BufferRasterSample
blitz::Array<int, 2> expected(int Lstart, int Sstart, int Nl, int Ns)
{
  blitz::Array<int, 2> r(Nl, Ns);
  for(int i = 0; i < Nl; ++i)
    for(int j = 0; j < Ns; ++j)
      r(i, j) = 1000 * (Lstart + i) + Sstart + j;
  return r;
}

class TestCalc : public CalcRasterSample {
public:
  TestCalc() : CalcRasterSample(7, 9) {}
  mutable int ncalc = 0;
protected:
  void calc(int Lstart, int Sstart) const override
  {
    ++ncalc;
    for(int i = 0; i < data.rows(); ++i)
      for(int j = 0; j < data.cols(); ++j)
        data(i, j) = 1000 * (Lstart + i) + Sstart + j + 0.4;
  }
};
}

BOOST_AUTO_TEST_CASE(memory)
{
  MemoryRasterSample m(expected(0, 0, 5, 6));
  BOOST_CHECK_EQUAL(m.number_line(), 5);
  BOOST_CHECK_EQUAL(m.number_sample(), 6);
  BOOST_CHECK_MATRIX_CLOSE(m.read(1, 2, 3, 4), expected(1, 2, 3, 4));
  // The old pointer interface still works.
  std::vector<int> buf(12);
  m.read_ptr(1, 2, 3, 4, buf.data());
  BOOST_CHECK_EQUAL(buf[0], 1002);
  BOOST_CHECK_EQUAL(buf[11], 3005);
  // Read into a strided view: every other column of a bigger array.
  blitz::Array<int, 2> big(3, 8);
  big = -1;
  m.read_into_checked(
      1, 2, to_mdspan(big(blitz::Range::all(), blitz::Range(0, 6, 2))));
  BOOST_CHECK_EQUAL(big(0, 0), 1002);
  BOOST_CHECK_EQUAL(big(0, 1), -1);
  BOOST_CHECK_EQUAL(big(2, 6), 3005);
  // Write, from blitz and from a pointer.
  blitz::Array<int, 2> v(2, 2);
  v = 1, 2, 3, 4;
  m.write(3, 4, v);
  BOOST_CHECK_EQUAL(m.data()(4, 5), 4);
  std::vector<int> w{7, 8, 9};
  m.write_ptr(0, 0, 1, 3, w.data());
  BOOST_CHECK_EQUAL(m.data()(0, 2), 9);
  BOOST_CHECK_THROW(m.read(4, 0, 2, 1), Exception);
  BOOST_CHECK_THROW(m.read_ptr(0, 5, 1, 2, buf.data()), Exception);
  // Negative sizes are caught before the mdspan is made.
  BOOST_CHECK_THROW(m.read_ptr(0, 0, -1, 2, buf.data()), Exception);
  BOOST_CHECK_THROW(m.read_ptr(0, 0, 1, -2, buf.data()), Exception);
  BOOST_CHECK_THROW(m.write(4, 5, v), Exception);
}

BOOST_AUTO_TEST_CASE(buffer)
{
  BufferRasterSample b(6, 8);
  // Contiguous: the pointer goes straight to the "C library".
  BOOST_CHECK_MATRIX_CLOSE(b.read(1, 2, 3, 4), expected(1, 2, 3, 4));
  std::vector<int> buf(6);
  b.read_ptr(2, 3, 2, 3, buf.data());
  BOOST_CHECK_EQUAL(buf[5], 3005);
  // One row of a column major array is still contiguous.
  blitz::Array<int, 2> one(1, 4, blitz::ColumnMajorArray<2>());
  b.read_into_checked(5, 0, to_mdspan(one));
  BOOST_CHECK_EQUAL(b.number_copy(), 0);
  BOOST_CHECK_EQUAL(one(0, 3), 5003);
  // Column major, and a column slice: copied through a temporary.
  blitz::Array<int, 2> cm(3, 4, blitz::ColumnMajorArray<2>());
  b.read_into_checked(1, 2, to_mdspan(cm));
  BOOST_CHECK_EQUAL(b.number_copy(), 1);
  BOOST_CHECK_MATRIX_CLOSE(cm, expected(1, 2, 3, 4));
  blitz::Array<int, 2> big(3, 6);
  b.read_into_checked(0, 0,
                      to_mdspan(big(blitz::Range::all(), blitz::Range(1, 4))));
  BOOST_CHECK_EQUAL(b.number_copy(), 2);
  BOOST_CHECK_EQUAL(big(2, 4), 2003);
  // Write, contiguous and strided.
  blitz::Array<int, 2> v(2, 2);
  v = 1, 2, 3, 4;
  b.write(0, 0, v);
  BOOST_CHECK_EQUAL(b.number_copy(), 2);
  b.write(4, 6, v.transpose(blitz::secondDim, blitz::firstDim));
  BOOST_CHECK_EQUAL(b.number_copy(), 3);
  BOOST_CHECK_EQUAL(b.read(0, 0, 1, 2)(0, 1), 2);
  BOOST_CHECK_EQUAL(b.read(4, 6, 1, 2)(0, 1), 3);
  BOOST_CHECK_THROW(b.read(5, 0, 2, 1), Exception);
}

BOOST_AUTO_TEST_CASE(read_with_pad)
{
  BufferRasterSample b(6, 8);
  // Off the top left corner: the inside part is a strided view of res.
  blitz::Array<int, 2> r = b.read_with_pad(-1, -2, 3, 4, -7);
  BOOST_CHECK_EQUAL(r(0, 0), -7);
  BOOST_CHECK_EQUAL(r(0, 3), -7);
  BOOST_CHECK_EQUAL(r(1, 1), -7);
  BOOST_CHECK_EQUAL(r(1, 2), 0);
  BOOST_CHECK_EQUAL(r(2, 3), 1001);
  // Off the bottom only: still contiguous, no copy needed.
  int ncopy = b.number_copy();
  r.reference(b.read_with_pad(4, 0, 4, 8));
  BOOST_CHECK_EQUAL(b.number_copy(), ncopy);
  BOOST_CHECK_EQUAL(r(1, 7), 5007);
  BOOST_CHECK_EQUAL(r(2, 0), 0);
  // All outside.
  r.reference(b.read_with_pad(10, 10, 2, 2, 3));
  BOOST_CHECK_EQUAL(blitz::sum(r), 12);
  // Inside: the same as read.
  BOOST_CHECK_MATRIX_CLOSE(b.read_with_pad(1, 1, 2, 2), expected(1, 1, 2, 2));
}

BOOST_AUTO_TEST_CASE(sub)
{
  auto b = std::make_shared<BufferRasterSample>(6, 8);
  SubRasterSample s(b, 2, 3, 3, 4);
  BOOST_CHECK_EQUAL(s.number_line(), 3);
  BOOST_CHECK_EQUAL(s.number_sample(), 4);
  BOOST_CHECK_MATRIX_CLOSE(s.read(0, 0, 3, 4), expected(2, 3, 3, 4));
  BOOST_CHECK_THROW(s.read(0, 0, 4, 4), Exception);
  blitz::Array<int, 2> r = s.read_with_pad(-1, 0, 2, 4);
  BOOST_CHECK_EQUAL(r(0, 0), 0);
  BOOST_CHECK_EQUAL(r(1, 0), 2003);
  blitz::Array<int, 2> v(1, 1);
  v = 42;
  s.write(2, 3, v);
  BOOST_CHECK_EQUAL(b->read(4, 6, 1, 1)(0, 0), 42);
  BOOST_CHECK_THROW(SubRasterSample(b, 4, 0, 3, 1), Exception);
  BOOST_CHECK_EQUAL(
      s.print_to_string(),
      "SubRasterSample 3 x 4 at (2, 3) of BufferRasterSample 6 x 8");
}

BOOST_AUTO_TEST_CASE(calc)
{
  auto c = std::make_shared<TestCalc>();
  BOOST_CHECK_MATRIX_CLOSE(c->read(1, 2, 3, 4), expected(1, 2, 3, 4));
  BOOST_CHECK_EQUAL(c->ncalc, 1);
  // Through a SubRasterSample, and the old pointer interface.
  SubRasterSample s(c, 1, 1, 5, 5);
  std::vector<int> buf(4);
  s.read_ptr(1, 1, 2, 2, buf.data());
  BOOST_CHECK_EQUAL(buf[3], 3003);
  BOOST_CHECK_THROW(c->write(0, 0, expected(0, 0, 1, 1)), Exception);
}

BOOST_AUTO_TEST_CASE(copy)
{
  // Tiles that don't divide the size: the last ones are smaller views of
  // the same buffer.
  auto c = std::make_shared<TestCalc>();
  MemoryRasterSample m(7, 9);
  copy_raster(*c, m, 3, 4);
  BOOST_CHECK_MATRIX_CLOSE(m.data(), expected(0, 0, 7, 9));
  BOOST_CHECK_EQUAL(c->ncalc, 9);
  BufferRasterSample b(7, 9);
  MemoryRasterSample m2(7, 9);
  copy_raster(b, m2, 100, 100);
  BOOST_CHECK_MATRIX_CLOSE(m2.data(), expected(0, 0, 7, 9));
  BOOST_CHECK_THROW(copy_raster(b, *std::make_shared<MemoryRasterSample>(7, 8)),
                    Exception);
  BOOST_CHECK_THROW(copy_raster(b, m2, 0, 1), Exception);
  // Into a read only one.
  BOOST_CHECK_THROW(copy_raster(b, *c), Exception);
}

BOOST_AUTO_TEST_SUITE_END()
