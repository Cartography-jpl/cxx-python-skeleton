#ifndef SKELETON_NULL_DELETER_H
#define SKELETON_NULL_DELETER_H
#include <boost/shared_ptr.hpp>

namespace Skeleton {
/****************************************************************/ /**
  Deleter that does nothing, for a shared_ptr to an object that is
  owned by something else (a stack or member object). Copied from
  geocal's and framework's null_deleter.h.
*******************************************************************/
struct null_deleter {
  void operator()(void const*) const {}
};

//-----------------------------------------------------------------------
/// Non-owning shared_ptr to a const object. The caller has to make sure
/// T lives longer than every copy of the pointer.
//-----------------------------------------------------------------------

template<class T> boost::shared_ptr<T> const_pointer(const T& t)
{
  return boost::shared_ptr<T>(const_cast<T*>(&t), null_deleter());
}

//-----------------------------------------------------------------------
/// Non-owning shared_ptr to an object. The caller has to make sure T
/// lives longer than every copy of the pointer.
//-----------------------------------------------------------------------

template<class T> boost::shared_ptr<T> to_ptr(T& t)
{
  return boost::shared_ptr<T>(&t, null_deleter());
}
}
#endif
