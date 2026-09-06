#ifndef SWIG_SERIALIZATION_CONTEXT_H
#define SWIG_SERIALIZATION_CONTEXT_H
#include <boost/shared_ptr.hpp>
#include <mutex>

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
  static void create_singleton();
  static void destroy_singleton();
private:
  static boost::shared_ptr<SwigSerializationContext> instance;
  static std::mutex instance_mutex;
};

/****************************************************************//**
  Simple C++ wrapper for creating a SwigSerializationContext on
  creation and deleting on destruction.
*******************************************************************/

class SwigSerializationContextWrapper {
public:
  SwigSerializationContextWrapper() { SwigSerializationContext::create_singleton(); }
  virtual ~SwigSerializationContextWrapper() { SwigSerializationContext::destroy_singleton(); }
};
  
}
#endif
