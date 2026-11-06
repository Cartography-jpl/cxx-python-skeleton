%include "skeleton_common.i"
%{
#include "array_sample.h"
%}

%base_import(generic_object)
%skeleton_shared_ptr(Skeleton::ArraySample);
%skeleton_shared_ptr(Skeleton::ColumnMajorSample);
namespace Skeleton {
class ArraySample : public GenericObject {
public:
  ArraySample();
  %python_attribute_std_array(position_, double, 3);
  std::array<double, 3> position() const;
  const std::array<double, 3>& position_ref() const;
  %python_attribute_with_set(matrix, blitz::Array<double, 2>);
  %python_attribute_with_set(count, std::optional<int>);
  %python_attribute_with_set(scale, std::optional<double>);
  std::string print_to_string() const;
  %pickle_serialization();
};

class ColumnMajorSample : public GenericObject {
public:
  ColumnMajorSample(int Rows = 2, int Cols = 3);
  %python_attribute_with_set(data, blitz::Array<double, 2>);
  blitz::Array<double, 2> strided_view() const;
  static std::string layout(const blitz::Array<double, 2>& A);
  static std::size_t data_address(const blitz::Array<double, 2>& A);
  static double sum(const blitz::Array<double, 2>& A);
  static void scale_in_place(blitz::Array<double, 2>& A, double V);
  std::string print_to_string() const;
};
}

// List of things "import *" will include
%python_export("ArraySample", "ColumnMajorSample")
