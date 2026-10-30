%include "skeleton_common.i"
%{
#include "array_typemap_sample.h"
%}

%base_import(generic_object)
%skeleton_shared_ptr(Skeleton::ArrayTypemapSample);
namespace Skeleton {
%feature("director") ArrayTypemapSample;
class ArrayTypemapSample : public GenericObject {
public:
  ArrayTypemapSample();
  virtual ~ArrayTypemapSample();
  blitz::Array<double, 1> echo_double_1(const blitz::Array<double, 1>& A) const;
  blitz::Array<double, 2> echo_double_2(const blitz::Array<double, 2>& A) const;
  blitz::Array<double, 3> echo_double_3(const blitz::Array<double, 3>& A) const;
  blitz::Array<float, 2> echo_float_2(const blitz::Array<float, 2>& A) const;
  blitz::Array<int, 1> echo_int_1(const blitz::Array<int, 1>& A) const;
  blitz::Array<int, 2> echo_int_2(const blitz::Array<int, 2>& A) const;
  blitz::Array<int, 3> echo_int_3(const blitz::Array<int, 3>& A) const;
  blitz::Array<unsigned int, 2> echo_uint_2(const blitz::Array<unsigned int, 2>& A) const;
  blitz::Array<bool, 1> echo_bool_1(const blitz::Array<bool, 1>& A) const;
  blitz::Array<bool, 2> echo_bool_2(const blitz::Array<bool, 2>& A) const;
  blitz::Array<short int, 2> echo_short_2(const blitz::Array<short int, 2>& A) const;
  blitz::Array<unsigned short int, 2> echo_ushort_2(const blitz::Array<unsigned short int, 2>& A) const;
  blitz::Array<char, 2> echo_char_2(const blitz::Array<char, 2>& A) const;
  blitz::Array<unsigned char, 2> echo_uchar_2(const blitz::Array<unsigned char, 2>& A) const;
  double sum_by_value(blitz::Array<double, 2> A) const;
  double sum_double_2(const blitz::Array<double, 2>& A) const;
  double element(const blitz::Array<double, 2>& A, int I, int J) const;
  void set_value(const blitz::Array<double, 2>& A, double V) const;
  void set_value_copy(const blitz::Array<double, 2>& FORCE_COPY, double V) const;
  void split(const blitz::Array<double, 2>& A, blitz::Array<double, 1>& OUTPUT1,
	     blitz::Array<double, 1>& OUTPUT2) const;
  void scaled(const blitz::Array<double, 2>& A, double S,
	      blitz::Array<double, 2>& OUTPUT, double& OUTPUT) const;
  std::vector<blitz::Array<double, 1> >
  echo_vector(const std::vector<blitz::Array<double, 1> >& V) const;
  int vector_size(std::vector<blitz::Array<int, 2> > V) const;
  %python_attribute(stored, blitz::Array<double, 2>);
  blitz::Array<double, 2>& stored_ref();
  double stored_value(int I, int J) const;
  virtual blitz::Array<double, 1> transform(const blitz::Array<double, 1>& A) const;
  blitz::Array<double, 1> apply_transform(const blitz::Array<double, 1>& A) const;
  std::string print_to_string() const;
};
}
// List of things "import *" will include
%python_export("ArrayTypemapSample")
