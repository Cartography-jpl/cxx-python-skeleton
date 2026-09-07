%include "skeleton_common.i"

%{
#include "skeleton_exception.h"
%}

%base_import(generic_object)

%skeleton_shared_ptr(Skeleton::Exception)

namespace Skeleton {
  class Exception : public GenericObject {
  public:
    Exception(const std::string& W);
    const char* what();
    std::string print_to_string() const;
  };
}

// List of things "import *" will include
%python_export("Exception")
