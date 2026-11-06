#ifndef SKELETON_PRINTABLE_H
#define SKELETON_PRINTABLE_H
#include "generic_object.h"
#include <concepts>
#include <format>
#include <sstream>
#include <string_view>

namespace Skeleton {

/****************************************************************/ /**
  This is a Mixin for classes that can be printed.

  There are two different functions we want to use for printing. For
  normal C++ use, we want the usual "<<" notation, e.g., cout << foo.
  For use with languages such as Ruby that don't have a native stream
  type, we want to "print_to_string()", which returns a string that
  Ruby can directly work with.

  We implement both of these functions in terms of a third function,
  "print(ostream& Os)".

  Classes T that want to be printable in both C++ and other languages
  should derive from Printable<T>, and then define print(ostream& Os).

  std::format works too: std::format("{}", foo) gives the same text as
  "<<" (see the std::formatter at the end of this file), and the
  std::string formatting options apply, e.g. std::format("{:>20}", foo).
*******************************************************************/

template<class T> class Printable : public virtual GenericObject {
public:
  //-----------------------------------------------------------------------
  /// Print to string. This is primarily useful for SWIG wrappers to
  /// this class, e.g. a to_s method in ruby.
  //-----------------------------------------------------------------------

  std::string print_to_string() const
  {
    std::ostringstream os;
    print_wrapper(os);
    return os.str();
  }

  // Alternative to print, this works better with python. If this
  // is an empty string we fall back to print - so C++ classes work
  // without change but python can intercept this by overriding desc.
  virtual std::string desc() const { return ""; }

  void print_wrapper(std::ostream& Os) const
  {
    std::string d = desc();
    if(d != "")
      Os << d;
    else
      static_cast<const T*>(this)->print(Os);
  }

  //-----------------------------------------------------------------------
  /// Print to stream.
  //-----------------------------------------------------------------------

  friend std::ostream& operator<<(std::ostream& Os, const T& P)
  {
    P.print_wrapper(Os);
    return Os;
  }
};

//-----------------------------------------------------------------------
/// Anything that can be printed with "<<" through Printable: a class
/// derived from Printable<T>, or from a class that is.
//-----------------------------------------------------------------------

template<class T>
concept PrintableObject =
    requires(const T& V, std::ostream& Os) { V.print_wrapper(Os); };
}

//-----------------------------------------------------------------------
/// std::format support for Printable classes, the same text as "<<".
/// This reuses the std::string_view formatter, so width, fill and
/// alignment work (e.g. "{:>20}").
//-----------------------------------------------------------------------

template<Skeleton::PrintableObject T>
struct std::formatter<T, char> : std::formatter<std::string_view, char> {
  auto format(const T& V, std::format_context& Ctx) const
  {
    return std::formatter<std::string_view, char>::format(V.print_to_string(),
                                                          Ctx);
  }
};
#endif
