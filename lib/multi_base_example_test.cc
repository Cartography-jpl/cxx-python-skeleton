#include "unit_test_support.h"
#include "multi_base_example.h"
#include "virtual_diamond_example.h"
#include "observer_example.h"

using namespace Skeleton;

BOOST_FIXTURE_TEST_SUITE(multi_base_example, GlobalFixture)

namespace {
class CameraObserver : public Observer<CameraLike> {
public:
  void notify_update(const CameraLike& C) override { focal = C.focal_length(); }
  double focal = 0;
};
}

BOOST_AUTO_TEST_CASE(multi_base)
{
  auto cam = std::make_shared<SimpleCameraLike>(10, 3);
  MultiBaseExample m(cam);
  BOOST_CHECK(std::dynamic_pointer_cast<SimpleCameraLike>(m.camera()) == cam);
  BOOST_CHECK(std::dynamic_pointer_cast<SimpleCameraLike>(
                  m.camera_as_with_parameter()) == cam);
  BOOST_CHECK(std::dynamic_pointer_cast<SimpleCameraLike>(
                  m.camera_as_generic()) == cam);
  BOOST_CHECK(std::dynamic_pointer_cast<SimpleCameraLike>(
                  m.camera_as_observable()) == cam);
  // Cross cast between the two bases
  BOOST_CHECK(std::dynamic_pointer_cast<WithParameterLike>(
                  m.camera_as_observable()) == m.camera_as_with_parameter());
  BOOST_CHECK_EQUAL(MultiBaseExample::number_band(cam), 3);
  BOOST_CHECK_EQUAL(MultiBaseExample::parameter_sum(cam), 10);
  BOOST_CHECK_EQUAL(MultiBaseExample::parameter_sum_ref(*cam), 10);
  BOOST_CHECK_EQUAL(MultiBaseExample::type_name(cam),
                    "Skeleton::SimpleCameraLike");
  CameraObserver obs;
  cam->add_observer(obs);
  blitz::Array<double, 1> p(1);
  p = 20;
  cam->parameter(p);
  BOOST_CHECK_EQUAL(obs.focal, 20);
  BOOST_CHECK_EQUAL(cam->print_to_string(), "SimpleCameraLike 20 3");
}

BOOST_AUTO_TEST_CASE(virtual_diamond)
{
  auto d = std::make_shared<Diamond>("d", 1, 2);
  VirtualDiamondExample v(d);
  BOOST_CHECK(std::dynamic_pointer_cast<Diamond>(v.as_root()) == d);
  BOOST_CHECK(std::dynamic_pointer_cast<Diamond>(v.as_nested()) == d);
  BOOST_CHECK(std::dynamic_pointer_cast<Diamond>(v.as_with_parameter()) == d);
  BOOST_CHECK(std::dynamic_pointer_cast<Diamond>(v.as_generic()) == d);
  BOOST_CHECK_EQUAL(VirtualDiamondExample::root_name(v.as_root()), "Diamond d");
  BOOST_CHECK_EQUAL(VirtualDiamondExample::nested_name(v.as_nested()),
                    "Diamond nested d");
  BOOST_CHECK_EQUAL(VirtualDiamondExample::parameter_sum(v.as_nested()), 3);
  BOOST_CHECK_EQUAL(d->print_to_string(), "Diamond d");
}

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
BOOST_AUTO_TEST_CASE(serialization)
{
  std::shared_ptr<GenericObject> cam =
      std::make_shared<SimpleCameraLike>(10, 3);
  auto cam2 = std::dynamic_pointer_cast<SimpleCameraLike>(
      serialize_read_string<GenericObject>(serialize_write_string(cam)));
  BOOST_REQUIRE(cam2);
  BOOST_CHECK_EQUAL(cam2->focal_length(), 10);
  BOOST_CHECK_EQUAL(cam2->number_band(), 3);
  std::shared_ptr<GenericObject> d = std::make_shared<Diamond>("d", 1, 2);
  auto d2 = std::dynamic_pointer_cast<Diamond>(
      serialize_read_string<GenericObject>(serialize_write_string(d)));
  BOOST_REQUIRE(d2);
  BOOST_CHECK_EQUAL(d2->name(), "d");
  BOOST_CHECK_EQUAL(d2->parameter_sum(), 3);
  // Through a pointer to a virtual base too
  std::shared_ptr<RootLike> r = std::dynamic_pointer_cast<RootLike>(d);
  auto r2 = serialize_read_string<RootLike>(serialize_write_string(r));
  BOOST_CHECK_EQUAL(r2->root_name(), "Diamond d");
  std::shared_ptr<WithParameterLike> w =
      std::dynamic_pointer_cast<WithParameterLike>(d);
  auto w2 = serialize_read_string<WithParameterLike>(serialize_write_string(w));
  BOOST_CHECK_EQUAL(w2->parameter_sum(), 3);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
