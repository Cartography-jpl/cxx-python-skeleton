#include "skeleton_exception.h"
// SKELETON_ADDR2LINE (not in the installed skeleton_config.h).
#include "skeleton_build_config.h"
#include <atomic>
// Boost.Stacktrace is only used in this file, so the header stays light
// and only libskeleton needs its link dependencies (-ldl). It records the
// addresses. If configure found addr2line (binutils), stacktrace() runs it
// for function names and file:line (symbolize() below); otherwise Boost's
// default backend gives addresses and exported names only.
#include <boost/stacktrace.hpp>
#ifdef SKELETON_ADDR2LINE
#include <boost/preprocessor/stringize.hpp>
#include <dlfcn.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#endif

using namespace Skeleton;

namespace {
std::atomic<bool> capture_stacktrace_flag{true};
}

void Exception::capture_stacktrace(bool V)
{
  capture_stacktrace_flag = V;
}
bool Exception::capture_stacktrace()
{
  return capture_stacktrace_flag;
}

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
    d_->trace.reserve(st.size());
    for(const auto& f : st)
      // Stored as void* for frame(void*) in stacktrace(); never written.
      d_->trace.push_back(const_cast<void*>(f.address()));
  } catch(...) {
    // A failure to get the stack trace shouldn't replace the error being
    // reported, so just go without one.
    d_->trace.clear();
  }
}

#ifdef SKELETON_ADDR2LINE
namespace {
// Is the ELF file a shared object (or PIE executable, both ET_DYN)? Then
// addr2line wants addresses relative to where it was loaded.
bool is_et_dyn(const std::string& File)
{
  std::ifstream is(File, std::ios::binary);
  std::array<char, 18> h{};
  is.read(h.data(), static_cast<std::streamsize>(h.size()));
  return is && h[16] == 3 && h[17] == 0;
}

std::string shell_quote(const std::string& S)
{
  std::string r = "'";
  for(char c : S)
    r += (c == '\'' ? std::string("'\\''") : std::string(1, c));
  return r + "'";
}
}

//-----------------------------------------------------------------------
/// Function name and file:line for each address, with one addr2line run
/// per module. Boost.Stacktrace's addr2line backend runs it once per
/// frame, always on the main executable, which gives wrong (and slow)
/// answers for code in shared libraries, e.g. a python extension.
//-----------------------------------------------------------------------

namespace {
std::vector<std::string> symbolize(const std::vector<void*>& Trace)
{
  std::vector<std::string> res(Trace.size());
  std::map<std::string, std::vector<std::size_t>> by_module;
  std::vector<std::uintptr_t> offset(Trace.size(), 0);
  for(std::size_t i = 0; i < Trace.size(); ++i) {
    // A return address is just past the call, look up the call itself.
    auto a = reinterpret_cast<std::uintptr_t>(Trace[i]);
    Dl_info di;
    if(a == 0 || !dladdr(static_cast<char*>(Trace[i]) - 1, &di) ||
       !di.dli_fname || !*di.dli_fname) {
      res[i] = std::format("{}", Trace[i]);
      continue;
    }
    std::string m = di.dli_fname;
    // The main program comes back as given on the command line.
    if(!m.contains('/')) {
      std::error_code ec;
      m = std::filesystem::read_symlink("/proc/self/exe", ec).string();
      if(ec)
        m = di.dli_fname;
    }
    offset[i] =
        a - 1 -
        (is_et_dyn(m) ? reinterpret_cast<std::uintptr_t>(di.dli_fbase) : 0);
    res[i] = std::format("{:#x} in {}", a, m);
    by_module[m].push_back(i);
  }
  for(const auto& [m, idx] : by_module) {
    // SKELETON_ADDR2LINE is the unquoted path (as Boost.Stacktrace wanted it).
    std::string cmd = shell_quote(BOOST_PP_STRINGIZE(SKELETON_ADDR2LINE)) +
                                  " -C -f -e " + shell_quote(m);
    for(std::size_t i : idx)
      cmd += std::format(" {:#x}", offset[i]);
    cmd += " 2>/dev/null";
    // Every part of cmd is quoted (shell_quote) or a number.
    FILE* f = popen(cmd.c_str(), "r"); // NOLINT(bugprone-command-processor)
    if(!f)
      continue;
    std::array<char, 4096> buf{};
    for(std::size_t i : idx) {
      std::string func, loc;
      if(fgets(buf.data(), static_cast<int>(buf.size()), f))
        func = buf.data();
      if(fgets(buf.data(), static_cast<int>(buf.size()), f))
        loc = buf.data();
      while(!func.empty() && func.back() == '\n')
        func.pop_back();
      while(!loc.empty() && loc.back() == '\n')
        loc.pop_back();
      if(func.empty() || func == "??")
        continue;
      res[i] = std::move(func);
      res[i].append(loc.starts_with("??") ? " in " : " at ");
      res[i].append(loc.starts_with("??") ? m : loc);
    }
    pclose(f);
  }
  return res;
}
}
#endif

std::string Exception::stacktrace() const
{
  std::string res;
#ifdef SKELETON_ADDR2LINE
  std::vector<std::string> f = symbolize(d_->trace);
  for(std::size_t i = 0; i < f.size(); ++i)
    res += std::format("{:2}# {}\n", i, f[i]);
#else
  for(std::size_t i = 0; i < d_->trace.size(); ++i)
    res += std::format(
        "{:2}# {}\n", i,
        boost::stacktrace::to_string(boost::stacktrace::frame(d_->trace[i])));
#endif
  return res;
}
