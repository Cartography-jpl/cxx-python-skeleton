%include "skeleton_common.i"

%{
#include "numpy_sample.h"
%}

%base_import(generic_object)

%skeleton_shared_ptr(Skeleton::NumpySample);

namespace Skeleton {
class NumpySample : public GenericObject {
public:
  NumpySample();
  NumpySample(const blitz::Array<double, 2>& Din);
  %python_attribute(data, blitz::Array<double, 2>);
  std::optional<blitz::Range> maybe_range(bool Return_empty) const;
};
}

// List of things "import *" will include
%python_export("NumpySample")
