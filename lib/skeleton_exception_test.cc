#include "unit_test_support.h"
#include "skeleton_exception.h"
#include <iomanip>
#include <string_view>

using namespace Skeleton;

BOOST_FIXTURE_TEST_SUITE(skeleton_exception, GlobalFixture)

BOOST_AUTO_TEST_CASE(basic)
{
  Exception e("x");
  BOOST_CHECK_EQUAL(std::string(e.what()), "x");
  Exception e2;
  BOOST_CHECK_EQUAL(std::string(e2.what()), "");
  e2 << "Value " << 1 << " " << 2.5;
  BOOST_CHECK_EQUAL(std::string(e2.what()), "Value 1 2.5");
  Exception e3("start ");
  e3 << "more";
  BOOST_CHECK_EQUAL(std::string(e3.what()), "start more");
}

BOOST_AUTO_TEST_CASE(copy)
{
  Exception e("a message");
  Exception e2(e);
  BOOST_CHECK_EQUAL(std::string(e2.what()), "a message");
  e2 << " changed";
  BOOST_CHECK_EQUAL(std::string(e.what()), "a message");
  BOOST_CHECK_EQUAL(std::string(e2.what()), "a message changed");
}

BOOST_AUTO_TEST_CASE(print)
{
  Exception e("x");
  BOOST_CHECK_EQUAL(e.print_to_string(), "Skeleton Exception:\n"
                                         "=========================\n"
                                         "x\n"
                                         "=========================\n");
}

BOOST_AUTO_TEST_CASE(throw_catch)
{
  try {
    Exception e;
    e << "Thrown " << 3;
    throw e;
  } catch(const std::exception& e) {
    BOOST_CHECK_EQUAL(std::string(e.what()), "Thrown 3");
  }
  BOOST_CHECK_THROW(throw Exception("x"), Exception);
}

BOOST_AUTO_TEST_CASE(range_checks)
{
  BOOST_CHECK_NO_THROW(range_check(1, 0, 2));
  BOOST_CHECK_THROW(range_check(2, 0, 2), Exception);
  BOOST_CHECK_THROW(range_check(-1, 0, 2), Exception);
  BOOST_CHECK_NO_THROW(range_check_inclusive(2, 0, 2));
  BOOST_CHECK_THROW(range_check_inclusive(3, 0, 2), Exception);
  BOOST_CHECK_NO_THROW(range_min_check(0, 0));
  BOOST_CHECK_THROW(range_min_check(-1, 0), Exception);
  BOOST_CHECK_NO_THROW(range_max_check(1, 2));
  BOOST_CHECK_THROW(range_max_check(2, 2), Exception);
  // The error reports the caller's line, both in the message and in
  // source_location().
  unsigned int line = std::source_location::current().line() + 2;
  try {
    range_check(5, 0, 2);
    BOOST_FAIL("range_check should have thrown");
  } catch(const Exception& e) {
    BOOST_CHECK_EQUAL(e.source_location().line(), line);
    BOOST_CHECK(std::string(e.what()).find(std::format("at line {}\n", line)) !=
                std::string::npos);
    std::string w = e.what();
    BOOST_CHECK(w.find("Out of range error in file") == 0);
    BOOST_CHECK(w.find("skeleton_exception_test.cc") != std::string::npos);
    BOOST_CHECK(w.find("Value:           5\n") != std::string::npos);
    BOOST_CHECK(w.find("Minimum allowed: 0\n") != std::string::npos);
    BOOST_CHECK(w.find("Maximum allowed: 2") != std::string::npos);
  }
}

BOOST_AUTO_TEST_CASE(range_check_types)
{
  // T comes from the value; the limits convert to it.
  std::size_t n = 3;
  BOOST_CHECK_NO_THROW(range_check(n, 0, 10));
  BOOST_CHECK_THROW(range_check(n, 0, 3), Exception);
  // The one-sided checks keep their trailing newline.
  try {
    range_min_check(-1, 0);
    BOOST_FAIL("range_min_check should have thrown");
  } catch(const Exception& e) {
    std::string w = e.what();
    BOOST_CHECK(w.ends_with("Minimum allowed: 0\n"));
  }
}

