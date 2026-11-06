%include "skeleton_common.i"

%{
#include "overload_example.h"
%}
%base_import(generic_object)
%import "numpy_sample.i"

%skeleton_shared_ptr(Skeleton::OverloadExample);

namespace Skeleton {
class OverloadExample : public GenericObject {
public:
  OverloadExample();
  static std::string which(const blitz::Array<double, 2>& A);
  static std::string which(const boost::shared_ptr<NumpySample>& S);
  static std::string which(const std::string& S);
  std::string print_to_string() const;
};
}

// List of things "import *" will include
%python_export("OverloadExample")
