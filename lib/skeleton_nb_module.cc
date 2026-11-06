// The python extension module skeleton_cxx._bindings. Each
// lib/*_nb.cc file has an init function, called here in dependency order
// (skeleton_nb_init.inc).
#include "skeleton_nb_opaque.h"
#include "skeleton_exception.h"

namespace nb = nanobind;

#include "skeleton_nb_init.inc"

NB_MODULE(_bindings, m)
{
  cxx_python_rules_nb::register_exceptions<Skeleton::Exception,
                                           Skeleton::TypeError>();
  cxx_python_rules_nb::init_module(m);
  cxx_python_rules_nb::bind_common_types(m);
  CXX_PYTHON_RULES_NB_INIT_ALL(m);
}
