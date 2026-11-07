#include "unit_test_support.h"
#include "generic_object_sample.h"
#include "generic_object_map.h"
#include "file_reference_sample.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

// Tests for the file based entry points in
// cxx_python_rules/lib/serialize_function.cc (XML, binary, .gz on both, the
// archive base directory while reading).

using namespace Skeleton;

// The helpers are only used by the serialization tests.
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
namespace {
// Scratch directory, removed at the end of the test.
class TempDir {
public:
  TempDir()
  {
    p = std::filesystem::temp_directory_path() /
        ("skeleton_serialize_test_" + std::to_string(::getpid()));
    std::filesystem::remove_all(p);
    std::filesystem::create_directories(p / "sub");
  }
  ~TempDir() { std::filesystem::remove_all(p); }
  std::string file(const std::string& F) const { return (p / F).string(); }
  std::filesystem::path p;
};

std::shared_ptr<GenericObjectMap> sample_map()
{
  auto m = std::make_shared<GenericObjectMap>();
  (*m)["a"] = std::make_shared<GenericObjectA>(3);
  (*m)["b"] = std::make_shared<GenericObjectB>(m->get<GenericObjectA>("a"));
  (*m)["c"] = std::make_shared<GenericObjectC>(m->get<GenericObjectA>("a"));
  return m;
}

void check_map(const std::shared_ptr<GenericObjectMap>& M)
{
  BOOST_REQUIRE(M);
  auto a = M->get<GenericObjectA>("a");
  BOOST_CHECK_EQUAL(a->v(), 3);
  BOOST_CHECK(M->get<GenericObjectB>("b")->a().get() == a.get());
  BOOST_CHECK(M->get<GenericObjectC>("c")->a().get() == a.get());
}

bool is_gz_name(const std::string& F)
{
  return F.ends_with(".gz");
}

bool is_gzip(const std::string& F)
{
  std::ifstream is(F, std::ios::binary);
  unsigned char b[2] = {0, 0};
  is.read(reinterpret_cast<char*>(b), 2);
  return b[0] == 0x1f && b[1] == 0x8b;
}
}
#endif

BOOST_FIXTURE_TEST_SUITE(serialize_function, GlobalFixture)

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
BOOST_AUTO_TEST_CASE(xml_file)
{
  TempDir d;
  for(std::string f : {"sub/t.xml", "sub/t.xml.gz"}) {
    serialize_write(d.file(f), sample_map());
    BOOST_CHECK_EQUAL(is_gzip(d.file(f)), is_gz_name(f));
    check_map(serialize_read<GenericObjectMap>(d.file(f)));
  }
  // Plain XML is readable text
  std::ifstream is(d.file("sub/t.xml"));
  std::string line;
  std::getline(is, line);
  BOOST_CHECK(line.find("<?xml") == 0);
}

BOOST_AUTO_TEST_CASE(binary_file)
{
  TempDir d;
  for(std::string f : {"sub/t.bin", "sub/t.bin.gz"}) {
    serialize_write_binary(d.file(f), sample_map());
    BOOST_CHECK_EQUAL(is_gzip(d.file(f)), is_gz_name(f));
    check_map(serialize_read_binary<GenericObjectMap>(d.file(f)));
  }
}

BOOST_AUTO_TEST_CASE(string_round_trip)
{
  check_map(serialize_read_string<GenericObjectMap>(
      serialize_write_string(sample_map())));
  check_map(serialize_read_binary_string<GenericObjectMap>(
      serialize_write_binary(sample_map())));
}

BOOST_AUTO_TEST_CASE(missing_file)
{
  TempDir d;
  BOOST_CHECK_THROW(serialize_read_generic(d.file("not_here.xml")),
                    std::exception);
  BOOST_CHECK_THROW(serialize_read_binary_generic(d.file("not_here.bin")),
                    std::exception);
}

BOOST_AUTO_TEST_CASE(directory_restored)
{
  // Reading sets the archive base directory to the archive's directory
  // (so relative paths in the archive are relative to it) without
  // changing the current directory, and restores the base directory,
  // also when the read fails.
  TempDir d;
  auto cwd = std::filesystem::current_path();
  BOOST_CHECK(archive_base_directory().empty());
  serialize_write(d.file("sub/t.xml"), sample_map());
  check_map(serialize_read<GenericObjectMap>(d.file("sub/t.xml")));
  BOOST_CHECK(std::filesystem::current_path() == cwd);
  {
    std::ofstream os(d.file("sub/bad.xml"));
    os << "<?xml version=\"1.0\"?>\n<not_a_boost_archive/>\n";
  }
  BOOST_CHECK_THROW(serialize_read_generic(d.file("sub/bad.xml")),
                    std::exception);
  BOOST_CHECK(std::filesystem::current_path() == cwd);
  {
    // Valid header, truncated body. This fails after the change of
    // directory.
    std::string s = serialize_write_string(sample_map());
    std::ofstream os(d.file("sub/truncated.xml"));
    os << s.substr(0, s.size() / 2);
  }
  BOOST_CHECK_THROW(serialize_read_generic(d.file("sub/truncated.xml")),
                    std::exception);
  BOOST_CHECK(std::filesystem::current_path() == cwd);
  BOOST_CHECK(archive_base_directory().empty());
}

