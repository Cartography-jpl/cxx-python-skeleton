%include "skeleton_common.i"

%{
#include "with_parameter_like.h"
%}
%base_import(generic_object)

%skeleton_shared_ptr(Skeleton::WithParameterLike);

// Abstract (the pure virtual functions aren't all declared here)
%nodefaultctor Skeleton::WithParameterLike;

namespace Skeleton {
class WithParameterLike : public GenericObject {
public:
  %python_attribute_with_set_virtual(parameter, blitz::Array<double, 1>);
  double parameter_sum() const;
};
}

// List of things "import *" will include
%python_export("WithParameterLike")
