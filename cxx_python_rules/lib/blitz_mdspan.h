#ifndef CXX_PYTHON_RULES_BLITZ_MDSPAN_H
#define CXX_PYTHON_RULES_BLITZ_MDSPAN_H
#include "cxx_python_rules_mdspan.h"
#include <blitz/array.h>
#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>

namespace CXX_PYTHON_RULES_NAMESPACE {
/****************************************************************/ /**
  Views between blitz::Array and mdspan, sharing the same memory. This
  is the C++ half of the numpy <-> Blitz bridge used by the python
  bindings (the other half, numpy <-> mdspan, is in
  cxx_python_rules/nanobind/blitz_array.h); it doesn't depend on python.

  Strides are in elements, as in both mdspan and blitz. Only
  non-negative strides are supported (a negative stride throws
  std::runtime_error: this generic code doesn't know the project's
  exception class); the python side copies such arrays first.
*******************************************************************/

template<class T, int N>
using strided_mdspan =
    md::mdspan<T, md::dextents<std::ptrdiff_t, N>, md::layout_stride>;

//-----------------------------------------------------------------------
/// mdspan view of a blitz::Array (any storage order or strides).
//-----------------------------------------------------------------------

template<class T, int N>
strided_mdspan<T, N> to_mdspan(const blitz::Array<T, N>& A)
{
  std::array<std::ptrdiff_t, N> ext{}, str{};
  for(int i = 0; i < N; ++i) {
    ext[i] = A.extent(i);
    str[i] = A.stride(i);
    if(str[i] < 0)
      throw std::runtime_error("to_mdspan: negative strides not supported");
  }
  using map = typename strided_mdspan<T, N>::mapping_type;
  // A const blitz::Array still shares writable data (copies of it see
  // writes), and the view is as writable as A's data; use T = const U
  // for a read-only view.
  return strided_mdspan<T, N>(const_cast<T*>(A.data()),
                              map(md::dextents<std::ptrdiff_t, N>(ext), str));
}

//-----------------------------------------------------------------------
/// blitz::Array using the memory of an mdspan. Owner (may be null) is
/// kept alive as long as the blitz array or any copy of it, so the
/// memory can belong to something else (e.g. a numpy array). With no
/// owner, the caller has to keep the memory alive.
//-----------------------------------------------------------------------

namespace detail {
template<class T> class OwnerMemoryBlock : public blitz::MemoryBlock<T> {
public:
  OwnerMemoryBlock(T* Data, std::size_t Length, std::shared_ptr<void> Owner)
    : blitz::MemoryBlock<T>(Length, Data), owner(std::move(Owner))
  {
  }
  ~OwnerMemoryBlock() override
  {
    // The memory belongs to owner, don't let blitz free it.
    this->dataBlockAddress() = nullptr;
  }
private:
  std::shared_ptr<void> owner;
};

// Pointer to the private MemoryBlockReference<T>::block_ member, so a
// blitz::Array can be given an OwnerMemoryBlock (blitz has no public way
// to do that; the SWIG typemaps used a #define hack on blitz/memblock.h).
// Set in blitz_mdspan.cc with the usual explicit-instantiation access
// trick, for the element types listed there.
template<class T> struct BlockMember {
  using type = blitz::MemoryBlock<T>* blitz::MemoryBlockReference<T>::*;
  static type ptr;
};
}

template<class Mdspan>
blitz::Array<typename Mdspan::element_type, Mdspan::rank()>
to_blitz(const Mdspan& M, std::shared_ptr<void> Owner = nullptr)
{
  using T = typename Mdspan::element_type;
  constexpr int N = Mdspan::rank();
  blitz::TinyVector<int, N> shape;
  blitz::TinyVector<blitz::diffType, N> stride;
  std::size_t span = 1;
  for(int i = 0; i < N; ++i) {
    shape(i) = static_cast<int>(M.extent(i));
    stride(i) = static_cast<blitz::diffType>(M.stride(i));
    if(stride(i) < 0)
      throw std::runtime_error("to_blitz: negative strides not supported");
    if(M.extent(i) == 0)
      span = 0;
  }
  if(span != 0)
    span = M.mapping().required_span_size();
  blitz::Array<T, N> res(M.data_handle(), shape, stride,
                         blitz::neverDeleteData);
  if(Owner) {
    // res has no block (neverDeleteData), so just give it one. A new
    // MemoryBlock starts with one reference, which is res's.
    blitz::MemoryBlockReference<T>& r = res;
    r.*(detail::BlockMember<T>::ptr) = new detail::OwnerMemoryBlock<T>(
        res.dataFirst(), span, std::move(Owner));
  }
  return res;
}
}
#endif
