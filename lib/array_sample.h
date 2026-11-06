#ifndef ARRAY_SAMPLE_H
#define ARRAY_SAMPLE_H
#include "printable.h"
#include <blitz/array.h>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>

namespace Skeleton {
/****************************************************************/ /**
  A serialized class with fixed size array and optional members, the
  kind geocal has (boost::array is used in about 700 places there).
  These were boost::array and boost::optional until phase S3 of
  claude_notes/implementation_plan.md changed them to std::array and
  std::optional; the golden archives written with the boost types
  (unit_test_data/golden/array_sample.*) still load, and writing them
  again gives the same bytes.

  Python sees each member as a property: position_ as a numpy array,
  matrix as a 2 x 3 numpy array, and
  count and scale as an int / float or None.
*******************************************************************/
class ArraySample : public Printable<ArraySample> {
public:
  ArraySample()
    : position_{{1, 2, 3}}, matrix_{{{{1, 2, 3}}, {{4, 5, 6}}}}, count_(7)
  {
  }
  ~ArraySample() override = default;

  //-----------------------------------------------------------------------
  /// A 3-vector. Public, because the python property (position_)
  /// uses the member directly.
  //-----------------------------------------------------------------------

  std::array<double, 3> position_;

  //-----------------------------------------------------------------------
  /// position_ returned by value and by reference (python gets a numpy
  /// array copy either way).
  //-----------------------------------------------------------------------

  std::array<double, 3> position() const { return position_; }
  const std::array<double, 3>& position_ref() const { return position_; }

  //-----------------------------------------------------------------------
  /// A 2 x 3 matrix, stored as an array of arrays.
  //-----------------------------------------------------------------------

  blitz::Array<double, 2> matrix() const
  {
    blitz::Array<double, 2> res(2, 3);
    for(int i = 0; i < 2; ++i)
      for(int j = 0; j < 3; ++j)
        res(i, j) = matrix_[i][j];
    return res;
  }
  void matrix(const blitz::Array<double, 2>& M)
  {
    if(M.rows() != 2 || M.cols() != 3)
      throw std::runtime_error("matrix must be 2 x 3");
    for(int i = 0; i < 2; ++i)
      for(int j = 0; j < 3; ++j)
        matrix_[i][j] = M(i, j);
  }

  //-----------------------------------------------------------------------
  /// Optional values. count is set by default, scale is empty.
  //-----------------------------------------------------------------------

  const std::optional<int>& count() const { return count_; }
  void count(const std::optional<int>& V) { count_ = V; }
  const std::optional<double>& scale() const { return scale_; }
  void scale(const std::optional<double>& V) { scale_ = V; }

  virtual void print(std::ostream& Os) const { Os << "ArraySample"; }
private:
  std::array<std::array<double, 3>, 2> matrix_;
  std::optional<int> count_;
  std::optional<double> scale_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  Column-major (Fortran order) Blitz arrays and strided views, as
  framework uses for Fortran interop (ColumnMajorArray, fortranArray).

  This is a separate class from ArraySample, and isn't serialized: the
  Blitz serialization (blitz_array_serialize_support.cc) doesn't record
  the storage order, so a column-major array wouldn't round trip.

  Python sees data() and strided_view() as numpy views of the C++
  memory (read only, the C++ object is kept alive by the view). The
  static functions describe the array python passed in, so a test can
  check what C++ got (layout, and whether it shares python's memory).
*******************************************************************/
class ColumnMajorSample : public Printable<ColumnMajorSample> {
public:
  //-----------------------------------------------------------------------
  /// Rows x Cols column-major array, with element (i, j) = 10 * i + j.
  //-----------------------------------------------------------------------

  ColumnMajorSample(int Rows = 2, int Cols = 3)
    : data_(Rows, Cols, blitz::ColumnMajorArray<2>())
  {
    for(int i = 0; i < Rows; ++i)
      for(int j = 0; j < Cols; ++j)
        data_(i, j) = 10 * i + j;
  }
  ~ColumnMajorSample() override = default;

  //-----------------------------------------------------------------------
  /// The column-major array.
  //-----------------------------------------------------------------------

  const blitz::Array<double, 2>& data() const { return data_; }

  //-----------------------------------------------------------------------
  /// Copy V into a new column-major array.
  //-----------------------------------------------------------------------

  void data(const blitz::Array<double, 2>& V)
  {
    blitz::Array<double, 2> d(V.shape(), blitz::ColumnMajorArray<2>());
    d = V;
    data_.reference(d);
  }

  //-----------------------------------------------------------------------
  /// Every other column of data(), sharing its memory (not contiguous).
  //-----------------------------------------------------------------------

  blitz::Array<double, 2> strided_view() const
  {
    return data_(blitz::Range::all(), blitz::Range(0, blitz::toEnd, 2));
  }

  //-----------------------------------------------------------------------
  /// "row_major", "column_major" or "strided", from the strides of A.
  //-----------------------------------------------------------------------

  static std::string layout(const blitz::Array<double, 2>& A)
  {
    if(A.stride(1) == 1 && A.stride(0) == A.cols())
      return "row_major";
    if(A.stride(0) == 1 && A.stride(1) == A.rows())
      return "column_major";
    return "strided";
  }

  //-----------------------------------------------------------------------
  /// Address of the first element, to check for zero copy.
  //-----------------------------------------------------------------------

  static std::size_t data_address(const blitz::Array<double, 2>& A)
  {
    return reinterpret_cast<std::size_t>(A.data());
  }

  //-----------------------------------------------------------------------
  /// Sum of A (A may have any layout).
  //-----------------------------------------------------------------------

  static double sum(const blitz::Array<double, 2>& A) { return blitz::sum(A); }

  //-----------------------------------------------------------------------
  /// Multiply A by V in place. With no copy, python sees the change.
  //-----------------------------------------------------------------------

  static void scale_in_place(blitz::Array<double, 2>& A, double V) { A *= V; }

  virtual void print(std::ostream& Os) const { Os << "ColumnMajorSample"; }
private:
  blitz::Array<double, 2> data_;
};
}

SKELETON_EXPORT_KEY(ArraySample);
#endif
