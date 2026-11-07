#ifndef NUMPY_SAMPLE_H
#define NUMPY_SAMPLE_H
#include "printable.h"
#include <blitz/array.h>
#include <optional>

namespace Skeleton {
/****************************************************************/ /**
  Simple class to test handling of numpy lifetime issues when wrapped
  in the python bindings. We define this here, then have a python test
  (numpy_sample_test.py) to test this out.
*******************************************************************/
class NumpySample : public Printable<NumpySample> {
public:
  NumpySample() : data_(2, 2) { data_ = 1, 2, 3, 4; }
  explicit NumpySample(const blitz::Array<double, 2>& Din) : data_(Din) {}
  const blitz::Array<double, 2>& data() const { return data_; }
  //-----------------------------------------------------------------------
  /// Used to test std::optional<blitz::Range> in the bindings. Returns
  /// a disengaged optional if Return_empty is true, otherwise a Range
  /// covering the rows of data().
  //-----------------------------------------------------------------------
  std::optional<blitz::Range> maybe_range(bool Return_empty) const
  {
    if(Return_empty)
      return std::optional<blitz::Range>();
    return std::optional<blitz::Range>(blitz::Range(0, data_.rows() - 1));
  }
private:
  blitz::Array<double, 2> data_;
};
}
#endif
