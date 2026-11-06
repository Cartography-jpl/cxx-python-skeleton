#ifndef ARRAY_TYPEMAP_SAMPLE_H
#define ARRAY_TYPEMAP_SAMPLE_H
#include "printable.h"
#include <blitz/array.h>
#include <vector>

namespace Skeleton {
/****************************************************************//**
  Simple class to test the blitz::Array <-> numpy typemaps in
  swig_rules/include/swig_array.i (see array_typemap_sample_test.py).

  The "echo" functions return what they were given, so python can
  check the values, shape and element type survive the round
  trip. sum_*, shares_memory and set_value let the test check
  whether the C++ side sees the numpy data directly or a copy.
*******************************************************************/
class ArrayTypemapSample : public Printable<ArrayTypemapSample> {
public:
  ArrayTypemapSample() : stored_(2, 3)
  { stored_ = 1, 2, 3, 4, 5, 6; }
  ~ArrayTypemapSample() override = default;

//-----------------------------------------------------------------------
/// Echo back an array, one function for each element type and a few
/// ranks.
//-----------------------------------------------------------------------

  blitz::Array<double, 1> echo_double_1(const blitz::Array<double, 1>& A) const
  { return A.copy(); }
  blitz::Array<double, 2> echo_double_2(const blitz::Array<double, 2>& A) const
  { return A.copy(); }
  blitz::Array<double, 3> echo_double_3(const blitz::Array<double, 3>& A) const
  { return A.copy(); }
  blitz::Array<float, 2> echo_float_2(const blitz::Array<float, 2>& A) const
  { return A.copy(); }
  blitz::Array<int, 1> echo_int_1(const blitz::Array<int, 1>& A) const
  { return A.copy(); }
  blitz::Array<int, 2> echo_int_2(const blitz::Array<int, 2>& A) const
  { return A.copy(); }
  blitz::Array<int, 3> echo_int_3(const blitz::Array<int, 3>& A) const
  { return A.copy(); }
  blitz::Array<unsigned int, 2> echo_uint_2(const blitz::Array<unsigned int, 2>& A) const
  { return A.copy(); }
  blitz::Array<bool, 1> echo_bool_1(const blitz::Array<bool, 1>& A) const
  { return A.copy(); }
  blitz::Array<bool, 2> echo_bool_2(const blitz::Array<bool, 2>& A) const
  { return A.copy(); }
  blitz::Array<short int, 2> echo_short_2(const blitz::Array<short int, 2>& A) const
  { return A.copy(); }
  blitz::Array<unsigned short int, 2> echo_ushort_2(const blitz::Array<unsigned short int, 2>& A) const
  { return A.copy(); }
  blitz::Array<char, 2> echo_char_2(const blitz::Array<char, 2>& A) const
  { return A.copy(); }
  blitz::Array<unsigned char, 2> echo_uchar_2(const blitz::Array<unsigned char, 2>& A) const
  { return A.copy(); }

//-----------------------------------------------------------------------
/// Pass by value (a different typemap than const reference).
//-----------------------------------------------------------------------

  double sum_by_value(blitz::Array<double, 2> A) const
  { return blitz::sum(A); }

//-----------------------------------------------------------------------
/// Sum of an array passed by const reference. Used with
/// non-contiguous and Fortran ordered input.
//-----------------------------------------------------------------------

  double sum_double_2(const blitz::Array<double, 2>& A) const
  { return blitz::sum(A); }

//-----------------------------------------------------------------------
/// Return element (I, J), to check indexing of strided input.
//-----------------------------------------------------------------------

  double element(const blitz::Array<double, 2>& A, int I, int J) const
  { return A(I, J); }

//-----------------------------------------------------------------------
/// Write V into A(0,0). Shows whether C++ sees the python data
/// directly (no copy) or a copy (FORCE_COPY).
//-----------------------------------------------------------------------

  void set_value(const blitz::Array<double, 2>& A, double V) const
  { const_cast<blitz::Array<double, 2>&>(A)(0, 0) = V; }
  void set_value_copy(const blitz::Array<double, 2>& A, double V) const
  { const_cast<blitz::Array<double, 2>&>(A)(0, 0) = V; }

//-----------------------------------------------------------------------
/// Multiple array returns, through OUTPUT/OUTPUT1/OUTPUT2 arguments.
//-----------------------------------------------------------------------

  void split(const blitz::Array<double, 2>& A, blitz::Array<double, 1>& Row0,
             blitz::Array<double, 1>& Col0) const
  {
    Row0.resize(A.cols());
    Row0 = A(0, blitz::Range::all());
    Col0.resize(A.rows());
    Col0 = A(blitz::Range::all(), 0);
  }
  void scaled(const blitz::Array<double, 2>& A, double S,
              blitz::Array<double, 2>& Res, double& Total) const
  {
    Res.resize(A.shape());
    Res = A * S;
    Total = blitz::sum(Res);
  }

//-----------------------------------------------------------------------
/// std::vector of arrays, in and out.
//-----------------------------------------------------------------------

  std::vector<blitz::Array<double, 1> >
  echo_vector(const std::vector<blitz::Array<double, 1> >& V) const
  {
    std::vector<blitz::Array<double, 1> > res;
    for(const auto& a : V)
      res.push_back(a.copy());
    return res;
  }
  int vector_size(std::vector<blitz::Array<int, 2> > V) const
  { return (int) V.size(); }

//-----------------------------------------------------------------------
/// A stored array returned by const reference (shares memory with
/// this object, the returned numpy keeps it alive) and by non-const
/// reference (writable, changes the stored array).
//-----------------------------------------------------------------------

  const blitz::Array<double, 2>& stored() const { return stored_; }
  blitz::Array<double, 2>& stored_ref() { return stored_; }
  double stored_value(int I, int J) const { return stored_(I, J); }

//-----------------------------------------------------------------------
/// Virtual function a python subclass can override, taking and
/// returning a blitz::Array (the directorin/directorout typemaps).
//-----------------------------------------------------------------------

  virtual blitz::Array<double, 1> transform(const blitz::Array<double, 1>& A) const
  {
    blitz::Array<double, 1> res(A.shape());
    res = A * 2;
    return res;
  }
  blitz::Array<double, 1> apply_transform(const blitz::Array<double, 1>& A) const
  { return transform(A); }

  virtual void print(std::ostream& Os) const { Os << "ArrayTypemapSample"; }
private:
  blitz::Array<double, 2> stored_;
};
}
#endif
