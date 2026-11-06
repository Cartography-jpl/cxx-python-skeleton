%include "skeleton_common.i"
%{
#include "array_sample.h"
%}

%base_import(generic_object)
%skeleton_shared_ptr(Skeleton::ArraySample);
namespace Skeleton {
class ArraySample : public GenericObject {
public:
  ArraySample();
  %python_attribute_boost_array(position_, double, 3);
  %python_attribute_with_set(matrix, blitz::Array<double, 2>);
  %python_attribute_with_set(count, boost::optional<int>);
  %python_attribute_with_set(scale, boost::optional<double>);
  std::string print_to_string() const;
  %pickle_serialization();
};
}

// List of things "import *" will include
%python_export("ArraySample")
