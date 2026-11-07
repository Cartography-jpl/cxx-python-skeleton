#include "blitz_mdspan.h"
#include <string>

// Explicit instantiation ignores access checks, so this is the standard
// way to get a pointer to a private member without changing the header
// that declares it.
namespace CXX_PYTHON_RULES_NAMESPACE {
namespace detail {
template<class T> typename BlockMember<T>::type BlockMember<T>::ptr = nullptr;

template<class T, typename BlockMember<T>::type P> struct BlockMemberSetter {
  static const bool done;
};
template<class T, typename BlockMember<T>::type P>
const bool BlockMemberSetter<T, P>::done = (BlockMember<T>::ptr = P, true);

#define CXX_PYTHON_RULES_BLOCK_MEMBER(T)                                       \
  template struct BlockMember<T>;                                              \
  template struct BlockMemberSetter<T, &blitz::MemoryBlockReference<T>::block_>;

CXX_PYTHON_RULES_BLOCK_MEMBER(double);
CXX_PYTHON_RULES_BLOCK_MEMBER(float);
CXX_PYTHON_RULES_BLOCK_MEMBER(int);
CXX_PYTHON_RULES_BLOCK_MEMBER(unsigned int);
CXX_PYTHON_RULES_BLOCK_MEMBER(short int);
CXX_PYTHON_RULES_BLOCK_MEMBER(unsigned short int);
CXX_PYTHON_RULES_BLOCK_MEMBER(char);
CXX_PYTHON_RULES_BLOCK_MEMBER(signed char);
CXX_PYTHON_RULES_BLOCK_MEMBER(unsigned char);
CXX_PYTHON_RULES_BLOCK_MEMBER(bool);
CXX_PYTHON_RULES_BLOCK_MEMBER(long);
CXX_PYTHON_RULES_BLOCK_MEMBER(std::string);
}
}
