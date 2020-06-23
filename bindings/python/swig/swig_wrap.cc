#define PYTHON_MODULE_NAME _swig_wrap
#include "python_lib_init.h"

extern "C" {
#include "skeleton_config.h"
  INIT_TYPE INIT_FUNC(_swig_array)(void);
  INIT_TYPE INIT_FUNC(_swig_boost_array)(void);
  INIT_TYPE INIT_FUNC(_swig_std)(void);
  INIT_TYPE INIT_FUNC(_skeleton_exception)(void);
  INIT_TYPE INIT_FUNC(_generic_object)(void);
  INIT_TYPE INIT_FUNC(_generic_object_sample)(void);
  INIT_TYPE INIT_FUNC(_generic_object_map)(void);
  INIT_TYPE INIT_FUNC(_iostream_swig)(void);
  INIT_TYPE INIT_FUNC(_serialize_function)(void);
  INIT_TYPE INIT_FUNC(_numpy_sample)(void);
}

static void module_init(PyObject* module)
{
  INIT_MODULE(module, "_swig_array", INIT_FUNC(_swig_array));
  INIT_MODULE(module, "_swig_boost_array", INIT_FUNC(_swig_boost_array));
  INIT_MODULE(module, "_swig_std", INIT_FUNC(_swig_std));
  INIT_MODULE(module, "_skeleton_exception", INIT_FUNC(_skeleton_exception));
  INIT_MODULE(module, "_generic_object", INIT_FUNC(_generic_object));
  INIT_MODULE(module, "_generic_object_sample", INIT_FUNC(_generic_object_sample));
  INIT_MODULE(module, "_generic_object_map", INIT_FUNC(_generic_object_map));
  INIT_MODULE(module, "_iostream_swig", INIT_FUNC(_iostream_swig));
  INIT_MODULE(module, "_serialize_function", INIT_FUNC(_serialize_function));
  INIT_MODULE(module, "_numpy_sample", INIT_FUNC(_numpy_sample));
}


