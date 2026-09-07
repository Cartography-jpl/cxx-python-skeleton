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

boost::shared_ptr<GenericObject>
serialize_read_binary_generic(const std::string& Fname);
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
import pickle

class FileSwap:
    """When we do pickling combined with boost serialization, we have
    different places where the pickle information comes from. There can
    be a straight file, and then data passed from boost serialization. However
    pickle.Pickler and pickle.Unpickler isn't set up to tame multiple streams.
    So we make this look like one stream. We do this by allowing the file
    handle to be replaced."""

    def __init__(self, file_base):
        self.file_base = file_base
        self.file_replace = None

    def read(self, size=-1):
        if self.file_replace is None:
            return self.file_base.read(size)
        return self.file_replace.read(size)

    def readinto(self, b):
        if self.file_replace is None:
            return self.file_base.readinto(b)
        return self.file_replace.readinto(b)

    def readline(self):
        if self.file_replace is None:
            return self.file_base.readline()
        return self.file_replace.readline()

    def write(self, b):
        if self.file_replace is None:
            return self.file_base.write(b)
        return self.file_replace.write(b)


class PicklerWithSerializationSupport(pickle.Pickler):
    '''Because we need to pass between C++ and python (e.g., a python
       object contains a director with C++ code that has been derived
       in python and has python objects) we need special handling to
       pass between python pickle and boost serialization. We
       automatically handle this if you are reading or writing a
       shelve file (e.g., a pressure.xml or something like that).

       However, we need extra handling for pickling a python object
       that may contain a Director. We have a specialization of
       pickle.Pickler called PicklerWithSerializationSupport and
       pickle.Unpickler called UnpicklerWithSerializationSupport. This
       is the standard way python has for handling special
       picklization code. As a convenience, this package also has a
       version of dump, dumps, load, and loads that used these pickler
       objects, just like pickle.dump is a shortcut for using
       pickle.Pickler. You can either use the Pickler classes
       directly, or through these short cut functions.
    '''

    def __init__(self, file, protocol=None, fix_imports=True, buffer_callback=None):
        self.file = FileSwap(file)
        super().__init__(
            self.file,
            protocol=protocol,
            fix_imports=fix_imports,
            buffer_callback=buffer_callback,
        )

    def serialization_dumps(self, obj):
        import io
        try:
            self.file.file_replace = io.BytesIO()
            self.dump(obj)
            return self.file.file_replace.getvalue()
        finally:
            self.file.file_replace = None


class UnpicklerWithSerializationSupport(pickle.Unpickler):
    '''Because we need to pass between C++ and python (e.g., a python
       object contains a director with C++ code that has been derived
       in python and has python objects) we need special handling to
       pass between python pickle and boost serialization. We
       automatically handle this if you are reading or writing a
       shelve file (e.g., a pressure.xml or something like that).

       However, we need extra handling for pickling a python object
       that may contain a Director. We have a specialization of
       pickle.Pickler called PicklerWithSerializationSupport and
       pickle.Unpickler called UnpicklerWithSerializationSupport. This
       is the standard way python has for handling special
       picklization code. As a convenience, this package also has a
       version of dump, dumps, load, and loads that used these pickler
       objects, just like pickle.dump is a shortcut for using
       pickle.Pickler. You can either use the Pickler classes
       directly, or through these short cut functions.
    '''
    def __init__(
        self, file, fix_imports=True, encoding="ASCII", errors="strict", buffers=None
    ):
        self.file = FileSwap(file)
        super().__init__(
            self.file,
            fix_imports=fix_imports,
            encoding=encoding,
            errors=errors,
            buffers=buffers,
        )

    def serialization_loads(self, data):
        import io
        try:
            self.file.file_replace = io.BytesIO(data)
            return self.load()
        finally:
            self.file.file_replace = None