BOOST_AUTO_TEST_CASE(archive_base_directory_scope)
{
  BOOST_CHECK(resolve_archive_path("a.txt") == "a.txt");
  {
    ArchiveBaseDirectory b("/data/x");
    BOOST_CHECK(resolve_archive_path("a.txt") == "/data/x/a.txt");
    BOOST_CHECK(resolve_archive_path("/abs/a.txt") == "/abs/a.txt");
    {
      // Nested scopes restore the outer one.
      ArchiveBaseDirectory b2("/data/y");
      BOOST_CHECK(resolve_archive_path("a.txt") == "/data/y/a.txt");
    }
    BOOST_CHECK(resolve_archive_path("a.txt") == "/data/x/a.txt");
    // Per thread: another thread sees no base directory.
    std::filesystem::path other;
    std::thread([&] { other = archive_base_directory(); }).join();
    BOOST_CHECK(other.empty());
  }
  BOOST_CHECK(archive_base_directory().empty());
}

BOOST_AUTO_TEST_CASE(relative_file_in_archive)
{
  // An object that reads a file when loaded finds it relative to the
  // archive, even with two threads loading from two directories at once.
  // (With the old chdir both threads changed the one current directory.)
  TempDir d;
  std::vector<std::filesystem::path> archives;
  for(std::string dir : {"d1", "d2"}) {
    std::filesystem::create_directories(d.file(dir));
    std::ofstream(d.file(dir + "/f.txt")) << dir << " contents\n";
    auto cwd = std::filesystem::current_path();
    std::filesystem::current_path(d.file(dir));
    auto t = std::make_shared<FileReferenceSample>("f.txt");
    std::filesystem::current_path(cwd);
    archives.push_back(d.file(dir + "/t.xml"));
    serialize_write(archives.back(), t);
  }
  for(int rep = 0; rep < 20; ++rep) {
    std::vector<std::string> got(2);
    std::vector<std::jthread> threads;
    for(std::size_t i = 0; i < 2; ++i)
      threads.emplace_back([&, i] {
        got[i] = serialize_read<FileReferenceSample>(archives[i])->contents();
      });
    threads.clear();
    BOOST_CHECK_EQUAL(got[0], "d1 contents");
    BOOST_CHECK_EQUAL(got[1], "d2 contents");
  }
}

BOOST_AUTO_TEST_CASE(write_unopenable_file)
{
  // A write to a file that can't be opened throws, naming the file.
  BOOST_CHECK_EXCEPTION(serialize_write("/nonexistent_dir/x.xml", sample_map()),
                        std::runtime_error, [](const std::runtime_error& e) {
                          return std::string(e.what()).find(
                                     "/nonexistent_dir/x.xml") !=
                                 std::string::npos;
                        });
}

BOOST_AUTO_TEST_CASE(skip_weak_ptr)
{
  // Normally a first pass marks every object in the archive, so a
  // weak_ptr is kept if its target is anywhere in the archive. With
  // skip_weak_ptr_handling(true) there is no first pass. Objects still
  // mark themselves as they are written (GenericObjectA::serialize calls
  // add_ptr_serialized_reference), so a weak_ptr is kept only if its
  // target was written *earlier* in the same archive. GenericObjectMap
  // writes in key order: "a" before "c", but "_c" before "a".
  auto m = std::make_shared<GenericObjectMap>();
  (*m)["a"] = std::make_shared<GenericObjectA>(3);
  (*m)["c"] = std::make_shared<GenericObjectC>(m->get<GenericObjectA>("a"));
  (*m)["_c"] = std::make_shared<GenericObjectC>(m->get<GenericObjectA>("a"));
  BOOST_CHECK(!skip_weak_ptr_handling());
  auto mr = serialize_read_string<GenericObjectMap>(serialize_write_string(m));
  BOOST_CHECK(mr->get<GenericObjectC>("c")->a());
  BOOST_CHECK(mr->get<GenericObjectC>("_c")->a());
  skip_weak_ptr_handling(true);
  BOOST_CHECK(skip_weak_ptr_handling());
  mr = serialize_read_string<GenericObjectMap>(serialize_write_string(m));
  skip_weak_ptr_handling(false);
  BOOST_CHECK(mr->get<GenericObjectC>("c")->a());
  BOOST_CHECK(!mr->get<GenericObjectC>("_c")->a());
}
#endif

BOOST_AUTO_TEST_SUITE_END()
