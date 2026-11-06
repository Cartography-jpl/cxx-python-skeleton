// The python extension module skeleton_swig._swig_wrap (the name is
// historical, see claude_notes/implementation_plan.md "Naming"). Each
// lib/*_nb.cc file has an init function, called here in dependency order
// (skeleton_nb_init.inc).
#include "skeleton_nb_opaque.h"
#include "skeleton_exception.h"

namespace nb = nanobind;

#include "skeleton_nb_init.inc"

NB_MODULE(_swig_wrap, m)
{
  swig_rules_nb::register_exceptions<Skeleton::Exception,
                                     Skeleton::TypeError>();
  swig_rules_nb::init_module(m);
  swig_rules_nb::bind_common_types(m);
  SWIG_RULES_NB_INIT_ALL(m);
}
