#include "thread_example.h"
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

using namespace Skeleton;

int ThreadExample::run(const std::shared_ptr<DirectorExample>& Obj, int V,
                       int Number_thread)
{
  std::vector<int> res(Number_thread, 0);
  std::exception_ptr first_error;
  std::mutex m;
  std::vector<std::thread> threads;
  threads.reserve(Number_thread);
  for(int i = 0; i < Number_thread; ++i)
    threads.emplace_back([&, i] {
      try {
        res[i] = Obj->func(V + i);
      } catch(...) {
        std::lock_guard<std::mutex> lock(m);
        if(!first_error)
          first_error = std::current_exception();
      }
    });
  for(auto& t : threads)
    t.join();
  if(first_error)
    std::rethrow_exception(first_error);
  int sum = 0;
  for(int r : res)
    sum += r;
  return sum;
}
