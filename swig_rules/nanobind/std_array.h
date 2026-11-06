#ifndef SWIG_RULES_NB_STD_ARRAY_H
#define SWIG_RULES_NB_STD_ARRAY_H
// std::array<T, N> of numbers <-> numpy, as the SWIG std::array typemaps
// did (swig_boost_array.i): in, anything numpy converts to a 1-D array of
// length N (list, tuple, any dtype or strides; always a copy); out, a new
// numpy array (a copy). Use this instead of nanobind/stl/array.h, which
// gives a python list (never include both).
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include "blitz_array.h"
#include <array>
#include <stdexcept>
#include <string>

namespace SWIG_MAPPER_NAMESPACE {
namespace nbblitz {
//-----------------------------------------------------------------------
/// Python object -> std::array, throwing std::runtime_error (python
/// RuntimeError) if it isn't N numbers. For property setters, where
/// SWIG gave a RuntimeError for the wrong size.
//-----------------------------------------------------------------------

template<class T, std::size_t N> std::array<T, N> to_std_array(nb::handle src)
{
  nb::object a =
      numpy().attr("asarray")(src, nb::arg("dtype") = numpy_dtype_name<T>());
  nb::detail::make_caster<nb::ndarray<const T, nb::ndim<1>, nb::device::cpu>> c;
  if(!c.from_python(a, 0, nullptr) || c.value.shape(0) != N)
    throw std::runtime_error("Expected an array of length " +
                             std::to_string(N));
  std::array<T, N> res;
  for(std::size_t i = 0; i < N; ++i)
    res[i] = c.value.data()[i * c.value.stride(0)];
  return res;
}
}
}

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template<class T, std::size_t N> struct type_caster<std::array<T, N>> {
  static_assert(std::is_arithmetic_v<T>,
                "swig_rules std_array.h handles arrays of numbers only");
  using StdArray = std::array<T, N>;
  NB_TYPE_CASTER(StdArray,
                 (io_name("numpy.typing.ArrayLike", "numpy.typing.NDArray")))

  bool from_python(handle src, uint32_t, cleanup_list*) noexcept
  {
    try {
      value = SWIG_MAPPER_NAMESPACE::nbblitz::to_std_array<T, N>(src);
      return true;
    } catch(...) {
      PyErr_Clear();
      return false;
    }
  }

  static handle from_cpp(const Value& v, rv_policy, cleanup_list*) noexcept
  {
    try {
      T* d = new T[N];
      std::copy(v.begin(), v.end(), d);
      capsule owner(d, [](void* p) noexcept { delete[] static_cast<T*>(p); });
      size_t shape[1] = {N};
      return ndarray<numpy, T>(d, 1, shape, owner).cast().release();
    } catch(python_error& e) {
      e.restore();
    }
    return handle();
  }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
#endif
