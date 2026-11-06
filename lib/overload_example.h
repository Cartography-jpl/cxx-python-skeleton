#ifndef OVERLOAD_EXAMPLE_H
#define OVERLOAD_EXAMPLE_H
#include "numpy_sample.h"
#include "printable.h"
#include <blitz/array.h>
#include <boost/shared_ptr.hpp>
#include <string>

namespace Skeleton {
/****************************************************************/ /**
  Overloads that differ only in the type of the first argument, like
  geocal's Resampler::resample_field (a Blitz double array, a
  shared_ptr to a RasterImage, or a file name). Each returns the name
  of the overload called, so a test can check which one python picks
  (claude_notes/implementation_plan.md S6.1 item 7).
*******************************************************************/
class OverloadExample : public Printable<OverloadExample> {
public:
  OverloadExample() = default;
  ~OverloadExample() override = default;
  static std::string which(const blitz::Array<double, 2>&) { return "array"; }
  static std::string which(const boost::shared_ptr<NumpySample>&)
  {
    return "NumpySample";
  }
  static std::string which(const std::string&) { return "string"; }
  virtual void print(std::ostream& Os) const { Os << "OverloadExample"; }
};
}
#endif
