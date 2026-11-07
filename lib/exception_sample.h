#pragma once
#include "skeleton_exception.h"
#include <stdexcept>
#include <exception>

namespace Skeleton {
/****************************************************************/ /**
  Simple class to test how C++ exceptions get passed to python (see
  printing_test.py).
*******************************************************************/
class ExceptionSample : public Printable<ExceptionSample> {
public:
  ExceptionSample() = default;
  //-----------------------------------------------------------------------
  /// Throw a Skeleton::Exception with the given message.
  //-----------------------------------------------------------------------
  void throw_skeleton_exception(const std::string& Msg) const
  {
    throw Exception(Msg);
  }
  //-----------------------------------------------------------------------
  /// Throw a std::exception (not derived from Skeleton::Exception)
  //-----------------------------------------------------------------------
  void throw_std_exception(const std::string& Msg) const
  {
    throw std::out_of_range(Msg);
  }
  //-----------------------------------------------------------------------
  /// Throw a Skeleton::TypeError (python TypeError).
  //-----------------------------------------------------------------------
  void throw_type_error(const std::string& Msg) const { throw TypeError(Msg); }
  //-----------------------------------------------------------------------
  /// Throw something not derived from std::exception at all.
  //-----------------------------------------------------------------------
  void throw_unknown() const { throw 1; }
  //-----------------------------------------------------------------------
  /// Use range_check, throws if V isn't in [0, 10).
  //-----------------------------------------------------------------------
  int checked_value(int V) const
  {
    range_check(V, 0, 10);
    return V;
  }
  //-----------------------------------------------------------------------
  /// Virtual function a python subclass can override (and raise in).
  //-----------------------------------------------------------------------
  virtual int value() const { return 1; }
  //-----------------------------------------------------------------------
  /// Call value(), and if it throws catch the exception in C++ and
  /// return its what(). This is what C++ code (e.g. a log message) sees
  /// when a python override raises.
  //-----------------------------------------------------------------------
  std::string value_error_message() const
  {
    try {
      value();
    } catch(const std::exception& e) {
      return e.what();
    }
    return "";
  }
  //-----------------------------------------------------------------------
  /// Call value() and copy the exception before rethrowing (exercises
  /// the PythonException copy, which has to keep the python error).
  //-----------------------------------------------------------------------
  int value_rethrow_copy() const
  {
    try {
      return value();
    } catch(...) {
      std::exception_ptr p = std::current_exception();
      std::rethrow_exception(p);
    }
  }
  //-----------------------------------------------------------------------
  /// Throw a Skeleton::Exception and return the caught exception, so
  /// python can look at its stack trace and source location.
  //-----------------------------------------------------------------------
  static Exception caught_exception(const std::string& Msg)
  {
    try {
      throw Exception(Msg);
    } catch(const Exception& e) {
      return e;
    }
  }
  virtual void print(std::ostream& Os) const { Os << "ExceptionSample"; }
};
}
