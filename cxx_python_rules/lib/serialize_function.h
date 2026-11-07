#pragma once
#include "generic_object.h"
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace CXX_PYTHON_RULES_NAMESPACE {
//-----------------------------------------------------------------------
/// Write and read one object (and everything it points to) with boost
/// serialization: XML or binary, to a file (gzip compressed if the name
/// ends in ".gz") or to a string. Errors throw std::runtime_error (or a
/// boost::archive exception).
//-----------------------------------------------------------------------

bool skip_weak_ptr_handling();
void skip_weak_ptr_handling(bool Skip);
bool have_serialize_supported();
void serialize_write(const std::filesystem::path& Fname,
                     const std::shared_ptr<GenericObject>& Obj);
void serialize_write_binary(const std::filesystem::path& Fname,
                            const std::shared_ptr<GenericObject>& Obj);

std::string serialize_write_string(const std::shared_ptr<GenericObject>& Obj);

std::shared_ptr<GenericObject>
serialize_read_generic(const std::filesystem::path& Fname);
std::shared_ptr<GenericObject>
serialize_read_binary_generic(const std::filesystem::path& Fname);

std::shared_ptr<GenericObject>
serialize_read_generic_string(const std::string& Data);

std::string serialize_write_binary(const std::shared_ptr<GenericObject>& Obj);
std::shared_ptr<GenericObject> serialize_read_binary(const std::string& Data);
std::shared_ptr<GenericObject>
serialize_read_binary(const std::string& Data,
                      const std::filesystem::path& Base_dir);

template<class T>
inline std::shared_ptr<T> serialize_read(const std::filesystem::path& Fname)
{
  return std::dynamic_pointer_cast<T>(serialize_read_generic(Fname));
}

template<class T>
inline std::shared_ptr<T>
serialize_read_binary(const std::filesystem::path& Fname)
{
  return std::dynamic_pointer_cast<T>(serialize_read_binary_generic(Fname));
}

template<class T>
inline std::shared_ptr<T> serialize_read_string(const std::string& Data)
{
  return std::dynamic_pointer_cast<T>(serialize_read_generic_string(Data));
}

template<class T>
inline std::shared_ptr<T> serialize_read_binary_string(const std::string& Data)
{
  return std::dynamic_pointer_cast<T>(serialize_read_binary(Data));
}

//-----------------------------------------------------------------------
/// Relative file names inside an archive (e.g. an object that reads a
/// file in its load()) are relative to the archive's directory, not the
/// current directory. Reading an archive from a file sets this base
/// directory for the time of the read; an object opens
/// resolve_archive_path(Name) instead of Name. The base is per thread
/// (so loads on different threads don't interfere; a thread started
/// during a load doesn't see it), and empty when nothing sets it.
///
/// This replaced changing the process's current directory during a load
/// (code review 2026-11-06 §2.5), which broke relative file access on
/// every other thread.
//-----------------------------------------------------------------------

const std::filesystem::path& archive_base_directory();

//-----------------------------------------------------------------------
/// Set the base directory for this thread, returning the previous one.
/// Prefer ArchiveBaseDirectory, which restores it.
//-----------------------------------------------------------------------

std::filesystem::path set_archive_base_directory(std::filesystem::path Dir);

//-----------------------------------------------------------------------
/// P if it is absolute or no base directory is set, otherwise the base
/// directory / P.
//-----------------------------------------------------------------------

std::filesystem::path resolve_archive_path(const std::filesystem::path& P);

//-----------------------------------------------------------------------
/// Set the archive base directory for a scope, and restore the previous
/// one at the end (so nested loads work).
//-----------------------------------------------------------------------

class ArchiveBaseDirectory {
public:
  explicit ArchiveBaseDirectory(std::filesystem::path Dir)
    : prev_(set_archive_base_directory(std::move(Dir)))
  {
  }
  ~ArchiveBaseDirectory() { set_archive_base_directory(std::move(prev_)); }
  ArchiveBaseDirectory(const ArchiveBaseDirectory&) = delete;
  ArchiveBaseDirectory& operator=(const ArchiveBaseDirectory&) = delete;
private:
  std::filesystem::path prev_;
};
}
