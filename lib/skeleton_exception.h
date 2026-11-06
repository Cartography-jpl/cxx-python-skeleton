#ifndef SKELETON_EXCEPTION_H
#define SKELETON_EXCEPTION_H

#include "printable.h"
#include <concepts>
#include <format>
#include <ios>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Skeleton {

//-----------------------------------------------------------------------
/// Format string plus the location it was written at, for the
/// format-string Exception constructor. The source location has to be
/// a defaulted argument of this helper rather than of the constructor,
/// because it can't come after the variadic format arguments. Like
/// std::format_string, the format is checked against the argument
/// types at compile time.
//-----------------------------------------------------------------------

template<class... Args> struct ExceptionFormat {
  template<class S>
    requires std::convertible_to<const S&, std::string_view>
  consteval ExceptionFormat(
      const S& Fmt, std::source_location Loc = std::source_location::current())
    : fmt(Fmt), loc(Loc)
  {
  }
  std::format_string<Args...> fmt;
  std::source_location loc;
};

/****************************************************************/ /**
  This is the base of the exception hierarchy for Skeleton code. This
  can be written to like a stream to contain information about the
  exception. This is derived from the standard library std::exception

  The message can also be given as a std::format string:

     throw Exception("Line {} out of range [0, {})", ln, nline);

  The exception records where it was created (source_location()), and
  a stack trace (stacktrace(), see capture_stacktrace).
*******************************************************************/

class Exception : public std::exception, public Printable<Exception> {
public:
  //-----------------------------------------------------------------------
  /// Default constructor. Can give an optional string describing
  /// the error. The string is used as is, it isn't a format string.
  //-----------------------------------------------------------------------

  Exception(const std::string& W = "",
            std::source_location Loc = std::source_location::current())
    : msg_(W), loc_(Loc)
  {
    capture();
  }

  //-----------------------------------------------------------------------
  // Constructor taking a std::format format string and its arguments (see
  // the class description). Hidden from doxygen with \cond: the docstring
  // generator (doxy2swig, now doxy2nanobind) gives
  // all constructor overloads one docstring, so this C++-only overload
  // would replace the text of the python Exception.__init__, which wraps
  // the constructor above.
  //-----------------------------------------------------------------------

  /// \cond
  template<class... Args>
    requires(sizeof...(Args) > 0)
  Exception(ExceptionFormat<std::type_identity_t<Args>...> Fmt, Args&&... A)
    : msg_(std::format(Fmt.fmt, std::forward<Args>(A)...)), loc_(Fmt.loc)
  {
    capture();
  }
  /// \endcond

  Exception(const Exception& E) = default;
  Exception& operator=(const Exception& E) = default;
  ~Exception() noexcept override = default;

  //-----------------------------------------------------------------------
  /// Write to exception what() string. Stream manipulators carry over
  /// to later values, as for a stream (e.g. e << std::setprecision(8)
  /// << x << y formats both x and y with 8 digits).
  //-----------------------------------------------------------------------

  template<class T> inline Exception& operator<<(const T& V)
  {
    std::ostringstream os;
    os.flags(flags_);
    os.precision(precision_);
    os.width(width_);
    os.fill(fill_);
    os << V;
    flags_ = os.flags();
    precision_ = os.precision();
    width_ = os.width();
    fill_ = os.fill();
    msg_ += os.str();
    return *this;
  }

  //-----------------------------------------------------------------------
  /// Print out description of object.
  //-----------------------------------------------------------------------

  virtual void print(std::ostream& Os) const
  {
    Os << "Skeleton Exception:\n"
       << "=========================\n"
       << what() << "\n"
       << "=========================\n";
  }

  //-----------------------------------------------------------------------
  /// Description of what the error is.
  //-----------------------------------------------------------------------

  const char* what() const noexcept override { return msg_.c_str(); }

  //-----------------------------------------------------------------------
  /// Where the exception was created (file, line, function). For a
  /// derived exception class this is where its constructor called
  /// ours, unless it passes its own source_location through.
  //-----------------------------------------------------------------------

