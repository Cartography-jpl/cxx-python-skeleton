#include "serialize_function.h"
#include "serialize_shared_ptr.h"
#include "weak_ptr_serialize_support.h"
#include <atomic>
#include <cerrno>
#include <system_error>
#include <fstream>
#include <spanstream>
#include <sstream>
#include <stdexcept>
#include <string>
#ifdef CXX_PYTHON_RULES_HAVE_BOOST_SERIALIZATION
#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/archive/polymorphic_binary_oarchive.hpp>
#include <boost/archive/polymorphic_xml_iarchive.hpp>
#include <boost/archive/polymorphic_xml_oarchive.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#endif
using namespace CXX_PYTHON_RULES_NAMESPACE;

//-----------------------------------------------------------------------
// The archive base directory (see serialize_function.h).
//-----------------------------------------------------------------------

namespace {
thread_local std::filesystem::path archive_base_dir;
}

const std::filesystem::path&
CXX_PYTHON_RULES_NAMESPACE::archive_base_directory()
{
  return archive_base_dir;
}

std::filesystem::path CXX_PYTHON_RULES_NAMESPACE::set_archive_base_directory(
    std::filesystem::path Dir)
{
  std::swap(Dir, archive_base_dir);
  return Dir;
}

std::filesystem::path
CXX_PYTHON_RULES_NAMESPACE::resolve_archive_path(const std::filesystem::path& P)
{
  if(P.is_absolute() || archive_base_dir.empty())
    return P;
  return archive_base_dir / P;
}

namespace {
std::atomic<bool> skip_weak_ptr_handling_val = false;
}

//-----------------------------------------------------------------------
/// As described in weak_ptr_serialize_support.h, writing an archive goes
/// through the serialization twice: once to mark the objects we
/// encounter, and once to write, keeping only the weak_ptrs to marked
/// objects. If you don't need that for std::weak_ptr, turn it off to
/// avoid the cost of the first pass.
//-----------------------------------------------------------------------

bool CXX_PYTHON_RULES_NAMESPACE::skip_weak_ptr_handling()
{
  return skip_weak_ptr_handling_val;
}

void CXX_PYTHON_RULES_NAMESPACE::skip_weak_ptr_handling(bool Skip)
{
  skip_weak_ptr_handling_val = Skip;
}

#ifdef CXX_PYTHON_RULES_HAVE_BOOST_SERIALIZATION
//-----------------------------------------------------------------------
/// Return true if we were built with serialization support, false
/// otherwise.
//-----------------------------------------------------------------------

bool CXX_PYTHON_RULES_NAMESPACE::have_serialize_supported()
{
  return true;
}

namespace {
using boost::serialization::make_nvp;
// The element name of the top object. "geocal_object" for compatibility
// with archives written before this code was shared.
constexpr const char* top_name = "geocal_object";

//-----------------------------------------------------------------------
// ": " and the system's reason for a failed open, if there is one. errno
// is cleared before opening: std::fstream doesn't promise to set it.
// std::generic_category().message is thread safe, unlike strerror.
//-----------------------------------------------------------------------

std::filesystem::path clear_errno(const std::filesystem::path& Fname)
{
  errno = 0;
  return Fname;
}

std::string errno_text()
{
  if(errno == 0)
    return "";
  return ": " + std::generic_category().message(errno);
}

//-----------------------------------------------------------------------
// A file opened for writing (reading), gzip compressed (decompressed)
// if its name ends in ".gz". Opening errors throw, naming the file.
//-----------------------------------------------------------------------

class OutputFile {
public:
  explicit OutputFile(const std::filesystem::path& Fname)
    : f(clear_errno(Fname))
  {
    if(!f)
      throw std::runtime_error("Trouble opening file " + Fname.string() +
                               " for writing" + errno_text());
    if(Fname.extension() == ".gz")
      sb.push(boost::iostreams::gzip_compressor());
    sb.push(f);
  }
  std::ostream& stream() { return os; }
private:
  // Declaration order matters: the filter (sb, which may hold
  // compressed data still to write) is destroyed before the file (f).
  std::ofstream f;
  boost::iostreams::filtering_ostreambuf sb;
  std::ostream os{&sb};
};

class InputFile {
public:
  explicit InputFile(const std::filesystem::path& Fname) : f(clear_errno(Fname))
  {
    if(!f)
      throw std::runtime_error("Trouble opening file " + Fname.string() +
                               errno_text());
    if(Fname.extension() == ".gz")
      sb.push(boost::iostreams::gzip_decompressor());
    sb.push(f);
  }
  std::istream& stream() { return is; }
private:
  std::ifstream f;
  boost::iostreams::filtering_istreambuf sb;
  std::istream is{&sb};
};

//-----------------------------------------------------------------------
// Go through the serialization once, marking all pointers we encounter
// (see skip_weak_ptr_handling). The scope's marks start empty, so
// skipping the first pass means no weak_ptr to a GenericObject is kept.
//-----------------------------------------------------------------------

void mark_pointer(const std::shared_ptr<GenericObject>& Obj)
{
  if(skip_weak_ptr_handling())
    return;
  std::ostringstream os_first_pass;
  boost::archive::polymorphic_binary_oarchive oa(os_first_pass);
  oa << make_nvp(top_name, Obj);
  // Don't actually want output, just side effect of marking pointers
}

template<class OArchive>
void write(std::ostream& Os, const std::shared_ptr<GenericObject>& Obj)
{
  // Marks of our own for the mark pass and the write, so a write nested
  // in this one doesn't disturb them (weak_ptr_serialize_support.h).
  PtrSerializeScope scope;
  mark_pointer(Obj);
  OArchive oa(Os);
  oa << make_nvp(top_name, Obj);
}

template<class IArchive> std::shared_ptr<GenericObject> read(std::istream& Is)
{
  IArchive ia(Is);
  std::shared_ptr<GenericObject> obj;
  ia >> make_nvp(top_name, obj);
  return obj;
}

//-----------------------------------------------------------------------
// Read a file. Relative names inside the archive are relative to the
// archive's directory (archive_base_directory()); the file name itself,
// if relative, is resolved against the current base directory too, so a
// nested read (an object loading another archive) finds its file as it
// did when this code changed the current directory.
//-----------------------------------------------------------------------

template<class IArchive>
std::shared_ptr<GenericObject> read_file(const std::filesystem::path& Fname)
{
  std::filesystem::path f = resolve_archive_path(Fname);
  InputFile in(f);
  ArchiveBaseDirectory base(std::filesystem::absolute(f).parent_path());
  return read<IArchive>(in.stream());
}
}

