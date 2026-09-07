%include "skeleton_common.i"
%{
#include "iostream_swig.h"
%}

%base_import(generic_object)
%skeleton_shared_ptr(Skeleton::IostreamSwig);
namespace Skeleton {
class IostreamSwig : public GenericObject {
public:
  IostreamSwig(const std::string& To_write = "hi there",
	       int Word_to_read = 2);
  void write(std::ostream& Os) const;
  std::string read(std::istream& In);
  std::string print_to_string() const;
};
}
// List of things "import *" will include
%python_export("IostreamSwig")

  
