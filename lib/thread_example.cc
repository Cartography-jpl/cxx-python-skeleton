#include "thread_example.h"
#include "skeleton_exception.h"
#include <algorithm>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

using namespace Skeleton;

int ThreadExample::run(const std::shared_ptr<DirectorExample>& Obj, int V,
                       int Number_thread)
{
  if(!Obj)
    throw Exception("ThreadExample::run: Obj is null");
  if(Number_thread < 1)
    throw Exception("ThreadExample::run: Number_thread must be at least 1, "
                    "got {}",
                    Number_thread);
  std::vector<int> res(static_cast<std::size_t>(Number_thread), 0);
  std::exception_ptr first_error;
  std::mutex m;
  {
    // Declared after what the threads use, so it is destroyed first:
    // std::jthread joins in its destructor, also when creating a later
    // thread throws (a joinable std::thread would call std::terminate).
    std::vector<std::jthread> threads;
    threads.reserve(res.size());
    for(std::size_t i = 0; i < res.size(); ++i)
      threads.emplace_back([&, i] {
        try {
          res[i] = Obj->func(V + static_cast<int>(i));
        } catch(...) {
          std::scoped_lock lock(m);
          if(!first_error)
            first_error = std::current_exception();
        }
      });
  }
  if(first_error)
    std::rethrow_exception(first_error);
  return std::ranges::fold_left(res, 0, std::plus<>());
}
