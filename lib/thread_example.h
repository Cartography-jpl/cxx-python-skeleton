#ifndef THREAD_EXAMPLE_H
#define THREAD_EXAMPLE_H
#include "director_example.h"
#include "printable.h"
#include <memory>

namespace Skeleton {
/****************************************************************/ /**
  Calls a (possibly python) virtual function on C++ worker threads and
  waits for them, the way geocal's ThreadPool does. An exception thrown
  on a worker (including a python exception from an override) is saved
  as a std::exception_ptr and the first one is rethrown by the waiting
  thread. See claude_notes/implementation_plan.md S6.1 item 11.
*******************************************************************/
class ThreadExample : public Printable<ThreadExample> {
public:
  ThreadExample() = default;

  //-----------------------------------------------------------------------
  /// Call Obj->func(V + i) for i = 0 ... Number_thread - 1, each on its
  /// own thread, and return the sum of the results.
  //-----------------------------------------------------------------------

  static int run(const std::shared_ptr<DirectorExample>& Obj, int V,
                 int Number_thread);
  virtual void print(std::ostream& Os) const { Os << "ThreadExample"; }
};
}
#endif
