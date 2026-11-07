#include "unit_test_support.h"
#include "skeleton_serialize_support.h"
#include "generic_object_sample.h"
#include "generic_object_map.h"
#include "director_example.h"
#include "array_sample.h"
#include "observer_example.h"
#include "multi_base_example.h"
#include "virtual_diamond_example.h"
#include "serialize_function.h"
#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
#include <boost/archive/polymorphic_xml_iarchive.hpp>
#include <boost/archive/polymorphic_binary_iarchive.hpp>
#endif
#include <fstream>
#include <iterator>
#include <utility>

// Check that we can read the archives in unit_test_data/golden. These
// were written by golden_archive_generator.cc, see the README.md in that
// directory.

using namespace Skeleton;

BOOST_FIXTURE_TEST_SUITE(golden_archive, GlobalFixture)

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
namespace {
template<class T>
std::shared_ptr<T> read_golden(const std::string& Dir, const std::string& Name,
                               bool Binary)
{
  std::string f = Dir + "golden/" + Name + (Binary ? ".bin" : ".xml");
  std::shared_ptr<T> r =
      Binary ? serialize_read_binary<T>(f) : serialize_read<T>(f);
  BOOST_REQUIRE(r);
  return r;
}

template<class T, int D>
void check_array(const std::string& Dir, const std::string& Name, bool Binary)
{
  std::string f = Dir + "golden/" + Name + (Binary ? ".bin" : ".xml");
  std::ifstream is(f, Binary ? std::ios::binary : std::ios::in);
  BOOST_REQUIRE(is.good());
  blitz::Array<T, D> a;
  if(Binary) {
    boost::archive::polymorphic_binary_iarchive ia(is);
    ia >> SKELETON_NVP2("array", a);
  } else {
    boost::archive::polymorphic_xml_iarchive ia(is);
    ia >> SKELETON_NVP2("array", a);
  }
  int sz = 1;
  for(int i = 0; i < D; ++i) {
    BOOST_CHECK_EQUAL(a.extent(i), (i == 0 ? 3 : 2));
    sz *= a.extent(i);
  }
  BOOST_REQUIRE_EQUAL(a.size(), sz);
  for(int i = 0; i < sz; ++i)
    BOOST_CHECK_EQUAL(a.data()[i], static_cast<T>(i % 2 == 0 ? i : -i));
}

template<int D>
void check_string_array(const std::string& Dir, const std::string& Name,
                        bool Binary)
{
  std::string f = Dir + "golden/" + Name + (Binary ? ".bin" : ".xml");
  std::ifstream is(f, Binary ? std::ios::binary : std::ios::in);
  BOOST_REQUIRE(is.good());
  blitz::Array<std::string, D> a;
  if(Binary) {
    boost::archive::polymorphic_binary_iarchive ia(is);
    ia >> SKELETON_NVP2("array", a);
  } else {
    boost::archive::polymorphic_xml_iarchive ia(is);
    ia >> SKELETON_NVP2("array", a);
  }
  BOOST_REQUIRE_EQUAL(a.size(), D == 1 ? 3 : 6);
  for(std::size_t i = 0; i < a.size(); ++i)
    BOOST_CHECK_EQUAL(a.data()[i], "s" + std::to_string(i));
}

template<class T>
void check_arrays(const std::string& Dir, const std::string& Type, bool Binary)
{
  std::string b = "blitz_" + Type + "_array_";
  check_array<T, 1>(Dir, b + "1d", Binary);
  check_array<T, 2>(Dir, b + "2d", Binary);
  check_array<T, 3>(Dir, b + "3d", Binary);
  check_array<T, 4>(Dir, b + "4d", Binary);
  check_array<T, 5>(Dir, b + "5d", Binary);
}
}

BOOST_AUTO_TEST_CASE(generic_objects)
{
  for(bool binary : {false, true}) {
    auto a = read_golden<GenericObjectA>(test_data_dir(), "generic_object_a",
                                         binary);
    BOOST_CHECK_EQUAL(a->v(), 7);
    auto b = read_golden<GenericObjectB>(test_data_dir(), "generic_object_b",
                                         binary);
    BOOST_CHECK_EQUAL(b->a()->v(), 7);
    auto b2 = read_golden<GenericObjectB2>(test_data_dir(), "generic_object_b2",
                                           binary);
    BOOST_CHECK_EQUAL(b2->a().v(), 8);
    auto c = read_golden<GenericObjectC>(test_data_dir(), "generic_object_c",
                                         binary);
    BOOST_CHECK(!c->a());
  }
}

