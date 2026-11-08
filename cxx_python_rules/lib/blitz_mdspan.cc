#include "blitz_mdspan.h"
#include <string>

// Explicit instantiation ignores access checks, so this is the standard
// way to get a pointer to a private member without changing the header
// that declares it.
namespace CXX_PYTHON_RULES_NAMESPACE::detail {
template<class T> BlockMember<T>::type BlockMember<T>::ptr = nullptr;

template<class T, BlockMember<T>::type P> struct BlockMemberSetter {
  static const bool done;
};
template<class T, BlockMember<T>::type P>
const bool BlockMemberSetter<T, P>::done = (BlockMember<T>::ptr = P, true);

// For each type in CXX_PYTHON_RULES_BLITZ_OWNER_TYPES (blitz_mdspan.h).
// BlockMember<T>::ptr is set when BlockMemberSetter<...>::done is
// initialized, at static initialization of this file. The order of static
// initializers in different files isn't defined, so to_blitz called from
// another file's static initializer could find ptr still null; it checks,
// and throws. (A compile-time form, a friend function defined by the
// explicit instantiation, doesn't fit: to_blitz is a header template used
// from every file, and an explicit instantiation definition may appear
// only once in a program.)
#define CXX_PYTHON_RULES_BLOCK_MEMBER(T)                                       \
  template struct BlockMember<T>;                                              \
  template struct BlockMemberSetter<T, &blitz::MemoryBlockReference<T>::block_>;
CXX_PYTHON_RULES_BLITZ_OWNER_TYPES(CXX_PYTHON_RULES_BLOCK_MEMBER)
}
