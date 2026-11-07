#include "director_hook.h"

CXX_PYTHON_RULES_NAMESPACE::DirectorHook&
CXX_PYTHON_RULES_NAMESPACE::director_hook()
{
  // Set once, by the python extension module when it is imported.
  static DirectorHook h = nullptr;
  return h;
}
