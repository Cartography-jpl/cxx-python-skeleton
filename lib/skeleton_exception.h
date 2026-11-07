#pragma once

#include "printable.h"
#include <concepts>
#include <format>
#include <ios>
#include <memory>
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
  // Implicit on purpose: throw Exception("fmt {}", x) converts the string
  // literal to ExceptionFormat, as std::format_string works.
  // NOLINTNEXTLINE(google-explicit-constructor)
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

  Use this (or a class derived from it) for errors in the library, not
  std::runtime_error: python sees both as RuntimeError, but only this
  carries the stack trace and source location.

  Copying an Exception can't throw (the message, location and stack
  trace are shared between copies, copy on write), so it is safe to
  copy while an exception is being thrown and from any thread. A class
  derived from it that adds members of its own (e.g. a std::string)
  loses that, unless it shares them the same way.

  Two choices made on purpose:
  * It derives from Printable, and so from GenericObject and
    enable_shared_from_this, which makes it a little heavier than a
    plain exception class. It is bound to python (as cpp_exception on
    the python exception), and every bound class has to be a
    GenericObject for the type registry (register_type).
  * The stack trace uses Boost.Stacktrace rather than C++23
    std::stacktrace: gcc 14, the minimum, still needs -lstdc++exp for
    std::stacktrace.

  Thread safety: copies can be used on any thread (they share the data,
  copy on write). Don't change one Exception (operator<<) from two
  threads.
*******************************************************************/

