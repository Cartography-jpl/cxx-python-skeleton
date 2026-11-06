#include "unit_test_support.h"
#include "observer_example.h"
#include "generic_object_map.h"

using namespace Skeleton;

BOOST_FIXTURE_TEST_SUITE(observer, GlobalFixture)

namespace {
// An observer kept as a member of another class.
class HasMemberObserver {
public:
  HasMemberObserver(ObservableExample& O) : obs("member")
  {
    O.add_observer(obs);
  }
  ObserverExample obs;
};
}

BOOST_AUTO_TEST_CASE(notification)
{
  ObservableExample o(1);
  ObserverExample stack_obs("stack");
  o.add_observer(stack_obs);
  HasMemberObserver h(o);
  auto sp_obs = std::make_shared<ObserverExample>("shared_ptr");
  o.add_observer(*sp_obs);
  BOOST_CHECK_EQUAL(o.number_observer(), 3);
  BOOST_CHECK_EQUAL(stack_obs.add_count(), 1);
  o.value(2);
  for(const ObserverExample* p : {&stack_obs, &h.obs, sp_obs.get()}) {
    BOOST_CHECK_EQUAL(p->notify_count(), 1);
    BOOST_CHECK_EQUAL(p->last_value(), 2);
  }
  // Adding again does nothing
  o.add_observer(stack_obs);
  BOOST_CHECK_EQUAL(o.number_observer(), 3);
  BOOST_CHECK_EQUAL(stack_obs.add_count(), 1);
  o.remove_observer(stack_obs);
  BOOST_CHECK_EQUAL(stack_obs.remove_count(), 1);
  o.value(3);
  BOOST_CHECK_EQUAL(stack_obs.notify_count(), 1);
  BOOST_CHECK_EQUAL(sp_obs->notify_count(), 2);
  o.clear_observers();
  BOOST_CHECK_EQUAL(o.number_observer(), 0);
  BOOST_CHECK_EQUAL(sp_obs->remove_count(), 1);
}

BOOST_AUTO_TEST_CASE(dead_observer)
{
  ObservableExample o;
  {
    ObserverExample obs("temporary");
    o.add_observer(obs);
    BOOST_CHECK_EQUAL(o.number_observer(), 1);
  }
  BOOST_CHECK_EQUAL(o.number_observer(), 0);
  // Notifying with a dead entry in the list is fine.
  o.value(5);
  // The observer can also outlive the observable.
  ObserverExample obs("outlives");
  {
    ObservableExample o2;
    o2.add_observer(obs);
  }
  BOOST_CHECK_EQUAL(obs.add_count(), 1);
}

BOOST_AUTO_TEST_CASE(keep_reference)
{
  ObservableExample o;
  std::weak_ptr<ObserverExample> w;
  {
    auto obs = std::make_shared<ObserverExample>("kept");
    w = obs;
    std::shared_ptr<Observer<ObservableExample>> p = obs;
    o.add_observer_and_keep_reference(p);
  }
  BOOST_REQUIRE(!w.expired());
  o.value(4);
  BOOST_CHECK_EQUAL(w.lock()->notify_count(), 1);
}

BOOST_AUTO_TEST_CASE(constructor_registration)
{
  auto o = std::make_shared<ObservableExample>(1);
  ObserverExample obs("ctor", o);
  BOOST_CHECK_EQUAL(o->number_observer(), 1);
  BOOST_CHECK_EQUAL(obs.add_count(), 1);
  o->value(7);
  BOOST_CHECK_EQUAL(obs.notify_count(), 1);
  BOOST_CHECK_EQUAL(obs.last_value(), 7);
}

BOOST_AUTO_TEST_CASE(cache_observer)
{
  auto o = std::make_shared<ObservableExample>(1);
  CacheExample c(o);
  BOOST_CHECK_EQUAL(o->number_cache_observer(), 1);
  BOOST_CHECK_EQUAL(c.value(), 2);
  BOOST_CHECK_EQUAL(c.value(), 2);
  BOOST_CHECK_EQUAL(c.recompute_count(), 1);
  o->value(3);
  BOOST_CHECK_EQUAL(c.value(), 6);
  BOOST_CHECK_EQUAL(c.recompute_count(), 2);
  o->add_cache_invalidated_observer(c);
  BOOST_CHECK_EQUAL(o->number_cache_observer(), 1);
  o->remove_cache_invalidated_observer(c);
  BOOST_CHECK_EQUAL(o->number_cache_observer(), 0);
}

namespace {
// An observer with enable_shared_from_this (GenericObject gets it in S6).
class SharedObserver : public ObserverExample,
                       public std::enable_shared_from_this<SharedObserver> {
public:
  SharedObserver() : ObserverExample("shared") {}
};
}

