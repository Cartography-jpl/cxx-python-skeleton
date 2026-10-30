#include "unit_test_support.h"
#include "skeleton_exception.h"

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
  BOOST_CHECK_EQUAL(e.print_to_string(),
		    "Skeleton Exception:\n"
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
  try {
    range_check(5, 0, 2);
    BOOST_FAIL("range_check should have thrown");
  } catch(const Exception& e) {
    std::string w = e.what();
    BOOST_CHECK(w.find("Out of range error in file") == 0);
    BOOST_CHECK(w.find("skeleton_exception_test.cc") != std::string::npos);
    BOOST_CHECK(w.find("Value:           5\n") != std::string::npos);
    BOOST_CHECK(w.find("Minimum allowed: 0\n") != std::string::npos);
    BOOST_CHECK(w.find("Maximum allowed: 2") != std::string::npos);
  }
}

BOOST_AUTO_TEST_SUITE_END()
