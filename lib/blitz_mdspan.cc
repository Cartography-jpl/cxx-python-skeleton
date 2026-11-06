#include "blitz_mdspan.h"
#include <string>

// Explicit instantiation ignores access checks, so this is the standard
// way to get a pointer to a private member without changing the header
// that declares it.
namespace Skeleton {
namespace detail {
template<class T> typename BlockMember<T>::type BlockMember<T>::ptr = nullptr;

template<class T, typename BlockMember<T>::type P> struct BlockMemberSetter {
  static const bool done;
};
template<class T, typename BlockMember<T>::type P>
const bool BlockMemberSetter<T, P>::done = (BlockMember<T>::ptr = P, true);

#define SKELETON_BLOCK_MEMBER(T)                                               \
  template struct BlockMember<T>;                                              \
  template struct BlockMemberSetter<T, &blitz::MemoryBlockReference<T>::block_>;

SKELETON_BLOCK_MEMBER(double);
SKELETON_BLOCK_MEMBER(float);
SKELETON_BLOCK_MEMBER(int);
SKELETON_BLOCK_MEMBER(unsigned int);
SKELETON_BLOCK_MEMBER(short int);
SKELETON_BLOCK_MEMBER(unsigned short int);
SKELETON_BLOCK_MEMBER(char);
SKELETON_BLOCK_MEMBER(signed char);
SKELETON_BLOCK_MEMBER(unsigned char);
SKELETON_BLOCK_MEMBER(bool);
SKELETON_BLOCK_MEMBER(long);
SKELETON_BLOCK_MEMBER(std::string);
}
}
