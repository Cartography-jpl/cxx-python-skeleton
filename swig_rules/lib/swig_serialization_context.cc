#include "swig_serialization_context.h"
#include <boost/make_shared.hpp>

using namespace SWIG_MAPPER_NAMESPACE;

boost::shared_ptr<SwigSerializationContext> SwigSerializationContext::instance;

void SwigSerializationContext::create_singleton()
{
  instance = boost::make_shared<SwigSerializationContext>(ConstructorKey{});
}

void SwigSerializationContext::destroy_singleton()
{
  instance.reset();
}

