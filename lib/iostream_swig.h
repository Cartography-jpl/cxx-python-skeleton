#pragma once
#include "printable.h"
#include <iosfwd>
#include <string>
#include <utility>

namespace Skeleton {
/****************************************************************/ /**
  This is a simple class that we can use to test that we use SWIG
  to map between python IO object and C++ stream object.
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
