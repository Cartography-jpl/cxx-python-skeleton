#ifndef CXX_PYTHON_RULES_DIRECTOR_HOOK_H
#define CXX_PYTHON_RULES_DIRECTOR_HOOK_H
#include "generic_object.h"
#include <memory>

namespace CXX_PYTHON_RULES_NAMESPACE {
//-----------------------------------------------------------------------
/// Hook for serialize_shared_ptr.h, so a python subclass of a C++ class
/// (a nanobind trampoline object) read from an archive is owned by its
/// python object. The python extension module sets it; without one (pure
/// C++) it is null. Given the object just read, it returns the
/// shared_ptr to use for it, or null to use a normal one. Replaces the
/// SWIG-era SwigTypeMapperBase::is_python_director/
/// swig_python_director_setup, and keeps the C++ library free of python.
//-----------------------------------------------------------------------

using DirectorHook =
    std::shared_ptr<GenericObject> (*)(const std::shared_ptr<GenericObject>&);
DirectorHook& director_hook();
}
#endif
