#include "swig_serialization_context.h"
#include <boost/make_shared.hpp>
#include <stdexcept>

using namespace SWIG_MAPPER_NAMESPACE;

boost::shared_ptr<SwigSerializationContext> SwigSerializationContext::instance_;
std::mutex SwigSerializationContext::mutex_;
std::set<const GenericObject*> SwigSerializationContext::ptr_referenced;

void SwigSerializationContext::create_singleton(void* pickler, void* unpickler)
{
  if (!instance_) {
    instance_ = boost::make_shared<SwigSerializationContext>(ConstructorKey{}, pickler, unpickler);
  }
}

boost::shared_ptr<SwigSerializationContext> SwigSerializationContext::instance()
{
  if (!instance_) {
    throw std::runtime_error("We don't have a SwigSerializationContext. Are use using PicklerWithSerializationSupport or UnpicklerWithSerializationSupport? This is needed for the additional support working with Directors");
  }
  return instance_;
}

void SwigSerializationContext::destroy_singleton()
{
  instance_.reset();
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

SerializeSessionLock::SerializeSessionLock(void* pickler, void* unpickler)
{
  if(SwigSerializationContext::have_instance())
    throw std::runtime_error("Can only have one instance of SerializeSessionLock at a time");
  SwigSerializationContext::mutex_.lock();
  SwigSerializationContext::create_singleton(pickler, unpickler);
}

SerializeSessionLock::~SerializeSessionLock()
{
  clear();
}
