#include "unit_test_support.h"
#include "generic_object_map.h"
#include "generic_object_sample.h"
#include <memory>

using namespace Skeleton;

BOOST_FIXTURE_TEST_SUITE(generic_object_array, GlobalFixture)

BOOST_AUTO_TEST_CASE(serialization1)
{
  if(!have_serialize_supported())
    return;
  // Serialize GenericObjectA and GenericObjectB separately,
  // should end up with two copies of GenericObjectA.
  std::shared_ptr<GenericObjectA> a = std::make_shared<GenericObjectA>(1);
  std::shared_ptr<GenericObjectB> b = std::make_shared<GenericObjectB>(a);
  std::string d1 = serialize_write_string(a);
  std::string d2 = serialize_write_string(b);
  if(false)
    std::cerr << d1 << d2;
  std::shared_ptr<GenericObjectA> ar =
      serialize_read_string<GenericObjectA>(d1);
  std::shared_ptr<GenericObjectB> br =
      serialize_read_string<GenericObjectB>(d2);
  BOOST_CHECK_EQUAL(ar->v(), 1);
  BOOST_CHECK_EQUAL(br->a()->v(), 1);
  BOOST_CHECK(br->a().get() != ar.get());
}

BOOST_AUTO_TEST_CASE(serialization1_weakptr)
{
  if(!have_serialize_supported())
    return;
  // Serialize GenericObjectA and GenericObjectC separately,
  // GenericObjectC has a weak ptr to A, which shouldn't get serialized
  std::shared_ptr<GenericObjectA> a = std::make_shared<GenericObjectA>(1);
  std::shared_ptr<GenericObjectC> c = std::make_shared<GenericObjectC>(a);
  std::string d1 = serialize_write_string(a);
  std::string d2 = serialize_write_string(c);
  if(false)
    std::cerr << d1 << d2;
  std::shared_ptr<GenericObjectA> ar =
      serialize_read_string<GenericObjectA>(d1);
  std::shared_ptr<GenericObjectC> cr =
      serialize_read_string<GenericObjectC>(d2);
  BOOST_CHECK_EQUAL(ar->v(), 1);
  BOOST_CHECK(!cr->a());
}

BOOST_AUTO_TEST_CASE(serialization2)
{
  if(!have_serialize_supported())
    return;
  // Serialize GenericObjectA and GenericObjectB together, should
  // end up with one copy GenericObjectA.
  std::shared_ptr<GenericObjectMap> m = std::make_shared<GenericObjectMap>();
  (*m)["a"] = std::make_shared<GenericObjectA>(1);
  (*m)["b"] = std::make_shared<GenericObjectB>(m->get<GenericObjectA>("a"));
  std::string d = serialize_write_string(m);
  if(false)
    std::cerr << d;
  std::shared_ptr<GenericObjectMap> mr =
      serialize_read_string<GenericObjectMap>(d);
  std::shared_ptr<GenericObjectA> ar = mr->get<GenericObjectA>("a");
  std::shared_ptr<GenericObjectB> br = mr->get<GenericObjectB>("b");
  BOOST_CHECK_EQUAL(ar->v(), 1);
  BOOST_CHECK_EQUAL(br->a()->v(), 1);
  BOOST_CHECK(br->a().get() == ar.get());
}

BOOST_AUTO_TEST_CASE(serialization2_weak)
{
  if(!have_serialize_supported())
    return;
  // Serialize GenericObjectA and GenericObjectC together, should
  // end up with one copy GenericObjectA, and C should keep the
  // weak_ptr to a
  std::shared_ptr<GenericObjectMap> m = std::make_shared<GenericObjectMap>();
  (*m)["a"] = std::make_shared<GenericObjectA>(1);
  (*m)["c"] = std::make_shared<GenericObjectC>(m->get<GenericObjectA>("a"));
  std::string d = serialize_write_string(m);
  if(false)
    std::cerr << d;
  std::shared_ptr<GenericObjectMap> mr =
      serialize_read_string<GenericObjectMap>(d);
  std::shared_ptr<GenericObjectA> ar = mr->get<GenericObjectA>("a");
  std::shared_ptr<GenericObjectC> cr = mr->get<GenericObjectC>("c");
  BOOST_CHECK_EQUAL(ar->v(), 1);
  BOOST_CHECK(cr->a());
  BOOST_CHECK_EQUAL(cr->a()->v(), 1);
  BOOST_CHECK(cr->a().get() == ar.get());
}

BOOST_AUTO_TEST_SUITE_END()
