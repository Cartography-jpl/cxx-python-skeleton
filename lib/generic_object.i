%include "skeleton_common.i"
%{
#include "generic_object.h"
%}

%skeleton_shared_ptr(Skeleton::GenericObject)
namespace Skeleton {
class GenericObject {
public:
  virtual ~GenericObject();
};
}

%typemap(out) const boost::shared_ptr< Skeleton::GenericObject >& {
  %set_output(Skeleton::swig_to_python_or_none($1));
}

%typemap(out) boost::shared_ptr< Skeleton::GenericObject >& {
  %set_output(Skeleton::swig_to_python_or_none($1));
}

%typemap(out) boost::shared_ptr< Skeleton::GenericObject > {
  %set_output(Skeleton::swig_to_python_or_none($1));
}

// List of things "import *" will include
%python_export("GenericObject")
