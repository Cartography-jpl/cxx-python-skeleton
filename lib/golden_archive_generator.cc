// Writes the "golden" archives in unit_test_data/golden. These are
// written by the current code and then committed, so later phases of
// the modernization can check they still read archives written by the
// original boost::shared_ptr/boost::array code.
//
// Regenerate deliberately (see unit_test_data/golden/README.md):
//   make golden-archives
//
// The objects created here need to stay in sync with
// golden_archive_test.cc and python/tests/unit/golden_archive_test.py.

#include "skeleton_serialize_support.h"
#include "generic_object_sample.h"
#include "generic_object_map.h"
#include "director_example.h"
#include "array_sample.h"
#include "serialize_function.h"
#include <boost/make_shared.hpp>
#include <boost/archive/polymorphic_xml_oarchive.hpp>
#include <boost/archive/polymorphic_binary_oarchive.hpp>
#include <fstream>
#include <iostream>

using namespace Skeleton;

static std::string outdir;

static void write_object(const std::string& Name,
                         const boost::shared_ptr<GenericObject>& Obj)
{
  serialize_write(outdir + "/" + Name + ".xml", Obj);
  serialize_write_binary(outdir + "/" + Name + ".bin", Obj);
}

template<class T, int D>
static void write_array(const std::string& Name)
{
  blitz::TinyVector<int, D> shp;
  int sz = 1;
  for(int i = 0; i < D; ++i) {
    shp(i) = (i == 0 ? 3 : 2);
    sz *= shp(i);
  }
  blitz::Array<T, D> a(shp);
  for(int i = 0; i < sz; ++i)
    a.data()[i] = static_cast<T>(i % 2 == 0 ? i : -i);
  {
    std::ofstream os(outdir + "/" + Name + ".xml");
    boost::archive::polymorphic_xml_oarchive oa(os);
    oa << SKELETON_NVP2("array", a);
  }
  {
    std::ofstream os(outdir + "/" + Name + ".bin", std::ios::binary);
    boost::archive::polymorphic_binary_oarchive oa(os);
    oa << SKELETON_NVP2("array", a);
  }
}

template<int D>
static void write_array_string(const std::string& Name)
{
  blitz::TinyVector<int, D> shp;
  for(int i = 0; i < D; ++i)
    shp(i) = (i == 0 ? 3 : 2);
  blitz::Array<std::string, D> a(shp);
  for(std::size_t i = 0; i < a.size(); ++i)
    a.data()[i] = "s" + std::to_string(i);
  {
    std::ofstream os(outdir + "/" + Name + ".xml");
    boost::archive::polymorphic_xml_oarchive oa(os);
    oa << SKELETON_NVP2("array", a);
  }
  {
    std::ofstream os(outdir + "/" + Name + ".bin", std::ios::binary);
    boost::archive::polymorphic_binary_oarchive oa(os);
    oa << SKELETON_NVP2("array", a);
  }
}

template<class T>
static void write_arrays(const std::string& Type)
{
  write_array<T, 1>("blitz_" + Type + "_array_1d");
  write_array<T, 2>("blitz_" + Type + "_array_2d");
  write_array<T, 3>("blitz_" + Type + "_array_3d");
  write_array<T, 4>("blitz_" + Type + "_array_4d");
  write_array<T, 5>("blitz_" + Type + "_array_5d");
}

int main(int argc, char** argv)
{
  if(argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <output directory>\n";
    return 1;
  }
  if(!have_serialize_supported()) {
    std::cerr << "Built without boost serialization support\n";
    return 1;
  }
  outdir = argv[1];
  auto a = boost::make_shared<GenericObjectA>(7);
  write_object("generic_object_a", a);
  write_object("generic_object_b", boost::make_shared<GenericObjectB>(a));
  write_object("generic_object_b2",
               boost::make_shared<GenericObjectB2>(GenericObjectA(8)));
  // Weak pointer to something not otherwise serialized: comes back null
  write_object("generic_object_c", boost::make_shared<GenericObjectC>(a));

  // Shared and weak entries in a map, a and b share one A, c is
  // weak pointer to it.
  auto m = boost::make_shared<GenericObjectMap>();
  (*m)["a"] = boost::make_shared<GenericObjectA>(1);
  (*m)["b"] = boost::make_shared<GenericObjectB>(m->get<GenericObjectA>("a"));
  (*m)["c"] = boost::make_shared<GenericObjectC>(m->get<GenericObjectA>("a"));
  (*m)["b2"] = boost::make_shared<GenericObjectB2>(GenericObjectA(2));
  write_object("generic_object_map", m);

  // The C++ only DirectorExample objects
  auto e1 = boost::make_shared<DirectorExample>(5, "e1");
  write_object("director_example", e1);
  auto e2 = boost::make_shared<DirectorExampleNested>(6, "e2", e1);
  write_object("director_example_nested", e2);
  auto dm = boost::make_shared<GenericObjectMap>();
  (*dm)["e1"] = e1;
  (*dm)["e2"] = e2;
  (*dm)["weak"] = boost::make_shared<DirectorExampleWeakPtr>(e1);
  write_object("director_example_map", dm);
  auto u = boost::make_shared<DirectorExampleUser>();
  u->set_value(e1);
  u->set_vec({e1, e2});
  u->set_vec_vec({{e1}, {e2, e1}});
  u->set_vec_int({1, 2, 3});
  u->set_vec_vec_int({{4}, {5, 6}});
  write_object("director_example_user", u);

  write_arrays<double>("double");
  write_arrays<int>("int");
  write_arrays<bool>("bool");
  // Added 2026-10-30 (ST), once these types could be serialized. The
  // files above were not changed.
  write_arrays<float>("float");
  write_arrays<unsigned int>("uint");
  write_arrays<unsigned short int>("ushort");
  write_arrays<short int>("short");
  write_array_string<1>("blitz_string_array_1d");
  write_array_string<2>("blitz_string_array_2d");
  // Added 2026-11-05 (phase S3.1), written with boost::array and
  // boost::optional before converting to std::array and std::optional.
  // Values differ from the ArraySample defaults, so a load that reads
  // nothing would be caught.
  auto as = boost::make_shared<ArraySample>();
  as->position_ = {{0.5, -1.25, 3e8}};
  blitz::Array<double, 2> mat(2, 3);
  mat = 1.5, -2, 3,
        4, 5, -6.5;
  as->matrix(mat);
  as->count(42);
  as->scale(2.5);
  write_object("array_sample", as);
  auto ase = boost::make_shared<ArraySample>();
  ase->count(boost::none);
  ase->scale(boost::none);
  write_object("array_sample_empty", ase);
  return 0;
}