BOOST_AUTO_TEST_CASE(array_sample)
{
  // Written with boost::array and boost::optional (S3.1); must keep
  // loading after the switch to std::array and std::optional.
  for(bool binary : {false, true}) {
    auto a = read_golden<ArraySample>(test_data_dir(), "array_sample", binary);
    BOOST_CHECK_EQUAL(a->position_[0], 0.5);
    BOOST_CHECK_EQUAL(a->position_[1], -1.25);
    BOOST_CHECK_EQUAL(a->position_[2], 3e8);
    blitz::Array<double, 2> m = a->matrix();
    BOOST_CHECK_EQUAL(m(0, 0), 1.5);
    BOOST_CHECK_EQUAL(m(0, 1), -2);
    BOOST_CHECK_EQUAL(m(0, 2), 3);
    BOOST_CHECK_EQUAL(m(1, 0), 4);
    BOOST_CHECK_EQUAL(m(1, 1), 5);
    BOOST_CHECK_EQUAL(m(1, 2), -6.5);
    // optional == value is false for an empty optional.
    BOOST_CHECK(a->count() == 42);
    BOOST_CHECK(a->scale() == 2.5);
    // Both empty; count is set in a default constructed ArraySample, so
    // this also checks that loading clears it.
    auto e =
        read_golden<ArraySample>(test_data_dir(), "array_sample_empty", binary);
    BOOST_CHECK(!e->count());
    BOOST_CHECK(!e->scale());
    BOOST_CHECK_EQUAL(e->position_[2], 3);
    BOOST_CHECK_EQUAL(e->matrix()(1, 2), 6);
    // Writing what we read gives the golden file byte for byte, so the
    // archive layout is unchanged too, not just readable.
    for(auto [name, obj] :
        {std::pair{"array_sample", a}, std::pair{"array_sample_empty", e}}) {
      std::string f =
          test_data_dir() + "golden/" + name + (binary ? ".bin" : ".xml");
      std::ifstream in(f, std::ios::binary);
      std::string golden((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
      std::string rewritten = binary ? serialize_write_binary_string(obj)
                                     : serialize_write_string(obj);
      BOOST_CHECK_MESSAGE(rewritten == golden, f << " rewritten differs");
    }
  }
}

BOOST_AUTO_TEST_CASE(generic_object_map)
{
  for(bool binary : {false, true}) {
    auto m = read_golden<GenericObjectMap>(test_data_dir(),
                                           "generic_object_map", binary);
    BOOST_CHECK_EQUAL(m->size(), 4u);
    auto a = m->get<GenericObjectA>("a");
    auto b = m->get<GenericObjectB>("b");
    auto c = m->get<GenericObjectC>("c");
    auto b2 = m->get<GenericObjectB2>("b2");
    BOOST_REQUIRE(a && b && c && b2);
    BOOST_CHECK_EQUAL(a->v(), 1);
    BOOST_CHECK(b->a().get() == a.get());
    BOOST_CHECK(c->a().get() == a.get());
    BOOST_CHECK_EQUAL(b2->a().v(), 2);
  }
}

BOOST_AUTO_TEST_CASE(director_example)
{
  for(bool binary : {false, true}) {
    auto e1 = read_golden<DirectorExample>(test_data_dir(), "director_example",
                                           binary);
    BOOST_CHECK_EQUAL(e1->name(), "e1");
    BOOST_CHECK_EQUAL(e1->func(10), 15);
    auto e2 = read_golden<DirectorExampleNested>(
        test_data_dir(), "director_example_nested", binary);
    BOOST_CHECK_EQUAL(e2->name(), "e2");
    BOOST_CHECK_EQUAL(e2->func(10), 16);
    BOOST_CHECK_EQUAL(e2->obj()->name(), "e1");
    auto m = read_golden<GenericObjectMap>(test_data_dir(),
                                           "director_example_map", binary);
    auto m1 = m->get<DirectorExample>("e1");
    auto m2 = m->get<DirectorExampleNested>("e2");
    auto mw = m->get<DirectorExampleWeakPtr>("weak");
    BOOST_REQUIRE(m1 && m2 && mw);
    BOOST_CHECK(m2->obj().get() == m1.get());
    BOOST_CHECK(mw->a().get() == m1.get());
    auto u = read_golden<DirectorExampleUser>(test_data_dir(),
                                              "director_example_user", binary);
    BOOST_CHECK_EQUAL(u->value()->name(), "e1");
    BOOST_REQUIRE_EQUAL(u->value_vec().size(), 2u);
    BOOST_CHECK_EQUAL(u->value_vec()[1]->name(), "e2");
    BOOST_REQUIRE_EQUAL(u->value_vec_vec().size(), 2u);
    BOOST_CHECK_EQUAL(u->value_vec_vec()[1].size(), 2u);
    BOOST_CHECK(u->value_vec_vec()[1][1].get() == u->value().get());
    BOOST_CHECK_EQUAL(u->value_vec_int().size(), 3u);
    BOOST_CHECK_EQUAL(u->value_vec_vec_int()[1][1], 6);
  }
}

BOOST_AUTO_TEST_CASE(observer)
{
  for(bool binary : {false, true}) {
    auto m =
        read_golden<GenericObjectMap>(test_data_dir(), "observer_map", binary);
    auto ob = m->get<ObservableExample>("observable");
    auto ctor = m->get<ObserverExample>("ctor");
    auto plain = m->get<ObserverExample>("plain");
    auto cache = m->get<CacheExample>("cache");
    BOOST_REQUIRE(ob && ctor && plain && cache);
    BOOST_CHECK_EQUAL(ob->value(), 2.5);
    // ctor, plain and kept; dropped wasn't in the archive.
    BOOST_CHECK_EQUAL(ob->number_observer(), 3);
    BOOST_CHECK_EQUAL(ob->number_cache_observer(), 1);
    BOOST_CHECK(ctor->observed().get() == ob.get());
    BOOST_CHECK(cache->observed().get() == ob.get());
    BOOST_CHECK_EQUAL(ctor->notify_count(), 1);
    BOOST_CHECK_EQUAL(plain->last_value(), 2.5);
    ob->value(4);
    BOOST_CHECK_EQUAL(ctor->notify_count(), 2);
    BOOST_CHECK_EQUAL(plain->notify_count(), 2);
    BOOST_CHECK_EQUAL(cache->value(), 8);
    auto c =
        read_golden<ObserverExample>(test_data_dir(), "observer_ctor", binary);
    BOOST_CHECK_EQUAL(c->name(), "ctor");
    BOOST_REQUIRE(c->observed());
    // Re-registered in load()
    BOOST_CHECK_EQUAL(c->observed()->number_observer(), 2);
    c->observed()->value(3);
    BOOST_CHECK_EQUAL(c->last_value(), 3);
  }
}

BOOST_AUTO_TEST_CASE(multi_base)
{
  for(bool binary : {false, true}) {
    auto cam = read_golden<SimpleCameraLike>(test_data_dir(),
                                             "simple_camera_like", binary);
    BOOST_CHECK_EQUAL(cam->focal_length(), 10);
    BOOST_CHECK_EQUAL(cam->number_band(), 3);
    auto d = read_golden<Diamond>(test_data_dir(), "diamond", binary);
    BOOST_CHECK_EQUAL(d->root_name(), "Diamond d");
    BOOST_CHECK_EQUAL(d->parameter_sum(), 3);
  }
}

BOOST_AUTO_TEST_CASE(rewrite_identical)
{
  // Every golden object archive, read and written again, gives the same
  // bytes. This is the S4 acceptance test (std::shared_ptr mustn't change
  // the archives), see claude_notes/implementation_plan.md §8.
  for(const char* name :
      {"generic_object_a", "generic_object_b", "generic_object_b2",
       "generic_object_c", "generic_object_map", "director_example",
       "director_example_nested", "director_example_map",
       "director_example_user", "array_sample", "array_sample_empty",
       "observer_map", "observer_ctor", "simple_camera_like", "diamond"}) {
    for(bool binary : {false, true}) {
      auto obj = read_golden<GenericObject>(test_data_dir(), name, binary);
      std::string f =
          test_data_dir() + "golden/" + name + (binary ? ".bin" : ".xml");
      std::ifstream in(f, std::ios::binary);
      std::string golden((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
      std::string rewritten = binary ? serialize_write_binary_string(obj)
                                     : serialize_write_string(obj);
      BOOST_CHECK_MESSAGE(rewritten == golden, f << " rewritten differs");
    }
  }
}

BOOST_AUTO_TEST_CASE(blitz_arrays)
{
  for(bool binary : {false, true}) {
    check_arrays<double>(test_data_dir(), "double", binary);
    check_arrays<int>(test_data_dir(), "int", binary);
    check_arrays<bool>(test_data_dir(), "bool", binary);
    check_arrays<float>(test_data_dir(), "float", binary);
    check_arrays<unsigned int>(test_data_dir(), "uint", binary);
    check_arrays<unsigned short int>(test_data_dir(), "ushort", binary);
    check_arrays<short int>(test_data_dir(), "short", binary);
    check_string_array<1>(test_data_dir(), "blitz_string_array_1d", binary);
    check_string_array<2>(test_data_dir(), "blitz_string_array_2d", binary);
  }
}
#endif

BOOST_AUTO_TEST_SUITE_END()
