#ifndef NUMPY_SAMPLE_H
#define NUMPY_SAMPLE_H
#include "printable.h"
#include <blitz/array.h>
#include <boost/optional.hpp>

namespace Skeleton {
/****************************************************************/ /**
  Simple class to test handling of numpy lifetime issues when wrapped
  in swig. We define this here, then have a python test
  (numpy_sample_test.py) to test this out.
*******************************************************************/
class NumpySample : public Printable<NumpySample> {
public:
  NumpySample() : data_(2, 2) { data_ = 1, 2, 3, 4; }
  NumpySample(const blitz::Array<double, 2>& Din) : data_(Din) {}
  const blitz::Array<double, 2>& data() const { return data_; }
  //-----------------------------------------------------------------------
  /// Used to test the boost::optional<blitz::Range> swig typemap. Returns
  /// a disengaged optional if Return_empty is true, otherwise a Range
  /// covering the rows of data().
  //-----------------------------------------------------------------------
  boost::optional<blitz::Range> maybe_range(bool Return_empty) const
  {
    if(Return_empty)
      return boost::optional<blitz::Range>();
    return boost::optional<blitz::Range>(blitz::Range(0, data_.rows() - 1));
  }
private:
  blitz::Array<double, 2> data_;
};
}
#endif
