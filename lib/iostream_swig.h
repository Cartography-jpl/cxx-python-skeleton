#ifndef IOSTREAM_SWIG_H
#define IOSTREAM_SWIG_H
#include "printable.h"
#include <iostream>

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

  IostreamSwig(const std::string& To_write = "hi there", int Word_to_read = 2)
    : to_write(To_write), word_to_read(Word_to_read)
  {
  }
  ~IostreamSwig() override = default;
  void write(std::ostream& Os) const;
  std::string read(std::istream& In);
  virtual void print(std::ostream& Os) const;
private:
  std::string to_write;
  int word_to_read;
};
}
#endif
