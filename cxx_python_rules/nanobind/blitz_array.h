#ifndef CXX_PYTHON_RULES_NB_BLITZ_ARRAY_H
#define CXX_PYTHON_RULES_NB_BLITZ_ARRAY_H
// numpy <-> blitz::Array<T, N> for nanobind, through an mdspan view
// (blitz_mdspan.h has the mdspan <-> Blitz half, which doesn't depend on
// python). Replaces swig_rules/include/swig_array.i; the behavior is the
// same (claude_notes/implementation_plan.md S6.1 item 7):
//
// From python (argument, by value, const& or &):
//  * a numpy array (or anything with the buffer/DLPack protocol) of
//    element type T and rank N is used without a copy, any strides. The
//    blitz array keeps the python object alive, so C++ can keep it (a
//    member set from python stays valid after python drops the array).
//    C++ writes are seen by python.
//  * anything else (a list, another dtype, negative strides) is converted
//    with numpy.asarray (a copy), but only in nanobind's second
//    (converting) pass of overload resolution.
//  * the wrong rank, or something numpy can't convert, doesn't match
//    (TypeError if no overload does).
// To python (return value):
//  * a numpy view of the C++ memory, which keeps the blitz data alive.
//    Read only, except for a non-const reference return (T&), which is
//    writable (python writes are seen by C++).
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include "blitz_mdspan.h"
#include <blitz/array.h>
#include <memory>

namespace CXX_PYTHON_RULES_NAMESPACE {
namespace nbblitz {
namespace nb = nanobind;

template<class T> constexpr const char* numpy_dtype_name()
{
  if constexpr(std::is_same_v<T, bool>)
    return "bool";
  else if constexpr(std::is_floating_point_v<T>)
    return sizeof(T) == 4 ? "float32" : "float64";
  else if constexpr(std::is_signed_v<T>) {
    if constexpr(sizeof(T) == 1)
      return "int8";
    else if constexpr(sizeof(T) == 2)
      return "int16";
    else if constexpr(sizeof(T) == 4)
      return "int32";
    else
      return "int64";
  } else {
    if constexpr(sizeof(T) == 1)
      return "uint8";
    else if constexpr(sizeof(T) == 2)
      return "uint16";
    else if constexpr(sizeof(T) == 4)
      return "uint32";
    else
      return "uint64";
  }
}

inline nb::module_& numpy()
{
  static nb::module_* m = new nb::module_(nb::module_::import_("numpy"));
  return *m;
}

// Keeps a python object alive; may be destroyed on any thread, with or
// without the GIL, and after the interpreter is gone.
inline std::shared_ptr<void> python_owner(nb::handle h)
{
  return std::shared_ptr<void>(h.inc_ref().ptr(), [](void* p) {
    if(Py_IsInitialized() && !Py_IsFinalizing()) {
      nb::gil_scoped_acquire gil;
      Py_DECREF(static_cast<PyObject*>(p));
    }
  });
}

template<class T, int N>
using in_array = nb::ndarray<const T, nb::ndim<N>, nb::device::cpu>;

// src as a blitz array sharing its memory, if src is already an array of
// the right type and rank with non-negative strides.
template<class T, int N> bool view_of(nb::handle src, blitz::Array<T, N>& res)
{
  nb::detail::make_caster<in_array<T, N>> c;
  if(!c.from_python(src, 0, nullptr))
    return false;
  in_array<T, N>& a = c.value;
  std::array<std::ptrdiff_t, N> ext, str;
  for(int i = 0; i < N; ++i) {
    ext[i] = static_cast<std::ptrdiff_t>(a.shape(i));
    str[i] = static_cast<std::ptrdiff_t>(a.stride(i));
    if(str[i] < 0)
      return false;
  }
  using M = Skeleton::strided_mdspan<T, N>;
  M m(const_cast<T*>(a.data()),
      typename M::mapping_type(Skeleton::md::dextents<std::ptrdiff_t, N>(ext),
                               str));
  res.reference(Skeleton::to_blitz(m, python_owner(src)));
  return true;
}

template<class T, int N>
nb::handle to_numpy(const blitz::Array<T, N>& A, bool Writable)
{
  // The capsule holds a blitz array sharing A's memory (blitz reference
  // counts it), so the numpy array keeps the data alive.
  auto* keep = new blitz::Array<T, N>(A);
  nb::capsule owner(keep, [](void* p) noexcept {
    delete static_cast<blitz::Array<T, N>*>(p);
  });
  size_t shape[N];
  int64_t stride[N];
  for(int i = 0; i < N; ++i) {
    shape[i] = static_cast<size_t>(A.extent(i));
    stride[i] = static_cast<int64_t>(A.stride(i));
  }
  T* data = const_cast<T*>(keep->data());
  if(Writable)
    return nb::ndarray<nb::numpy, T>(data, N, shape, owner, stride)
        .cast()
        .release();
  return nb::ndarray<nb::numpy, const T>(data, N, shape, owner, stride)
      .cast()
      .release();
}
}
}

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template<class T, int N> struct type_caster<blitz::Array<T, N>> {
  using BlitzArray = blitz::Array<T, N>;
  NB_TYPE_CASTER(BlitzArray,
                 (io_name("numpy.typing.ArrayLike", "numpy.typing.NDArray")))

  bool from_python(handle src, uint32_t flags, cleanup_list*) noexcept
  {
    namespace b = CXX_PYTHON_RULES_NAMESPACE::nbblitz;
    try {
      if(b::view_of<T, N>(src, value))
        return true;
      if(!(flags & (uint32_t)cast_flags::convert))
        return false;
      object a = b::numpy().attr("array")(
          src, arg("dtype") = b::numpy_dtype_name<T>(), arg("copy") = true);
      return b::view_of<T, N>(a, value);
    } catch(...) {
      PyErr_Clear();
      return false;
    }
  }

  static handle from_cpp(Value& v, rv_policy, cleanup_list*) noexcept
  {
    return convert(v, true);
  }
  static handle from_cpp(const Value& v, rv_policy, cleanup_list*) noexcept
  {
    return convert(v, false);
  }
  static handle from_cpp(Value&& v, rv_policy, cleanup_list*) noexcept
  {
    return convert(v, false);
  }
private:
  static handle convert(const Value& v, bool Writable) noexcept
  {
    try {
      return CXX_PYTHON_RULES_NAMESPACE::nbblitz::to_numpy<T, N>(v, Writable);
    } catch(python_error& e) {
      e.restore();
    } catch(const std::exception& e) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
    }
    return handle();
  }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
#endif
