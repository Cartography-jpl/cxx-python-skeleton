#ifndef SWIG_RULES_SAMPLE_H
#define SWIG_RULES_SAMPLE_H
#include "printable.h"
#include <optional>
#include <boost/rational.hpp>
#include <array>
#include <blitz/array.h>
#include <vector>

namespace Skeleton {
/****************************************************************/ /**
  Small value type, wrapped with %shared_ptr but also stored by value
  in a std::vector (like GeoCal::Time in std::vector<Time>). Used to
  test %shared_ptr_value_traits and %pickle_vector.
*******************************************************************/
class ValueSample : public Printable<ValueSample> {
public:
  ValueSample(double V = 0) : v_(V) {}
  ~ValueSample() override = default;
  double v() const { return v_; }
  virtual void print(std::ostream& Os) const
  {
    Os << "ValueSample(" << v_ << ")";
  }
private:
  double v_;
};

/****************************************************************/ /**
  Simple class to test the smaller rules in swig_rules/include
  (see swig_rules_sample_test.py): std::optional, boost::rational,
  std::array, the %python_attribute variants and the pickle
  macros.
*******************************************************************/
class SwigRulesSample : public Printable<SwigRulesSample> {
public:
  SwigRulesSample(int Count = 0, const std::string& Name = "")
    : count_(Count), name_(Name), scale_(1.0)
  {
    position_ = {{1.0, 2.0, 3.0}};
    data_.resize(2, 2);
    data_ = 0;
  }
  ~SwigRulesSample() override = default;

  //-----------------------------------------------------------------------
  /// Constructor arguments, used by %pickle_init.
  //-----------------------------------------------------------------------

  int count() const { return count_; }
  const std::string& name() const { return name_; }

  //-----------------------------------------------------------------------
  /// std::optional, in and out, by value and by const reference
  //-----------------------------------------------------------------------

  const std::optional<int>& optional_int() const { return oint_; }
  void optional_int(const std::optional<int>& V) { oint_ = V; }
  std::optional<double> optional_double() const { return odouble_; }
  void optional_double(std::optional<double> V) { odouble_ = V; }
  const std::optional<double>& optional_double_ref() const { return odouble_; }
  const std::optional<blitz::Range>& optional_range_ref() const
  {
    return orange_;
  }
  void optional_range(const std::optional<blitz::Range>& R) { orange_ = R; }
  int optional_range_size(const std::optional<blitz::Range>& R) const
  {
    return R ? R->length(0) : -1;
  }

  //-----------------------------------------------------------------------
  /// boost::rational
  //-----------------------------------------------------------------------

  double rational_value(const boost::rational<int>& R) const
  {
    return boost::rational_cast<double>(R);
  }
  boost::rational<int> half() const { return boost::rational<int>(1, 2); }

  //-----------------------------------------------------------------------
  /// std::array, as a member (%python_attribute_std_array) and as an
  /// argument.
  //-----------------------------------------------------------------------

  std::array<double, 3> position_;
  double sum_array(const std::array<double, 3>& A) const
  {
    return A[0] + A[1] + A[2];
  }
  double first_element(std::array<double, 3> A) const { return A[0]; }

  //-----------------------------------------------------------------------
  /// Accessors under a different name than the python attribute
  /// (%python_attribute2), a non-const accessor
  /// (%python_attribute_nonconst), and a virtual one a python
  /// subclass can override (%python_attribute_with_set_virtual).
  //-----------------------------------------------------------------------

  int get_count_squared() const { return count_ * count_; }
  double get_scale() const { return scale_; }
  void get_scale(const double& V) { scale_ = V; }
  int counter() { return count_; }
  blitz::Array<double, 2>& data_ptr() { return data_; }
  double data_value(int I, int J) const { return data_(I, J); }
  virtual double weight() const { return 2.0; }
  virtual void weight(const double& V) { weight_set_ = V; }
  double weight_set() const { return weight_set_; }
  double weight_from_cpp() const { return weight(); }

  //-----------------------------------------------------------------------
  /// std::vector of a by-value type that is wrapped with %shared_ptr
  //-----------------------------------------------------------------------

  std::vector<ValueSample> values() const
  {
    return std::vector<ValueSample>{ValueSample(1), ValueSample(2)};
  }
  double sum_values(const std::vector<ValueSample>& V) const
  {
    double res = 0;
    for(const auto& v : V)
      res += v.v();
    return res;
  }
  std::vector<int> int_vector() const { return std::vector<int>{1, 2, 3}; }
  int sum_int_vector(const std::vector<int>& V) const
  {
    int res = 0;
    for(int i : V)
      res += i;
    return res;
  }

  virtual void print(std::ostream& Os) const
  {
    Os << "SwigRulesSample(" << count_ << ", " << name_ << ")";
  }
private:
  int count_;
  std::string name_;
  double scale_;
  double weight_set_ = 0;
  std::optional<int> oint_;
  std::optional<double> odouble_;
  std::optional<blitz::Range> orange_;
  blitz::Array<double, 2> data_;
};
}
#endif
