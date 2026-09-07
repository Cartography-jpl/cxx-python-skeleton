// This wraps a shared_ptr up for Skeleton. In particular, we use
// RTTI to make sure that the most specific type is returned in
// python, rather than the most general type. This maps better to the
// standard duck typing done in python.

%{
#define SWIG_MAPPER_NAMESPACE Skeleton
%}

%include "shared_ptr_type_mapper.i"

%define %skeleton_shared_ptr(TYPE...)
%shared_ptr_type_mapper(Skeleton, TYPE)
%enddef
