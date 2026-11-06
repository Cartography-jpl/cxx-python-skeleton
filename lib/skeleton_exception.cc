#include "skeleton_exception.h"
#include <atomic>
// Boost.Stacktrace is only used in this file, so the header stays light
// and only libskeleton needs its link dependencies (-ldl). If configure
// found addr2line (binutils), use it for function names and file:line;
// otherwise the default backend gives addresses and exported names only.
#ifdef SKELETON_ADDR2LINE
#define BOOST_STACKTRACE_USE_ADDR2LINE
#define BOOST_STACKTRACE_ADDR2LINE_LOCATION SKELETON_ADDR2LINE
#endif
#include <boost/stacktrace.hpp>

using namespace Skeleton;

static std::atomic<bool> capture_stacktrace_flag{true};

void Exception::capture_stacktrace(bool V) { capture_stacktrace_flag = V; }
bool Exception::capture_stacktrace() { return capture_stacktrace_flag; }

//-----------------------------------------------------------------------
/// Record the return addresses of the current stack. Only the addresses
/// are stored; symbol lookup happens in stacktrace().
//-----------------------------------------------------------------------

void Exception::capture()
{
  if(!capture_stacktrace_flag)
    return;
  try {
    // Skip this function itself.
    boost::stacktrace::stacktrace st(1, static_cast<std::size_t>(-1));
    trace_.reserve(st.size());
    for(const auto& f : st)
      trace_.push_back(const_cast<void*>(f.address()));
  } catch(...) {
    // A failure to get the stack trace shouldn't replace the error being
    // reported, so just go without one.
    trace_.clear();
  }
}

std::string Exception::stacktrace() const
{
  std::string res;
  for(std::size_t i = 0; i < trace_.size(); ++i)
    res += std::format("{:2}# {}\n", i,
                       boost::stacktrace::to_string(
                         boost::stacktrace::frame(trace_[i])));
  return res;
}
