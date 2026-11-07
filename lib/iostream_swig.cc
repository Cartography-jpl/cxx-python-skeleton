#include "iostream_swig.h"
#include <istream>
#include <ostream>

using namespace Skeleton;

//-----------------------------------------------------------------------
/// Write our data to the given ostream.
//-----------------------------------------------------------------------

void IostreamSwig::write(std::ostream& Os) const
{
  Os << to_write;
}

//-----------------------------------------------------------------------
/// Read data from the given istream.
//-----------------------------------------------------------------------

std::string IostreamSwig::read(std::istream& In)
{
  std::string res;
  for(int i = 0; i < word_to_read; ++i) {
    std::string t;
    In >> t;
    if(i != 0)
      res += " ";
    res += t;
  }
  return res;
}

//-----------------------------------------------------------------------
/// Print description of class.
//-----------------------------------------------------------------------

void IostreamSwig::print(std::ostream& Os) const
{
  Os << "IostreamSwigTest";
}
