#pragma once
// numpy <-> blitz::Array<T, N> for nanobind, through an mdspan view
// (blitz_mdspan.h has the mdspan <-> Blitz half, which doesn't depend on
// python). The behavior is that of the SWIG typemaps it replaced:
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
// A non-const reference parameter (blitz::Array<T, N>&), which C++ may
// write to, only takes a view of a writable array of the right type: a
// read-only array, or one that had to be copied (a list, another dtype,
// negative strides), raises TypeError instead, because the caller would
// not see what C++ writes.
//
// To python (return value):
//  * a numpy view of the C++ memory, which keeps the blitz data alive.
//    Read only, except for a non-const reference return (T&), which is
//    writable (python writes are seen by C++).
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include "blitz_mdspan.h"
#include <blitz/array.h>
#include <array>
#include <memory>
#include <type_traits>
#include <utility>

namespace CXX_PYTHON_RULES_NAMESPACE::nbblitz {
namespace nb = nanobind;

template<class T> constexpr const char* numpy_dtype_name()
{
  static_assert(std::is_arithmetic_v<T>,
                "numpy_dtype_name: only numbers and bool are supported");
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
using in_array = nb::ndarray<const T, nb::ndim<static_cast<std::size_t>(N)>,
                             nb::device::cpu>;
template<class T, int N>
using writable_array =
    nb::ndarray<T, nb::ndim<static_cast<std::size_t>(N)>, nb::device::cpu>;

// src as a blitz array sharing its memory, if src is already an array of
// the right type and rank with non-negative strides (and, if Writable,
// not read-only).
template<class T, int N, bool Writable>
bool view_of(nb::handle src, blitz::Array<T, N>& res)
{
  using A = std::conditional_t<Writable, writable_array<T, N>, in_array<T, N>>;
  nb::detail::make_caster<A> c;
  if(!c.from_python(src, 0, nullptr))
    return false;
  A& a = c.value;
  constexpr auto rank = static_cast<std::size_t>(N);
  std::array<std::ptrdiff_t, rank> ext{}, str{};
  for(std::size_t i = 0; i < rank; ++i) {
    ext[i] = static_cast<std::ptrdiff_t>(a.shape(i));
    str[i] = static_cast<std::ptrdiff_t>(a.stride(i));
    if(str[i] < 0)
      return false;
  }
  using M = CXX_PYTHON_RULES_NAMESPACE::strided_mdspan<T, N>;
  M m(const_cast<T*>(a.data()),
      typename M::mapping_type(
          CXX_PYTHON_RULES_NAMESPACE::md::dextents<std::ptrdiff_t, rank>(ext),
          str));
  res.reference(CXX_PYTHON_RULES_NAMESPACE::to_blitz(m, python_owner(src)));
  return true;
}

template<class T, int N>
nb::handle to_numpy(const blitz::Array<T, N>& A, bool Writable)
{
  // The capsule holds a blitz array sharing A's memory (blitz reference
  // counts it), so the numpy array keeps the data alive.
  // Owned by the unique_ptr until the capsule has it, so nothing leaks
  // if making the capsule throws.
  auto keep = std::make_unique<blitz::Array<T, N>>(A);
  nb::capsule owner(keep.get(), [](void* p) noexcept {
    delete static_cast<blitz::Array<T, N>*>(p);
  });
  blitz::Array<T, N>* kept = keep.release();
  constexpr auto rank = static_cast<std::size_t>(N);
  std::array<size_t, rank> shape{};
  std::array<int64_t, rank> stride{};
  for(std::size_t i = 0; i < rank; ++i) {
    shape[i] = static_cast<size_t>(A.extent(static_cast<int>(i)));
    stride[i] = static_cast<int64_t>(A.stride(static_cast<int>(i)));
  }
  T* data = const_cast<T*>(kept->data());
  if(Writable)
    return nb::ndarray<nb::numpy, T>(data, rank, shape.data(), owner,
                                     stride.data())
        .cast()
        .release();
  return nb::ndarray<nb::numpy, const T>(data, rank, shape.data(), owner,
                                         stride.data())
      .cast()
      .release();
}
}

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template<class T, int N> struct type_caster<blitz::Array<T, N>> {
  using Value = blitz::Array<T, N>;
  static constexpr auto Name =
      io_name("numpy.typing.ArrayLike", "numpy.typing.NDArray");
  // What NB_TYPE_CASTER has, except that a non-const lvalue reference
  // parameter gets Value& (checked, see writable_reference) and a const
  // one const Value&; nanobind passes the declared parameter type here.
  template<typename T_>
  using Cast = std::conditional_t<
      is_pointer_v<T_>, Value*,
      std::conditional_t<
          std::is_lvalue_reference_v<T_>,
          std::conditional_t<std::is_const_v<std::remove_reference_t<T_>>,
                             const Value&, Value&>,
          Value&&>>;
  template<typename T_> static constexpr bool can_cast() { return true; }
  template<typename T_,
           enable_if_t<std::is_same_v<std::remove_cv_t<T_>, Value>> = 0>
  static handle from_cpp(T_* p, rv_policy policy, cleanup_list* list) noexcept
  {
    if(!p)
      return none().release();
    return from_cpp(*p, policy, list);
  }
  explicit operator Value*() { return &value; }
  explicit operator const Value&() { return value; }
  explicit operator Value&() { return writable_reference(); }
  explicit operator Value&&() { return std::move(value); }
  Value value;

  bool from_python(handle src, uint32_t flags, cleanup_list*) noexcept
  {
    namespace b = CXX_PYTHON_RULES_NAMESPACE::nbblitz;
    try {
      shares_writable = false;
      if(b::view_of<T, N, true>(src, value)) {
        shares_writable = true;
        return true;
      }
      if(b::view_of<T, N, false>(src, value))
        return true;
      if(!(flags & static_cast<uint32_t>(cast_flags::convert)))
        return false;
      object a = b::numpy().attr("array")(
          src, arg("dtype") = b::numpy_dtype_name<T>(), arg("copy") = true);
      return b::view_of<T, N, false>(a, value);
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
  // True if value is a view of the python array itself, which is
  // writable.
  bool shares_writable = false;
  Value& writable_reference()
  {
    if(!shares_writable)
      throw type_error("this argument is written to by C++, so it must be a "
                       "writable numpy array of the right dtype and rank, "
                       "not read-only or converted (a copy would not show "
                       "the changes)");
    return value;
  }
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