def dump(
    obj, file, protocol=None, fix_imports=True, buffer_callback=None
):
    '''Because we need to pass between C++ and python (e.g., a python
       object contains a director with C++ code that has been derived
       in python and has python objects) we need special handling to
       pass between python pickle and boost serialization. We
       automatically handle this if you are reading or writing a
       shelve file (e.g., a pressure.xml or something like that).

       However, we need extra handling for pickling a python object
       that may contain a Director. We have a specialization of
       pickle.Pickler called PicklerWithSerializationSupport and
       pickle.Unpickler called UnpicklerWithSerializationSupport. This
       is the standard way python has for handling special
       picklization code. As a convenience, this package also has a
       version of dump, dumps, load, and loads that used these pickler
       objects, just like pickle.dump is a shortcut for using
       pickle.Pickler. You can either use the Pickler classes
       directly, or through these short cut functions.
    '''
    p = PicklerWithSerializationSupport(
        file,
        protocol=protocol,
        fix_imports=fix_imports,
        buffer_callback=buffer_callback,
    )
    p.dump(obj)


def dumps(
    obj, protocol=None, fix_imports=True, buffer_callback=None
):
    '''Because we need to pass between C++ and python (e.g., a python
       object contains a director with C++ code that has been derived
       in python and has python objects) we need special handling to
       pass between python pickle and boost serialization. We
       automatically handle this if you are reading or writing a
       shelve file (e.g., a pressure.xml or something like that).

       However, we need extra handling for pickling a python object
       that may contain a Director. We have a specialization of
       pickle.Pickler called PicklerWithSerializationSupport and
       pickle.Unpickler called UnpicklerWithSerializationSupport. This
       is the standard way python has for handling special
       picklization code. As a convenience, this package also has a
       version of dump, dumps, load, and loads that used these pickler
       objects, just like pickle.dump is a shortcut for using
       pickle.Pickler. You can either use the Pickler classes
       directly, or through these short cut functions.
    '''
    import io
    file = io.BytesIO()
    p = PicklerWithSerializationSupport(
        file,
        protocol=protocol,
        fix_imports=fix_imports,
        buffer_callback=buffer_callback,
    )
    p.dump(obj)
    return file.getvalue()


def load(
    file, fix_imports=True, encoding="ASCII", errors="strict", buffers=None
):
    '''Because we need to pass between C++ and python (e.g., a python
       object contains a director with C++ code that has been derived
       in python and has python objects) we need special handling to
       pass between python pickle and boost serialization. We
       automatically handle this if you are reading or writing a
       shelve file (e.g., a pressure.xml or something like that).

       However, we need extra handling for pickling a python object
       that may contain a Director. We have a specialization of
       pickle.Pickler called PicklerWithSerializationSupport and
       pickle.Unpickler called UnpicklerWithSerializationSupport. This
       is the standard way python has for handling special
       picklization code. As a convenience, this package also has a
       version of dump, dumps, load, and loads that used these pickler
       objects, just like pickle.dump is a shortcut for using
       pickle.Pickler. You can either use the Pickler classes
       directly, or through these short cut functions.
    '''
    p = UnpicklerWithSerializationSupport(
        file, fix_imports=fix_imports, encoding=encoding, errors=errors, buffers=None
    )
    return p.load()


def loads(
    data, fix_imports=True, encoding="ASCII", errors="strict", buffers=None
):
    '''Because we need to pass between C++ and python (e.g., a python
       object contains a director with C++ code that has been derived
       in python and has python objects) we need special handling to
       pass between python pickle and boost serialization. We
       automatically handle this if you are reading or writing a
       shelve file (e.g., a pressure.xml or something like that).

       However, we need extra handling for pickling a python object
       that may contain a Director. We have a specialization of
       pickle.Pickler called PicklerWithSerializationSupport and
       pickle.Unpickler called UnpicklerWithSerializationSupport. This
       is the standard way python has for handling special
       picklization code. As a convenience, this package also has a
       version of dump, dumps, load, and loads that used these pickler
       objects, just like pickle.dump is a shortcut for using
       pickle.Pickler. You can either use the Pickler classes
       directly, or through these short cut functions.
    '''
    import io
    file = io.BytesIO(data)
    p = UnpicklerWithSerializationSupport(
        file, fix_imports=fix_imports, encoding=encoding, errors=errors, buffers=None
    )
    return p.load()
%}

// List of things "import *" will include
%python_export("have_serialize_supported","serialize_write","serialize_write_string","serialize_read_generic","serialize_read_generic_string","serialize_write_binary","serialize_read_binary", "serialize_read_binary_generic", "PicklerWithSerializationSupport", "UnpicklerWithSerializationSupport", "dump", "dumps", "load", "loads")
