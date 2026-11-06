#ifndef ARRAY_SAMPLE_H
#define ARRAY_SAMPLE_H
#include "printable.h"
#include <blitz/array.h>
#include <array>
#include <optional>
#include <stdexcept>

namespace Skeleton {
/****************************************************************/ /**
  A serialized class with fixed size array and optional members, the
  kind geocal has (boost::array is used in about 700 places there).
  These were boost::array and boost::optional until phase S3 of
  claude_notes/implementation_plan.md changed them to std::array and
  std::optional; the golden archives written with the boost types
  (unit_test_data/golden/array_sample.*) still load, and writing them
  again gives the same bytes.

  Python sees each member as a property: position_ as a numpy array
  (%python_attribute_std_array), matrix as a 2 x 3 numpy array, and
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
  /// A 3-vector. Public, because %python_attribute_std_array uses the
  /// member directly.
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
}

SKELETON_EXPORT_KEY(ArraySample);
#endif
