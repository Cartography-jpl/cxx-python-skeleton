#ifndef SKELETON_NULL_DELETER_H
#define SKELETON_NULL_DELETER_H
#include <memory>

namespace Skeleton {
/****************************************************************/ /**
  Deleter that does nothing, for a shared_ptr to an object that is
  owned by something else (a stack or member object). Copied from
  geocal's and framework's null_deleter.h.

  The pointers below are made with the aliasing constructor from a
  shared_ptr<void>, not as shared_ptr<T>(p, null_deleter()). A
  shared_ptr<T> made from a raw pointer sets the weak_this of a class
  with enable_shared_from_this, so shared_from_this() would then return
  this non-owning pointer instead of the real owner (a dangling pointer
  risk). A shared_ptr<void> leaves weak_this alone. Observer::this_obj
  is made by to_ptr, so this covers it too. See
  claude_notes/implementation_plan.md §2.1 and §2.4.
*******************************************************************/
struct null_deleter {
  void operator()(void const*) const {}
};

//-----------------------------------------------------------------------
/// Non-owning shared_ptr to a const object. The caller has to make sure
/// T lives longer than every copy of the pointer.
//-----------------------------------------------------------------------

template<class T> std::shared_ptr<T> const_pointer(const T& t)
{
  T* p = const_cast<T*>(&t);
  return std::shared_ptr<T>(
      std::shared_ptr<void>(static_cast<void*>(p), null_deleter()), p);
}

//-----------------------------------------------------------------------
/// Non-owning shared_ptr to an object. The caller has to make sure T
/// lives longer than every copy of the pointer.
//-----------------------------------------------------------------------

template<class T> std::shared_ptr<T> to_ptr(T& t)
{
  return std::shared_ptr<T>(
      std::shared_ptr<void>(static_cast<void*>(&t), null_deleter()), &t);
}
}
#endif
