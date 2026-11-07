#pragma once
#include "generic_object.h"
#include <concepts>
#include <format>
#include <sstream>
#include <string_view>

namespace Skeleton {

/****************************************************************/ /**
  This is a Mixin for classes that can be printed.

  C++ code prints with the usual "<<" (cout << foo), and the python
  bindings use print_to_string() for __str__. Both are implemented with
  print(std::ostream& Os), which a class T deriving from Printable<T>
  defines: "virtual void print(std::ostream& Os) const" in the first
  class that has it, "void print(std::ostream& Os) const override" in
  classes derived from that one.

  std::format works too: std::format("{}", foo) gives the same text as
  "<<" (see the std::formatter at the end of this file), and the
  std::string formatting options apply, e.g. std::format("{:>20}", foo).
*******************************************************************/

template<class T> class Printable : public virtual GenericObject {
public:
  //-----------------------------------------------------------------------
  /// Print to string. The python bindings use this for __str__.
  //-----------------------------------------------------------------------

  std::string print_to_string() const
  {
    std::ostringstream os;
    print_wrapper(os);
    return os.str();
  }

  //-----------------------------------------------------------------------
  /// Alternative to print for python: a python subclass overrides desc
  /// to give its own description. The default, an empty string, means
  /// "use print", so C++ classes need nothing.
  //-----------------------------------------------------------------------

  virtual std::string desc() const { return ""; }

  void print_wrapper(std::ostream& Os) const
  {
    std::string d = desc();
    if(!d.empty())
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
private:
  // Only T can derive from Printable<T> (so class A : Printable<B> is an
  // error, not a class that prints as a B).
  Printable() = default;
  friend T;
};

//-----------------------------------------------------------------------
/// Anything that can be printed with "<<" through Printable: a class
/// derived from Printable<T>, or from a class that is, which has
/// print(std::ostream&).
//-----------------------------------------------------------------------

template<class T>
concept PrintableObject = requires(const T& V, std::ostream& Os) {
  V.print_wrapper(Os);
  V.print(Os);
};
}

//-----------------------------------------------------------------------
/// std::format support for Printable classes, the same text as "<<".
/// This reuses the std::string_view formatter, so width, fill and
/// alignment work (e.g. "{:>20}").
//-----------------------------------------------------------------------

// A specialization of std::formatter for a program-defined type, which the
// standard allows.
template<Skeleton::PrintableObject T>
// NOLINTNEXTLINE(bugprone-std-namespace-modification)
struct std::formatter<T, char> : std::formatter<std::string_view, char> {
  auto format(const T& V, std::format_context& Ctx) const
  {
    return std::formatter<std::string_view, char>::format(V.print_to_string(),
                                                          Ctx);
  }
};