BOOST_AUTO_TEST_CASE(format_constructor)
{
  Exception e("x = {}, y = {:.2f}, s = {}", 1, 2.5, "str");
  BOOST_CHECK_EQUAL(std::string(e.what()), "x = 1, y = 2.50, s = str");
  // With no arguments the string is used as is, not as a format string.
  Exception e2("{} 100% literal");
  BOOST_CHECK_EQUAL(std::string(e2.what()), "{} 100% literal");
  // << still appends after the format constructor.
  e << " more";
  BOOST_CHECK_EQUAL(std::string(e.what()), "x = 1, y = 2.50, s = str more");
  try {
    throw Exception("Line {} out of range [0, {})", 10, 5);
  } catch(const std::exception& ex) {
    BOOST_CHECK_EQUAL(std::string(ex.what()), "Line 10 out of range [0, 5)");
  }
  // A Printable object as a format argument.
  Exception e3("{}", Exception("inner"));
  BOOST_CHECK_EQUAL(std::string(e3.what()),
                    Exception("inner").print_to_string());
}

BOOST_AUTO_TEST_CASE(manipulators)
{
  // Stream state carries over between <<, as on a stream.
  Exception e;
  e << std::setprecision(3) << 3.14159 << " " << 2.71828;
  BOOST_CHECK_EQUAL(std::string(e.what()), "3.14 2.72");
  Exception e2;
  e2 << std::hex << 255 << " " << 16 << std::dec << " " << 16;
  BOOST_CHECK_EQUAL(std::string(e2.what()), "ff 10 16");
  Exception e3;
  e3 << "[" << std::setw(4) << 7 << "]" << std::setfill('0') << std::setw(3)
     << 5;
  BOOST_CHECK_EQUAL(std::string(e3.what()), "[   7]005");
  // The defaults are a fresh stream's.
  Exception e4;
  e4 << 1.0 / 3 << " " << true;
  BOOST_CHECK_EQUAL(std::string(e4.what()), "0.333333 1");
}

BOOST_AUTO_TEST_CASE(source_location)
{
  int line = __LINE__ + 1;
  Exception e("x");
  BOOST_CHECK_EQUAL(e.source_location().line(), line);
  BOOST_CHECK(std::string_view(e.source_location().file_name())
                  .ends_with("skeleton_exception_test.cc"));
  line = __LINE__ + 1;
  Exception e2("x = {}", 1);
  BOOST_CHECK_EQUAL(e2.source_location().line(), line);
  BOOST_CHECK(std::string_view(e2.source_location().file_name())
                  .ends_with("skeleton_exception_test.cc"));
  line = __LINE__ + 1;
  Exception e3;
  BOOST_CHECK_EQUAL(e3.source_location().line(), line);
  // A copy keeps the original location.
  Exception e4(e2);
  BOOST_CHECK_EQUAL(e4.source_location().line(), e2.source_location().line());
}

BOOST_AUTO_TEST_CASE(stacktrace)
{
  BOOST_CHECK(Exception::capture_stacktrace());
  Exception e("x");
  std::string st = e.stacktrace();
  BOOST_CHECK(!st.empty());
  BOOST_CHECK(st.find(" 0# ") == 0);
  // A copy has the same stack trace.
  Exception e2(e);
  BOOST_CHECK_EQUAL(e2.stacktrace(), st);
  // Turned off, nothing is captured. Restore the default afterwards.
  Exception::capture_stacktrace(false);
  Exception e3("x");
  Exception::capture_stacktrace(true);
  BOOST_CHECK_EQUAL(e3.stacktrace(), "");
  BOOST_CHECK(!Exception("y").stacktrace().empty());
}

BOOST_AUTO_TEST_SUITE_END()
