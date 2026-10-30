%include "skeleton_common.i"
%{
#include "exception_sample.h"
%}

%base_import(generic_object)
%skeleton_shared_ptr(Skeleton::ExceptionSample);
namespace Skeleton {
class ExceptionSample : public GenericObject {
public:
  ExceptionSample();
  void throw_skeleton_exception(const std::string& Msg) const;
  void throw_std_exception(const std::string& Msg) const;
  void throw_unknown() const;
  int checked_value(int V) const;
  std::string print_to_string() const;
};
}
// List of things "import *" will include
%python_export("ExceptionSample")
