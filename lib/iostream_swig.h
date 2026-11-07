#pragma once
#include "printable.h"
#include <iosfwd>
#include <string>
#include <utility>

namespace Skeleton {
/****************************************************************/ /**
  A simple class to test C++ functions taking a std::ostream or
  std::istream, which python calls with a file object
  (cxx_python_rules/nanobind/iostream.h). The name is from the SWIG
  bindings, kept for geocal's python code.

  Thread safety: as for a standard library type: const member functions
  can be called from several threads at once; anything else needs
  exclusive access.
*******************************************************************/

class IostreamSwig : public Printable<IostreamSwig> {
public:
  //-----------------------------------------------------------------------
  /// Constructor
  //-----------------------------------------------------------------------

  explicit IostreamSwig(std::string To_write = "hi there", int Word_to_read = 2)
    : to_write(std::move(To_write)), word_to_read(Word_to_read)
  {
  }
  void write(std::ostream& Os) const;
  std::string read(std::istream& In);
  virtual void print(std::ostream& Os) const;
private:
  std::string to_write;
  int word_to_read = 0;
};
}
