// Python bindings for GenericObject (generic_object.h), the base of every
// bound class. A shared_ptr<GenericObject> returned from C++ becomes the
// most derived python type through the shared_ptr caster (gc_shared_ptr.h).
#include "skeleton_nb_opaque.h"
#include "generic_object.h"

namespace nb = nanobind;
using namespace Skeleton;

void init_generic_object(nb::module_& m)
{
  const auto doc = cxx_python_rules_nb::docs("Skeleton::GenericObject");
  nb::class_<GenericObject> c(m, "GenericObject", doc());
  cxx_python_rules_nb::register_type<GenericObject>();
}