  const std::source_location& source_location() const { return loc_; }

  //-----------------------------------------------------------------------
  /// The stack trace captured when the exception was created, one
  /// frame per line. Empty if capturing was turned off. Turning the
  /// addresses into function names is only done here, so it costs
  /// nothing unless it is asked for.
  //-----------------------------------------------------------------------

  std::string stacktrace() const;

  //-----------------------------------------------------------------------
  /// Turn capturing the stack trace on or off (on by default), for
  /// code that throws and catches exceptions often.
  //-----------------------------------------------------------------------

  static void capture_stacktrace(bool V);
  static bool capture_stacktrace();
private:
  void capture();
  std::string msg_;
  std::source_location loc_;
  std::vector<void*> trace_;
  // Stream state carried between operator<< calls.
  std::ios_base::fmtflags flags_ = std::ios_base::dec | std::ios_base::skipws;
  std::streamsize precision_ = 6;
  std::streamsize width_ = 0;
  char fill_ = ' ';
};

/****************************************************************/ /**
  An Exception for an argument of the wrong type. The python bindings
  turn it into a python TypeError (it isn't nanobind's nb::type_error,
  so the C++ library doesn't depend on python).
*******************************************************************/

class TypeError : public Exception {
public:
  using Exception::Exception;
  ~TypeError() noexcept override = default;
};

/** \defgroup Error Error checking routines */
/*@{*/
//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

template<class T>
inline void range_check_template(const T& Val, // Value to be checked.
                                 const T& Min, // Minimum allowed value.
                                 const T& Max, // Maximum allowed value.
                                 const char* File, int Line)
{
  if(Val < Min || !(Val < Max)) {
    Exception e;
    e << "Out of range error in file " << File << " at line " << Line << "\n"
      << "Value:           " << Val << "\n"
      << "Minimum allowed: " << Min << "\n"
      << "Maximum allowed: " << Max;
    throw e;
  }
}

template<class T>
inline void
range_check_inclusive_template(const T& Val, // Value to be checked.
                               const T& Min, // Minimum allowed value.
                               const T& Max, // Maximum allowed value.
                               const char* File, int Line)
{
  if(Val < Min || Val > Max) {
    Exception e;
    e << "Out of range error in file " << File << " at line " << Line << "\n"
      << "Value:           " << Val << "\n"
      << "Minimum allowed: " << Min << "\n"
      << "Maximum allowed: " << Max;
    throw e;
  }
}

//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

#define range_check(V, Min, Max)                                               \
  Skeleton::range_check_template(V, Min, Max, __FILE__, __LINE__)
#define range_check_inclusive(V, Min, Max)                                     \
  Skeleton::range_check_inclusive_template(V, Min, Max, __FILE__, __LINE__)

//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

template<class T>
inline void range_min_check_template(const T& Val, // Value to be checked.
                                     const T& Min, // Minimum allowed value.
                                     const char* File, int Line)
{
  if(Val < Min) {
    Exception e;
    e << "Out of range error in file " << File << " at line " << Line << "\n"
      << "Value:           " << Val << "\n"
      << "Minimum allowed: " << Min << "\n";
    throw e;
  }
}

//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

#define range_min_check(V, Min)                                                \
  Skeleton::range_min_check_template(V, Min, __FILE__, __LINE__)

//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

template<class T>
inline void range_max_check_template(const T& Val, // Value to be checked.
                                     const T& Max, // Maximum allowed value.
                                     const char* File, int Line)
{
  if(!(Val < Max)) {
    Exception e;
    e << "Out of range error in file " << File << " at line " << Line << "\n"
      << "Value:           " << Val << "\n"
      << "Maximum allowed: " << Max << "\n";
    throw e;
  }
}

//-----------------------------------------------------------------------
/// Range check
//-----------------------------------------------------------------------

#define range_max_check(V, Max)                                                \
  Skeleton::range_max_check_template(V, Max, __FILE__, __LINE__)

}
/*@}*/
#endif
