// Python bindings for ThreadExample (thread_example.h).
#include "skeleton_nb_opaque.h"
#include "thread_example.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_thread_example(nb::module_& m)
{
  const auto doc = cxx_python_rules_nb::docs("Skeleton::ThreadExample");
  nb::class_<ThreadExample> c(m, "ThreadExample", doc());
  c.def(nb::new_([]() { return std::make_shared<ThreadExample>(); }));
  // Release the GIL while waiting for the worker threads, so a python
  // override called on a worker can take it (the trampoline does).
  // Without this it deadlocks.
  c.def_static("run", &ThreadExample::run, "Obj"_a, "V"_a, "Number_thread"_a,
               nb::call_guard<nb::gil_scoped_release>(), doc("run"));
  cxx_python_rules_nb::printable(c);
  cxx_python_rules_nb::register_type<ThreadExample>();
}
