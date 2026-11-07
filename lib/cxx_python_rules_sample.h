#pragma once
#include "printable.h"
#include "skeleton_exception.h"
#include <optional>
#include <boost/rational.hpp>
#include <array>
#include <blitz/array.h>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>

namespace Skeleton {
/****************************************************************/ /**
  Small value type, bound with std::shared_ptr but also stored by value
  in a std::vector (like GeoCal::Time in std::vector<Time>). Used to
  test a bound std::vector of values and its pickling
  (cxx_python_rules_nb::pickle_vector).
*******************************************************************/
class ValueSample : public Printable<ValueSample> {
public:
  explicit ValueSample(double V = 0) : v_(V) {}
  double v() const { return v_; }
  virtual void print(std::ostream& Os) const
  {
    Os << "ValueSample(" << v_ << ")";
  }
private:
  double v_ = 0;
};

/****************************************************************/ /**
  Simple class to test the smaller rules (cxx_python_rules/nanobind,
  the SWIG swig_rules/include before phase S6; see
  cxx_python_rules_sample_test.py): std::optional, boost::rational,
  std::array, properties and the pickle helpers.
*******************************************************************/
class CxxPythonRulesSample : public Printable<CxxPythonRulesSample> {
public:
  explicit CxxPythonRulesSample(int Count = 0, std::string Name = "")
    : count_(Count), name_(std::move(Name))
  {
    data_.resize(2, 2);
    data_ = 0;
  }

  //-----------------------------------------------------------------------
  /// Constructor arguments, used by cxx_python_rules_nb::pickle_init.
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
  /// std::array, as a member (a numpy array property) and as an
  /// argument.
  //-----------------------------------------------------------------------

  std::array<double, 3> position_{1.0, 2.0, 3.0};
  double sum_array(const std::array<double, 3>& A) const
  {
    return A[0] + A[1] + A[2];
  }
  double first_element(std::array<double, 3> A) const { return A[0]; }

  //-----------------------------------------------------------------------
  /// Accessors under a different name than the python property, a
  /// non-const accessor, and a virtual one a python subclass can
  /// override.
  //-----------------------------------------------------------------------

  int get_count_squared() const { return count_ * count_; }
  double get_scale() const { return scale_; }
  void get_scale(const double& V) { scale_ = V; }
  int counter() { return count_; }
  blitz::Array<double, 2>& data_ptr() { return data_; }
  double data_value(int I, int J) const
  {
    range_check(I, 0, data_.rows());
    range_check(J, 0, data_.cols());
    return data_(I, J);
  }
  virtual double weight() const { return 2.0; }
  virtual void weight(const double& V) { weight_set_ = V; }
  double weight_set() const { return weight_set_; }
  double weight_from_cpp() const { return weight(); }

  //-----------------------------------------------------------------------
  /// std::vector of a by-value type that is bound with std::shared_ptr
  //-----------------------------------------------------------------------

  std::vector<ValueSample> values() const
  {
    return std::vector<ValueSample>{ValueSample(1), ValueSample(2)};
  }
  double sum_values(const std::vector<ValueSample>& V) const
  {
    return std::ranges::fold_left(
        V, 0.0, [](double S, const ValueSample& X) { return S + X.v(); });
  }
  std::vector<int> int_vector() const { return std::vector<int>{1, 2, 3}; }
  int sum_int_vector(const std::vector<int>& V) const
  {
    return std::ranges::fold_left(V, 0, std::plus<>());
  }

  virtual void print(std::ostream& Os) const
  {
    Os << "CxxPythonRulesSample(" << count_ << ", " << name_ << ")";
  }
private:
  int count_ = 0;
  std::string name_;
  double scale_ = 1.0;
  double weight_set_ = 0;
  std::optional<int> oint_;
  std::optional<double> odouble_;
  std::optional<blitz::Range> orange_;
  blitz::Array<double, 2> data_;
};
}
