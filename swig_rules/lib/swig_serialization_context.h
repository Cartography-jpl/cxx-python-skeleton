#ifndef SWIG_SERIALIZATION_CONTEXT_H
#define SWIG_SERIALIZATION_CONTEXT_H
#include "generic_object.h"
#include <boost/shared_ptr.hpp>
#include <mutex>
#include <set>

namespace SWIG_MAPPER_NAMESPACE {

/****************************************************************//**
  While boost serialization handles object tracking, as does the
  python Pickler class, this doesn't happen correctly when we
  bounce back and forth between the two (e.g., a C++ object that
  contains a python object that contains a C++ object). We
  have a context class to contain a single instance of the
  Pickler object and the boost::archive used for serilization.

  There is at most one of these objects. We create one, do either
  the python pickle or boost serialization, and then destroy the
  object.

  This class also holds the pointer-tracking state used to support
  serializing boost::weak_ptr (see the discussion in
  weak_ptr_serialize_support.h). A serialize_write* call is really a
  two-pass sequence -- a mark pass that populates this state,
  followed by the real archive write that reads it back -- so both
  the singleton and the mark state need to be protected by, and live
  for, the same critical section. That is what SerializeSessionLock
  is for.
*******************************************************************/

class SwigSerializationContext {
private:
  // A private struct that nobody else can construct
  struct ConstructorKey {
    explicit ConstructorKey() = default;
  };
public:
  SwigSerializationContext(ConstructorKey) {}
  virtual ~SwigSerializationContext() {}

  static void clear_ptr_serialized_reference();
  static void add_ptr_serialized_reference(const GenericObject* P);
  static bool is_ptr_serialized(const GenericObject* P);

private:
  static void create_singleton();
  static void destroy_singleton();
  friend class SerializeSessionLock;

  static boost::shared_ptr<SwigSerializationContext> instance;

  /// Guards both the singleton pointer above and the pointer-mark
  /// state below. Recursive because a serialize_write* call can
  /// recurse on the same thread (e.g. a SWIG director object being
  /// pickled can trigger another serialize_write_binary call on a
  /// framework object it holds before the outer call finishes).
  static std::recursive_mutex mutex_;
  static std::set<const GenericObject*> ptr_referenced;
  static thread_local int depth;
};

/****************************************************************//**
  RAII guard for one serialization/deserialization session. Locks
  SwigSerializationContext's mutex for the whole create+mark+write
  (or read) sequence -- not just around the singleton swap -- so that
  sequence is one atomic critical section wrt other threads. Same-
  thread re-entry still works because the mutex is recursive; only
  the outermost instance on a thread creates/destroys the singleton
  and clears the pointer-mark state.

  Pass Create_context = false for callers that only need the
  pointer-mark state guarded (e.g. pickling, where the singleton
  context is managed by an outer call already).
*******************************************************************/

class SerializeSessionLock {
public:
  explicit SerializeSessionLock(bool Create_context = true);
  ~SerializeSessionLock();
  SerializeSessionLock(const SerializeSessionLock&) = delete;
  SerializeSessionLock& operator=(const SerializeSessionLock&) = delete;
private:
  bool create_context;
};
}
#endif
