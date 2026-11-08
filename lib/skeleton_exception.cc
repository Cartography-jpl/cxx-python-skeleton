#include "skeleton_exception.h"
// SKELETON_ADDR2LINE (not in the installed skeleton_config.h).
#include "skeleton_build_config.h"
#include <atomic>
#include <format>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>
// Boost.Stacktrace is only used in this file, so the header stays light
// and only libskeleton needs its link dependencies (-ldl). It records the
// addresses. If configure found addr2line (binutils), stacktrace() runs it
// for function names and file:line (symbolize() below); otherwise Boost's
// default backend gives addresses and exported names only.
#include <boost/stacktrace.hpp>
#ifdef SKELETON_ADDR2LINE
#include <boost/preprocessor/stringize.hpp>
#include <dlfcn.h>
#include <elf.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string_view>
#include <utility>
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
  // e_ident, then e_type: at offset 16 in both ELF classes, in the byte
  // order e_ident[EI_DATA] gives.
  std::array<char, EI_NIDENT + 2> h{};
  is.read(h.data(), static_cast<std::streamsize>(h.size()));
  if(!is || std::string_view(h.data(), SELFMAG) != ELFMAG)
    return false;
  auto b = [&](std::size_t I) {
    return static_cast<unsigned>(static_cast<unsigned char>(h[I]));
  };
  const unsigned type = b(EI_DATA) == unsigned{ELFDATA2MSB}
                            ? (b(EI_NIDENT) << 8U) | b(EI_NIDENT + 1)
                            : b(EI_NIDENT) | (b(EI_NIDENT + 1) << 8U);
  return type == unsigned{ET_DYN};
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
// The frames of a trace in one module: index in the trace, and the
// address to give addr2line (relative to the module's load address for a
// shared object).
using ModuleFrames = std::vector<std::pair<std::size_t, std::uintptr_t>>;

//-----------------------------------------------------------------------
// Map each address to its module and offset. Res gets the text to show
// if addr2line finds nothing ("<address> in <module>", or just the
// address if dladdr doesn't know it); the result has the frames to look
// up, by module.
//-----------------------------------------------------------------------

std::map<std::string, ModuleFrames> locate(const std::vector<void*>& Trace,
                                           std::vector<std::string>& Res)
{
  std::map<std::string, ModuleFrames> by_module;
  for(const auto [i, p] : std::views::enumerate(Trace)) {
    const auto idx = static_cast<std::size_t>(i);
    // A return address is just past the call, look up the call itself.
    auto a = reinterpret_cast<std::uintptr_t>(p);
    Dl_info di;
    if(a == 0 || !dladdr(static_cast<char*>(p) - 1, &di) || !di.dli_fname ||
       !*di.dli_fname) {
      Res[idx] = std::format("{}", p);
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
    const std::uintptr_t offset =
        a - 1 -
        (is_et_dyn(m) ? reinterpret_cast<std::uintptr_t>(di.dli_fbase) : 0);
    Res[idx] = std::format("{:#x} in {}", a, m);
    by_module[m].emplace_back(idx, offset);
  }
  return by_module;
}

//-----------------------------------------------------------------------
// Run addr2line once for the frames in Module, and replace their entries
// in Res with "function at file:line" (or "function in module") where it
// finds a function.
//-----------------------------------------------------------------------

void addr2line(const std::string& Module, const ModuleFrames& Frames,
               std::vector<std::string>& Res)
{
  // SKELETON_ADDR2LINE is the unquoted path (as Boost.Stacktrace wanted it).
  std::string cmd = shell_quote(BOOST_PP_STRINGIZE(SKELETON_ADDR2LINE)) +
                                " -C -f -e " + shell_quote(Module);
  for(const auto& fr : Frames)
    std::format_to(std::back_inserter(cmd), " {:#x}", fr.second);
  cmd += " 2>/dev/null";
  // Every part of cmd is quoted (shell_quote) or a number.
  FILE* f = popen(cmd.c_str(), "r"); // NOLINT(bugprone-command-processor)
  if(!f)
    return;
  std::array<char, 4096> buf{};
  for(const auto& fr : Frames) {
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
    std::string& r = Res[fr.first];
    r = std::move(func);
    r.append(loc.starts_with("??") ? " in " : " at ");
    r.append(loc.starts_with("??") ? Module : loc);
  }
  pclose(f);
}

std::vector<std::string> symbolize(const std::vector<void*>& Trace)
{
  std::vector<std::string> res(Trace.size());
  for(const auto& [m, frames] : locate(Trace, res))
    addr2line(m, frames, res);
  return res;
}
}
#endif

std::string Exception::stacktrace() const
{
  std::string res;
#ifdef SKELETON_ADDR2LINE
  for(const auto& [i, f] : std::views::enumerate(symbolize(d_->trace)))
    std::format_to(std::back_inserter(res), "{:2}# {}\n", i, f);
#else
  for(const auto [i, p] : std::views::enumerate(d_->trace))
    std::format_to(std::back_inserter(res), "{:2}# {}\n", i,
                   boost::stacktrace::to_string(boost::stacktrace::frame(p)));
#endif
  return res;
}