// Copy without move on purpose (see the copy constructor).
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class Exception : public std::exception, public Printable<Exception> {
public:
  //-----------------------------------------------------------------------
  /// Default constructor. Can give an optional string describing
  /// the error. The string is used as is, it isn't a format string.
  //-----------------------------------------------------------------------

  explicit Exception(std::string W = "",
                     std::source_location Loc = std::source_location::current())
    : d_(std::make_shared<Data>(std::move(W), Loc))
  {
    capture();
  }

  //-----------------------------------------------------------------------
  // Constructor taking a std::format format string and its arguments (see
  // the class description). Hidden from doxygen with \cond: the docstring
  // generator (doxy2nanobind.py) gives all constructor overloads one
  // docstring, so this C++-only overload
  // would replace the text of the python Exception.__init__, which wraps
  // the constructor above.
  //-----------------------------------------------------------------------

  /// \cond
  template<class... Args>
    requires(sizeof...(Args) > 0)
  explicit Exception(ExceptionFormat<std::type_identity_t<Args>...> Fmt,
                     Args&&... A)
    : d_(std::make_shared<Data>(std::format(Fmt.fmt, std::forward<Args>(A)...),
                                Fmt.loc))
  {
    capture();
  }
  /// \endcond

  // Copies share the data, so copying can't throw. Declaring these also
  // means there is no move constructor (a move copies), so the data is
  // never null.
  Exception(const Exception&) noexcept = default;
  Exception& operator=(const Exception&) noexcept = default;
  ~Exception() override = default;

  //-----------------------------------------------------------------------
  /// Write to exception what() string. Stream manipulators carry over
  /// to later values, as for a stream (e.g. e << std::setprecision(8)
  /// << x << y formats both x and y with 8 digits).
  //-----------------------------------------------------------------------

  template<class T> Exception& operator<<(const T& V)
  {
    Data& d = own();
    std::ostringstream os;
    os.flags(d.flags);
    os.precision(d.precision);
    os.width(d.width);
    os.fill(d.fill);
    os << V;
    d.flags = os.flags();
    d.precision = os.precision();
    d.width = os.width();
    d.fill = os.fill();
    d.msg += os.str();
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

  const char* what() const noexcept override { return d_->msg.c_str(); }

  //-----------------------------------------------------------------------
  /// Where the exception was created (file, line, function). For a
  /// derived exception class this is where its constructor called
  /// ours, unless it passes its own source_location through.
  //-----------------------------------------------------------------------

  const std::source_location& source_location() const { return d_->loc; }

  //-----------------------------------------------------------------------
  /// The stack trace captured when the exception was created, one
  /// frame per line. Empty if capturing was turned off. Turning the
  /// addresses into function names is only done here, so it costs
  /// nothing unless it is asked for.
  //-----------------------------------------------------------------------

  [[nodiscard]] std::string stacktrace() const;

  //-----------------------------------------------------------------------
  /// Turn capturing the stack trace on or off (on by default), for
  /// code that throws and catches exceptions often.
  //-----------------------------------------------------------------------

  static void capture_stacktrace(bool V);
  static bool capture_stacktrace();
private:
  struct Data {
    Data(std::string Msg, std::source_location Loc)
      : msg(std::move(Msg)), loc(Loc)
    {
    }
    std::string msg;
    std::source_location loc;
    std::vector<void*> trace;
    // Stream state carried between operator<< calls.
    std::ios_base::fmtflags flags = std::ios_base::dec | std::ios_base::skipws;
    std::streamsize precision = 6;
    std::streamsize width = 0;
    char fill = ' ';
  };

  //-----------------------------------------------------------------------
  // The data, copied first if another Exception shares it (operator<< is
  // used while building the exception, when it normally has one owner).
  //-----------------------------------------------------------------------

  Data& own()
  {
    if(d_.use_count() > 1)
      d_ = std::make_shared<Data>(*d_);
    return *d_;
  }
  void capture();
  std::shared_ptr<Data> d_;
};

/****************************************************************/ /**
  An Exception for an argument of the wrong type. The python bindings
  turn it into a python TypeError (it isn't nanobind's nb::type_error,
  so the C++ library doesn't depend on python).
*******************************************************************/

class TypeError : public Exception {
public:
  using Exception::Exception;
};

/** \defgroup Error Error checking routines */
/*@{*/
//-----------------------------------------------------------------------
/// Range checks. Each throws an Exception if Val is out of range. The
/// message (and Exception::source_location()) give the caller's file
/// and line, from the defaulted source location argument; these used to
/// be macros only to get __FILE__ and __LINE__.
///
/// T is deduced from Val alone, so the limits convert to it
/// (range_check(n, 0, 10) works for a std::size_t n), but Val and the
/// limits are always compared as the same type: no mixed signed and
/// unsigned comparisons. The message is written with operator<<, so T
/// needs that and nothing more (no std::formatter).
//-----------------------------------------------------------------------

namespace detail {
template<class T>
[[noreturn]] void range_error(const T& Val, const T* Min, const T* Max,
                              const std::source_location& Loc)
{
  Exception e("", Loc);
  e << "Out of range error in file " << Loc.file_name() << " at line "
    << Loc.line() << "\n"
    << "Value:           " << Val;
  if(Min)
    e << "\nMinimum allowed: " << *Min;
  if(Max)
    e << "\nMaximum allowed: " << *Max;
  // The one-sided checks have always ended with a newline.
  if(!Min || !Max)
    e << "\n";
  throw e;
}
}

//-----------------------------------------------------------------------
/// Check Min <= Val < Max.
//-----------------------------------------------------------------------

template<class T>
inline void
range_check(const T& Val, const std::type_identity_t<T>& Min,
            const std::type_identity_t<T>& Max,
            std::source_location Loc = std::source_location::current())
{
  if(Val < Min || !(Val < Max))
    detail::range_error(Val, &Min, &Max, Loc);
}

//-----------------------------------------------------------------------
/// Check Min <= Val <= Max.
//-----------------------------------------------------------------------

template<class T>
inline void range_check_inclusive(
    const T& Val, const std::type_identity_t<T>& Min,
    const std::type_identity_t<T>& Max,
    std::source_location Loc = std::source_location::current())
{
  if(Val < Min || Max < Val)
    detail::range_error(Val, &Min, &Max, Loc);
}

//-----------------------------------------------------------------------
/// Check Min <= Val.
//-----------------------------------------------------------------------

template<class T>
inline void
range_min_check(const T& Val, const std::type_identity_t<T>& Min,
                std::source_location Loc = std::source_location::current())
{
  if(Val < Min)
    detail::range_error(Val, &Min, static_cast<const T*>(nullptr), Loc);
}

//-----------------------------------------------------------------------
/// Check Val < Max.
//-----------------------------------------------------------------------

template<class T>
inline void
range_max_check(const T& Val, const std::type_identity_t<T>& Max,
                std::source_location Loc = std::source_location::current())
{
  if(!(Val < Max))
    detail::range_error(Val, static_cast<const T*>(nullptr), &Max, Loc);
}
}
/*@}*/
