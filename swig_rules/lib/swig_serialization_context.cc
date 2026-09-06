#include "swig_serialization_context.h"
#include <boost/make_shared.hpp>

using namespace SWIG_MAPPER_NAMESPACE;

boost::shared_ptr<SwigSerializationContext> SwigSerializationContext::instance;
std::recursive_mutex SwigSerializationContext::mutex_;
std::set<const GenericObject*> SwigSerializationContext::ptr_referenced;
thread_local int SwigSerializationContext::depth = 0;

void SwigSerializationContext::create_singleton()
{
  if (!instance) {
    instance = boost::make_shared<SwigSerializationContext>(ConstructorKey{});
  }
}

void SwigSerializationContext::destroy_singleton()
{
  instance.reset();
}

void SwigSerializationContext::clear_ptr_serialized_reference()
{
  // Only the outermost SerializeSessionLock for this thread should
  // reset the mark state -- a nested serialize_write* call (e.g.
  // triggered while pickling a SWIG director object that itself
  // holds other framework objects) must not wipe out the still-in-
  // progress outer mark pass.
  //if (depth <= 1)
  //  ptr_referenced.clear();
  //
  // Not sure about the logic here. clear_ptr_serialized_reference is *only*
  // called by mark_pointer, which is in turn *only* called by the various serialize_write
  // functions. In C++, we don't nest the serialize_write functions. However, when we put
  // python in the mix we might serialize an C++ object that contains a python object, which
  // in turn contains another C++ object. Currently the serialization of python calls pickle,
  // and pickle of a C++ object in turn calls serialization. This seems to mean that the
  // logic above (ptr_referenced cleared only in the outermost call) is what we want. However,
  // we get null pointer calls when we use that in framework unit tests. Leave in place what
  // works, we can come back at some point and reason through this. It seems like we
  // need some better logic here. I think we somehow need to handle pickling different, passing
  // in an archive or something like that.
  ptr_referenced.clear();
}

void SwigSerializationContext::add_ptr_serialized_reference(const GenericObject* P)
{
  ptr_referenced.insert(P);
}

bool SwigSerializationContext::is_ptr_serialized(const GenericObject* P)
{
  return ptr_referenced.find(P) != ptr_referenced.end();
}

SerializeSessionLock::SerializeSessionLock(bool Create_context)
  : create_context(Create_context)
{
  SwigSerializationContext::mutex_.lock();
  ++SwigSerializationContext::depth;
  if (create_context)
    SwigSerializationContext::create_singleton();
}

SerializeSessionLock::~SerializeSessionLock()
{
  if (create_context)
    SwigSerializationContext::destroy_singleton();
  --SwigSerializationContext::depth;
  SwigSerializationContext::mutex_.unlock();
}
