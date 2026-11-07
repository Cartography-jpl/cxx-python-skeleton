// The python extension module skeleton_cxx._bindings. Each lib/*_nb.cc
// file binds one header and has an init_<name> function, called below.
#include "skeleton_nb_opaque.h"
#include "skeleton_exception.h"

namespace nb = nanobind;

void init_generic_object(nb::module_& m);
void init_array_sample(nb::module_& m);
void init_array_typemap_sample(nb::module_& m);
void init_director_example(nb::module_& m);
void init_skeleton_exception(nb::module_& m);
void init_exception_sample(nb::module_& m);
void init_generic_object_sample(nb::module_& m);
void init_file_reference_sample(nb::module_& m);
void init_generic_object_map(nb::module_& m);
void init_iostream_swig(nb::module_& m);
void init_observer(nb::module_& m);
void init_with_parameter_like(nb::module_& m);
void init_multi_base_example(nb::module_& m);
void init_numpy_sample(nb::module_& m);
void init_observer_example(nb::module_& m);
void init_overload_example(nb::module_& m);
void init_raster_sample(nb::module_& m);
void init_serialize_function(nb::module_& m);
void init_cxx_python_rules_sample(nb::module_& m);
void init_thread_example(nb::module_& m);
void init_virtual_diamond_example(nb::module_& m);

NB_MODULE(_bindings, m)
{
  cxx_python_rules_nb::register_exceptions<Skeleton::Exception,
                                           Skeleton::TypeError>();
  cxx_python_rules_nb::init_module(m);
  cxx_python_rules_nb::bind_common_types(m);
  // A class has to be bound before any class that has it as its nanobind
  // base (nb::class_<Derived, Base>), so the order matters: e.g.
  // init_observer (CacheInvalidatedObserver) comes before
  // init_observer_example (CacheExample).
  init_generic_object(m);
  init_array_sample(m);
  init_array_typemap_sample(m);
  init_director_example(m);
  init_skeleton_exception(m);
  init_exception_sample(m);
  init_generic_object_sample(m);
  init_file_reference_sample(m);
  init_generic_object_map(m);
  init_iostream_swig(m);
  init_observer(m);
  init_with_parameter_like(m);
  init_multi_base_example(m);
  init_numpy_sample(m);
  init_observer_example(m);
  init_overload_example(m);
  init_raster_sample(m);
  init_serialize_function(m);
  init_cxx_python_rules_sample(m);
  init_thread_example(m);
  init_virtual_diamond_example(m);
}