void CXX_PYTHON_RULES_NAMESPACE::serialize_write(
    const std::filesystem::path& Fname,
    const std::shared_ptr<GenericObject>& Obj)
{
  OutputFile out(Fname);
  write<boost::archive::polymorphic_xml_oarchive>(out.stream(), Obj);
}

void CXX_PYTHON_RULES_NAMESPACE::serialize_write_binary(
    const std::filesystem::path& Fname,
    const std::shared_ptr<GenericObject>& Obj)
{
  OutputFile out(Fname);
  write<boost::archive::polymorphic_binary_oarchive>(out.stream(), Obj);
}

std::string CXX_PYTHON_RULES_NAMESPACE::serialize_write_string(
    const std::shared_ptr<GenericObject>& Obj)
{
  std::ostringstream os;
  write<boost::archive::polymorphic_xml_oarchive>(os, Obj);
  return os.str();
}

std::string CXX_PYTHON_RULES_NAMESPACE::serialize_write_binary_string(
    const std::shared_ptr<GenericObject>& Obj)
{
  std::ostringstream os;
  write<boost::archive::polymorphic_binary_oarchive>(os, Obj);
  return os.str();
}

std::shared_ptr<GenericObject>
CXX_PYTHON_RULES_NAMESPACE::serialize_read_generic(
    const std::filesystem::path& Fname)
{
  return read_file<boost::archive::polymorphic_xml_iarchive>(Fname);
}

std::shared_ptr<GenericObject>
CXX_PYTHON_RULES_NAMESPACE::serialize_read_binary_generic(
    const std::filesystem::path& Fname)
{
  return read_file<boost::archive::polymorphic_binary_iarchive>(Fname);
}

std::shared_ptr<GenericObject>
CXX_PYTHON_RULES_NAMESPACE::serialize_read_generic_string(std::string_view Data)
{
  // A stream over the caller's data, without copying it.
  std::ispanstream is(Data);
  return read<boost::archive::polymorphic_xml_iarchive>(is);
}

std::shared_ptr<GenericObject>
CXX_PYTHON_RULES_NAMESPACE::serialize_read_binary_generic_string(
    std::string_view Data)
{
  std::ispanstream is(Data);
  return read<boost::archive::polymorphic_binary_iarchive>(is);
}

std::shared_ptr<GenericObject>
CXX_PYTHON_RULES_NAMESPACE::serialize_read_binary_generic_string(
    std::string_view Data, const std::filesystem::path& Base_dir)
{
  ArchiveBaseDirectory base(Base_dir);
  return serialize_read_binary_generic_string(Data);
}

#else
//-----------------------------------------------------------------------
// Built without boost serialization: every read and write throws.
//-----------------------------------------------------------------------

bool CXX_PYTHON_RULES_NAMESPACE::have_serialize_supported()
{
  return false;
}

namespace {
[[noreturn]] void no_serialization()
{
  throw std::runtime_error(
      "This library was built without boost::serialization support");
}
}

void CXX_PYTHON_RULES_NAMESPACE::serialize_write(
    const std::filesystem::path&, const std::shared_ptr<GenericObject>&)
{
  no_serialization();
}
void CXX_PYTHON_RULES_NAMESPACE::serialize_write_binary(
    const std::filesystem::path&, const std::shared_ptr<GenericObject>&)
{
  no_serialization();
}
std::string CXX_PYTHON_RULES_NAMESPACE::serialize_write_string(
    const std::shared_ptr<GenericObject>&)
{
  no_serialization();
}
std::string CXX_PYTHON_RULES_NAMESPACE::serialize_write_binary_string(
    const std::shared_ptr<GenericObject>&)
{
  no_serialization();
}
std::shared_ptr<GenericObject>
CXX_PYTHON_RULES_NAMESPACE::serialize_read_generic(const std::filesystem::path&)
{
  no_serialization();
}
std::shared_ptr<GenericObject>
CXX_PYTHON_RULES_NAMESPACE::serialize_read_binary_generic(
    const std::filesystem::path&)
{
  no_serialization();
}
std::shared_ptr<GenericObject>
CXX_PYTHON_RULES_NAMESPACE::serialize_read_generic_string(std::string_view)
{
  no_serialization();
}
std::shared_ptr<GenericObject>
CXX_PYTHON_RULES_NAMESPACE::serialize_read_binary_generic_string(
    std::string_view)
{
  no_serialization();
}
std::shared_ptr<GenericObject>
CXX_PYTHON_RULES_NAMESPACE::serialize_read_binary_generic_string(
    std::string_view, const std::filesystem::path&)
{
  no_serialization();
}
#endif
