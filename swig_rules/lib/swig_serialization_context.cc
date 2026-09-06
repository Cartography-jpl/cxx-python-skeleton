#include "swig_serialization_context.h"
#include <boost/make_shared.hpp>

using namespace SWIG_MAPPER_NAMESPACE;

boost::shared_ptr<SwigSerializationContext> SwigSerializationContext::instance;
std::mutex SwigSerializationContext::instance_mutex;

void SwigSerializationContext::create_singleton()
{
  std::lock_guard<std::mutex> lock(instance_mutex);
  if (!instance) {
    instance = boost::make_shared<SwigSerializationContext>(ConstructorKey{});
  }
}

void SwigSerializationContext::destroy_singleton()
{
  std::lock_guard<std::mutex> lock(instance_mutex);
  instance.reset();
}

