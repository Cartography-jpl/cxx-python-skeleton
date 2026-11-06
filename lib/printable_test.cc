#include "unit_test_support.h"
#include "printable.h"
#include "generic_object_sample.h"
#include "director_example.h"
#include "generic_object_map.h"
#include <format>

using namespace Skeleton;

BOOST_FIXTURE_TEST_SUITE(printable, GlobalFixture)

class Test : public Printable<Test> {
public:
  void print(std::ostream& Os) const {Os << "Test";}
};

// Output shorter than 4 characters. print_to_string used to start
// from a "blah" seed string and write over it, so this came out as
// "Alah".
class ShortTest : public Printable<ShortTest> {
public:
  void print(std::ostream& Os) const {Os << "A";}
};

class EmptyTest : public Printable<EmptyTest> {
public:
  void print(std::ostream&) const {}
};

// desc() overrides print, which is how python subclasses change
// printing.
class DescTest : public Printable<DescTest> {
public:
  void print(std::ostream& Os) const {Os << "print";}
  std::string desc() const override { return "desc"; }
};

// Derived from a Printable class, not from Printable<DerivedTest>
// itself. "<<" and std::format both use Test's print.
class DerivedTest : public Test {
};

template<class T> std::string stream_string(const T& V)
{
  std::ostringstream os;
  os << V;
  return os.str();
}

BOOST_AUTO_TEST_CASE(basic_test)
{
  Test t;
  BOOST_CHECK_EQUAL(stream_string(t), "Test");
  BOOST_CHECK_EQUAL(t.print_to_string(), "Test");
}

BOOST_AUTO_TEST_CASE(short_output)
{
  ShortTest t;
  BOOST_CHECK_EQUAL(stream_string(t), "A");
  BOOST_CHECK_EQUAL(t.print_to_string(), "A");
  EmptyTest t2;
  BOOST_CHECK_EQUAL(stream_string(t2), "");
  BOOST_CHECK_EQUAL(t2.print_to_string(), "");
}

BOOST_AUTO_TEST_CASE(desc_override)
{
  DescTest t;
  BOOST_CHECK_EQUAL(stream_string(t), "desc");
  BOOST_CHECK_EQUAL(t.print_to_string(), "desc");
}

BOOST_AUTO_TEST_CASE(sample_classes)
{
  // operator<< and print_to_string agree, with the exact text. These
  // are the strings python's str() gives too (see printing_test.py).
  auto a = boost::make_shared<GenericObjectA>(1);
  BOOST_CHECK_EQUAL(a->print_to_string(), "A");
  BOOST_CHECK_EQUAL(stream_string(*a), "A");
  GenericObjectB b(a);
  BOOST_CHECK_EQUAL(b.print_to_string(), "B");
  BOOST_CHECK_EQUAL(stream_string(b), "B");
  GenericObjectB2 b2(*a);
  BOOST_CHECK_EQUAL(b2.print_to_string(), "B2");
  BOOST_CHECK_EQUAL(stream_string(b2), "B2");
  GenericObjectC c(a);
  BOOST_CHECK_EQUAL(c.print_to_string(), "C");
  BOOST_CHECK_EQUAL(stream_string(c), "C");
  GenericObjectMap m;
  BOOST_CHECK_EQUAL(m.print_to_string(), "GenericObjectMap");
  BOOST_CHECK_EQUAL(stream_string(m), "GenericObjectMap");
}

BOOST_AUTO_TEST_CASE(format)
{
  // std::format gives the same text as "<<".
  Test t;
  BOOST_CHECK_EQUAL(std::format("{}", t), stream_string(t));
  BOOST_CHECK_EQUAL(std::format("{}", ShortTest()), "A");
  BOOST_CHECK_EQUAL(std::format("{}", EmptyTest()), "");
  BOOST_CHECK_EQUAL(std::format("{}", DescTest()), "desc");
  DerivedTest d;
  BOOST_CHECK_EQUAL(std::format("{}", d), stream_string(d));
  BOOST_CHECK_EQUAL(std::format("{}", d), "Test");
  auto a = boost::make_shared<GenericObjectA>(1);
  BOOST_CHECK_EQUAL(std::format("{}", *a), stream_string(*a));
  GenericObjectMap m;
  BOOST_CHECK_EQUAL(std::format("{}", m), stream_string(m));
  // The std::string options work: width, fill, alignment.
  BOOST_CHECK_EQUAL(std::format("[{:>6}]", t), "[  Test]");
  BOOST_CHECK_EQUAL(std::format("[{:*<6}]", t), "[Test**]");
  BOOST_CHECK_EQUAL(std::format("{} and {}", t, ShortTest()), "Test and A");
  // A class that isn't Printable isn't picked up by the formatter.
  static_assert(PrintableObject<Test>);
  static_assert(PrintableObject<DerivedTest>);
  static_assert(!PrintableObject<std::string>);
  static_assert(!PrintableObject<int>);
}

BOOST_AUTO_TEST_SUITE_END()
