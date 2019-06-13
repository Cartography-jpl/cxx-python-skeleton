// -*- mode: c++; -*-
// (Not really c++, but closest emacs mode)

%include "geocal_common.i"

%{
#include <iostream>
#include <boost/iostreams/filtering_stream.hpp>
#include <string>
#include "printable.h"
namespace GeoCal {
// Class that we use to test the iostream interface  
class IostreamSwigTest : public Printable<IostreamSwigTest> {
public:
  IostreamSwigTest(const std::string& To_write = "hi there",
		   int Word_to_read = 2)
    : to_write(To_write), word_to_read(Word_to_read)
  {
  }
  virtual ~IostreamSwigTest() {}
  void write(std::ostream& Os) const
  { Os << to_write; }
  std::string read(std::istream& In)
  {
    std::string res = "";
    for(int i = 0; i < word_to_read; ++i) {
      std::string t;
      In >> t;
      if(i != 0)
	res += " ";
      res += t;
    }
    return res;
  }
  virtual void print(std::ostream& Os) const { Os << "IostreamSwigTest"; }
private:
  std::string to_write;
  int word_to_read;
};
}
%}
%base_import(generic_object)
%geocal_shared_ptr(GeoCal::IostreamSwigTest);
namespace GeoCal {
class IostreamSwigTest : public GenericObject {
public:
  IostreamSwigTest(const std::string& To_write = "hi there",
		   int Word_to_read = 2);
  void write(std::ostream& Os) const;
  std::string read(std::istream& In);
  std::string print_to_string() const;
};
}
// List of things "import *" will include
%python_export("IostreamSwigTest")

  
