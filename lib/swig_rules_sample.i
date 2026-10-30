%include "skeleton_common.i"
%{
#include "swig_rules_sample.h"
%}

%base_import(generic_object)
%skeleton_shared_ptr(Skeleton::ValueSample);
%skeleton_shared_ptr(Skeleton::SwigRulesSample);

// ValueSample is wrapped as a shared_ptr, but used by value in a
// std::vector, see swig_vector_shared_ptr.i. Needs to come before the
// %template.
%shared_ptr_value_traits(Skeleton::ValueSample)
%template(Vector_ValueSample) std::vector<Skeleton::ValueSample>;
%extend std::vector<Skeleton::ValueSample> {
  %pickle_vector();
};

namespace Skeleton {
class ValueSample : public GenericObject {
public:
  ValueSample(double V = 0);
  %python_attribute(v, double);
  std::string print_to_string() const;
  %pickle_init(1, self.v);
};

%feature("director") SwigRulesSample;
class SwigRulesSample : public GenericObject {
public:
  SwigRulesSample(int Count = 0, const std::string& Name = "");
  virtual ~SwigRulesSample();
  %python_attribute(count, int);
  %python_attribute(name, std::string);
  %python_attribute_with_set(optional_int, boost::optional<int>);
  boost::optional<double> optional_double() const;
  void optional_double(boost::optional<double> V);
  const boost::optional<double>& optional_double_ref() const;
  const boost::optional<blitz::Range>& optional_range_ref() const;
  void optional_range(const boost::optional<blitz::Range>& R);
  int optional_range_size(const boost::optional<blitz::Range>& R) const;
  double rational_value(const boost::rational<int>& R) const;
  boost::rational<int> half() const;
  %python_attribute_boost_array(position_, double, 3);
  double sum_array(const boost::array<double, 3>& A) const;
  double first_element(boost::array<double, 3> A) const;
  %python_attribute2(count_squared, get_count_squared, int);
  %python_attribute2_with_set(scale, get_scale, double);
  %python_attribute_nonconst(counter, int);
  // Used this way in geocal's CalcRaster: a writable view of a C++ array
  %python_attribute2_nonconst(data, data_ptr, blitz::Array<double, 2>&);
  double data_value(int I, int J) const;
  %python_attribute_with_set_virtual(weight, double);
  double weight_set() const;
  double weight_from_cpp() const;
  std::vector<ValueSample> values() const;
  double sum_values(const std::vector<ValueSample>& V) const;
  std::vector<int> int_vector() const;
  int sum_int_vector(const std::vector<int>& V) const;
  std::string print_to_string() const;
  %pickle_init(1, self.count, self.name);
};
}
// List of things "import *" will include
%python_export("ValueSample", "SwigRulesSample", "Vector_ValueSample")
