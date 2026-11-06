// Stuff to include everywhere for using boost::array and std::array. We
// also have swig_boost_array.i which includes stuff compiled in one spot.

%include "swig_array_inc.i"

%{
#include <boost/array.hpp>
#include <algorithm>
#include <array>
#include <cstddef>

template<class T, int D> inline boost::array<T, D>
  to_boost_array(PyObject* numpy)
{
  blitz::Array<T, 1> b = to_blitz_array<T, 1>(numpy);
  if(b.rows() != D)
    throw std::runtime_error("Array not expeced size");
  boost::array<T, D> res;
  std::copy(b.begin(), b.end(), res.begin());
  return res;
}

template<class T, std::size_t D> inline std::array<T, D>
  to_std_array(PyObject* numpy)
{
  blitz::Array<T, 1> b = to_blitz_array<T, 1>(numpy);
  if(b.rows() != (int) D)
    throw std::runtime_error("Array not expeced size");
  std::array<T, D> res;
  std::copy(b.begin(), b.end(), res.begin());
  return res;
}

//-----------------------------------------------------------------------
/// A new 1-d numpy array with a copy of the values (nullptr with a python
/// error set if numpy can't make one).
//-----------------------------------------------------------------------

template<class T, std::size_t D> inline PyObject*
  std_array_to_numpy(const std::array<T, D>& A)
{
  npy_intp dims[1] = {(npy_intp) D};
  PyObject* res = PyArray_SimpleNew(1, dims, type_to_npy<T>());
  if(!res)
    return nullptr;
  std::copy(A.begin(), A.end(),
            static_cast<T*>(PyArray_DATA((PyArrayObject*) res)));
  return res;
}
%}
