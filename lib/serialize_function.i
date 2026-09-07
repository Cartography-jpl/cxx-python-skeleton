%include "skeleton_common.i"
%{
#include "serialize_function.h"
#include "swig_serialization_context.h"  
%}
%import "generic_object.i"

namespace Skeleton {
bool have_serialize_supported();
void serialize_write(const std::string& Fname, 
		     const boost::shared_ptr<GenericObject>& Obj);
std::string serialize_write_string(const boost::shared_ptr<GenericObject>& Obj);

boost::shared_ptr<GenericObject> 
serialize_read_generic(const std::string& Fname);

boost::shared_ptr<GenericObject> 
serialize_read_generic_string(const std::string& Data);
}

%typemap(out) std::string {
  $result = PyByteArray_FromStringAndSize($1.data(), $1.size());
}

%typemap(in) const std::string& {
  if(!PyByteArray_Check($input)) {
    PyErr_Clear();
    PyErr_SetString(PyExc_TypeError,"not a bytearray");
    return NULL;
  }
  $1 = new std::string(PyByteArray_AS_STRING($input), PyByteArray_GET_SIZE($input));
}

%typemap(freearg) const std::string& {
  delete $1;
}

%skeleton_shared_ptr(Skeleton::SerializeSessionLock);
namespace Skeleton {
std::string serialize_write_binary(const boost::shared_ptr<GenericObject>& Obj);

std::string serialize_write_pickle(const boost::shared_ptr<GenericObject>& Obj);
  
boost::shared_ptr<GenericObject> 
serialize_read_binary(const std::string& Data);

boost::shared_ptr<GenericObject> 
serialize_read_pickle(const std::string& Data);
  
class SerializeSessionLock : public GenericObject {
public:
  SerializeSessionLock();
  void clear();
};
}

%pythoncode %{
class PickleWithSwigSupport:
    '''Pickle, with extra support for working with Swig objects.'''
    def __enter__(self):
       self.lock = SerializeSessionLock()
       return self
  
    def __exit__(self, exc_type, exc_val, exc_tb):
       self.lock.clear()
       return False
%}

// List of things "import *" will include
%python_export("have_serialize_supported","serialize_write","serialize_write_string","serialize_read_generic","serialize_read_generic_string","serialize_write_binary","serialize_read_binary", "PickleWithSwigSupport")