BOOST_AUTO_TEST_CASE(shared_from_this_owner)
{
  // this_obj (to_ptr in the Observer constructor) and to_ptr/const_pointer
  // must not claim weak_this: shared_from_this() returns the real owner.
  auto p = std::make_shared<SharedObserver>();
  auto np = to_ptr(*p);
  auto cp = const_pointer(*p);
  ObservableExample o;
  o.add_observer(*p);
  auto s = p->shared_from_this();
  BOOST_CHECK(!s.owner_before(p) && !p.owner_before(s));
  BOOST_CHECK_EQUAL(p.use_count(), 2);
  BOOST_CHECK(np.owner_before(p) || p.owner_before(np));
  BOOST_CHECK(cp.get() == p.get());
  // An object not owned by a shared_ptr has no shared_from_this.
  SharedObserver stack_obs;
  BOOST_CHECK(stack_obs.weak_from_this().expired());
  o.add_observer(stack_obs);
  o.value(1);
  BOOST_CHECK_EQUAL(stack_obs.notify_count(), 1);
  BOOST_CHECK(stack_obs.weak_from_this().expired());
}

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
BOOST_AUTO_TEST_CASE(serialization)
{
  // Observable alone: the observer isn't in the archive, so it is
  // dropped (two pass weak_ptr rule).
  auto o = std::make_shared<ObservableExample>(1);
  auto obs = std::make_shared<ObserverExample>("plain");
  o->add_observer(*obs);
  auto o2 = serialize_read_string<ObservableExample>(serialize_write_string(o));
  BOOST_CHECK_EQUAL(o2->value(), 1);
  BOOST_CHECK_EQUAL(o2->number_observer(), 0);

  // Both in a map: the connection is kept.
  auto m = std::make_shared<GenericObjectMap>();
  (*m)["o"] = o;
  (*m)["obs"] = obs;
  auto m2 = serialize_read_string<GenericObjectMap>(serialize_write_string(m));
  auto mo = m2->get<ObservableExample>("o");
  auto mobs = m2->get<ObserverExample>("obs");
  BOOST_REQUIRE(mo && mobs);
  BOOST_CHECK_EQUAL(mo->number_observer(), 1);
  mo->value(9);
  BOOST_CHECK_EQUAL(mobs->notify_count(), 1);
  BOOST_CHECK_EQUAL(mobs->last_value(), 9);

  // Observer that registered in its constructor: written on its own, the
  // Observable comes along and the observer registers again in load().
  auto o3 = std::make_shared<ObservableExample>(2);
  auto cobs = std::make_shared<ObserverExample>("ctor", o3);
  o3->value(3);
  auto cobs2 =
      serialize_read_string<ObserverExample>(serialize_write_string(cobs));
  BOOST_CHECK_EQUAL(cobs2->name(), "ctor");
  BOOST_CHECK_EQUAL(cobs2->notify_count(), 1);
  BOOST_CHECK_EQUAL(cobs2->add_count(), 1);
  BOOST_REQUIRE(cobs2->observed());
  BOOST_CHECK_EQUAL(cobs2->observed()->number_observer(), 1);
  cobs2->observed()->value(4);
  BOOST_CHECK_EQUAL(cobs2->notify_count(), 2);
  BOOST_CHECK_EQUAL(cobs2->last_value(), 4);

  // Observable read first, so the observer is read from inside its
  // olist and registers itself (load()) before its archive entry is read.
  // It must still be in the list only once.
  auto m3 = std::make_shared<GenericObjectMap>();
  (*m3)["a"] = o3;
  (*m3)["b"] = cobs;
  auto m4 = serialize_read_string<GenericObjectMap>(serialize_write_string(m3));
  auto mo3 = m4->get<ObservableExample>("a");
  auto mcobs = m4->get<ObserverExample>("b");
  BOOST_CHECK_EQUAL(mo3->number_observer(), 1);
  // Writing it again gives the same archive.
  BOOST_CHECK_EQUAL(serialize_write_string(m4), serialize_write_string(m3));
  mo3->value(6);
  BOOST_CHECK_EQUAL(mcobs->notify_count(), 2);

  // add_observer_and_keep_reference: the Observable owns the observer,
  // so it is written with it.
  auto o4 = std::make_shared<ObservableExample>(5);
  std::shared_ptr<Observer<ObservableExample>> kept =
      std::make_shared<ObserverExample>("kept");
  o4->add_observer_and_keep_reference(kept);
  auto o5 =
      serialize_read_string<ObservableExample>(serialize_write_string(o4));
  BOOST_CHECK_EQUAL(o5->number_observer(), 1);

  // Cache observer, with its observable.
  auto c = std::make_shared<CacheExample>(o3);
  BOOST_CHECK_EQUAL(c->value(), 6);
  auto c2 = serialize_read_string<CacheExample>(serialize_write_string(c));
  BOOST_CHECK_EQUAL(c2->observed()->number_cache_observer(), 1);
  BOOST_CHECK_EQUAL(c2->value(), 6);
  c2->observed()->value(10);
  BOOST_CHECK_EQUAL(c2->value(), 20);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
