#define PYTHON_MODULE_NAME _swig_wrap
#include "python_lib_init.h"

extern "C" {
#include "skeleton_config.h"
  INIT_TYPE INIT_FUNC(_swig_array)(void);
  INIT_TYPE INIT_FUNC(_swig_boost_array)(void);
}

static void module_init(PyObject* module)
{
  INIT_MODULE(module, "_swig_array", INIT_FUNC(_swig_array));
  INIT_MODULE(module, "_swig_boost_array", INIT_FUNC(_swig_boost_array));
}


