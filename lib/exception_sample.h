#ifndef EXCEPTION_SAMPLE_H
#define EXCEPTION_SAMPLE_H
#include "skeleton_exception.h"
#include <stdexcept>

namespace Skeleton {
/****************************************************************//**
  Simple class to test how C++ exceptions get passed to python (see
  printing_test.py).
*******************************************************************/
class ExceptionSample : public Printable<ExceptionSample> {
public:
  ExceptionSample() {}
  virtual ~ExceptionSample() {}
//-----------------------------------------------------------------------
/// Throw a Skeleton::Exception with the given message.
//-----------------------------------------------------------------------
  void throw_skeleton_exception(const std::string& Msg) const
  { throw Exception(Msg); }
//-----------------------------------------------------------------------
/// Throw a std::exception (not derived from Skeleton::Exception)
//-----------------------------------------------------------------------
  void throw_std_exception(const std::string& Msg) const
  { throw std::out_of_range(Msg); }
//-----------------------------------------------------------------------
/// Throw something not derived from std::exception at all.
//-----------------------------------------------------------------------
  void throw_unknown() const
  { throw 1; }
//-----------------------------------------------------------------------
/// Use range_check, throws if V isn't in [0, 10).
//-----------------------------------------------------------------------
  int checked_value(int V) const
  { range_check(V, 0, 10); return V; }
  virtual void print(std::ostream& Os) const { Os << "ExceptionSample"; }
};
}
#endif
