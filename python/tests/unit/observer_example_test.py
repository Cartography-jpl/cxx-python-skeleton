# Observer/Observable (lib/observer.h), copied from framework's observer.h.
# Phase S5 of claude_notes/implementation_plan.md: these pinned down the
# behavior under SWIG, and pass under nanobind (S6).
import gc
import pickle

from skeleton import (
    CacheExample,
    GenericObjectMap,
    ObservableExample,
    ObserverExample,
    ObserverObservableExample,
)


class PyObserver(ObserverObservableExample):
    """Python subclass of the template instance Observer<ObservableExample>
    (a director on a template, as in framework)."""

    deleted = []  # noqa: RUF012

    def __init__(self, name):
        super().__init__()
        self.name = name
        self.values = []

    def notify_update(self, obj):
        self.values.append(obj.value)

    def __del__(self):
        self.deleted.append(self.name)


def test_python_constructed_observer():
    o = ObservableExample(1)
    obs = ObserverExample("a")
    o.add_observer(obs)
    assert o.number_observer == 1
    assert obs.add_count == 1
    o.value = 2
    assert obs.notify_count == 1
    assert obs.last_value == 2
    o.add_observer(obs)
    assert o.number_observer == 1
    o.remove_observer(obs)
    assert obs.remove_count == 1
    assert o.number_observer == 0


def test_dead_observer_dropped():
    o = ObservableExample(1)
    obs = ObserverExample("a")
    o.add_observer(obs)
    del obs
    gc.collect()
    assert o.number_observer == 0
    o.value = 3


def test_constructor_registration():
    o = ObservableExample(1)
    obs = ObserverExample("ctor", o)
    assert o.number_observer == 1
    o.value = 5
    assert obs.notify_count == 1
    assert obs.last_value == 5
    assert obs.observed.value == 5


def test_python_subclass_observer():
    o = ObservableExample(1)
    p = PyObserver("sub")
    o.add_observer(p)
    o.value = 2
    o.value = 3
    assert p.values == [2, 3]
    # Python owns p, the observable only has a weak reference.
    del p
    gc.collect()
    assert "sub" in PyObserver.deleted
    assert o.number_observer == 0
    o.value = 4


def test_python_subclass_keep_reference():
    o = ObservableExample(1)
    p = PyObserver("kept")
    o.add_observer_and_keep_reference(p)
    del p
    gc.collect()
    # The observable keeps the python object alive.
    assert "kept" not in PyObserver.deleted
    assert o.number_observer == 1
    o.value = 7
    # Dropping the observable releases it.
    del o
    gc.collect()
    assert "kept" in PyObserver.deleted


def test_cache_observer():
    o = ObservableExample(1)
    c = CacheExample(o)
    assert o.number_cache_observer == 1
    assert c.value == 2
    assert c.value == 2
    assert c.recompute_count == 1
    o.value = 3
    assert c.value == 6
    assert c.recompute_count == 2


def test_pickle():
    o = ObservableExample(1)
    obs = ObserverExample("ctor", o)
    o.value = 2
    # Pickled on its own, the observer brings its observable along and
    # registers with it again when loaded.
    obs2 = pickle.loads(pickle.dumps(obs))
    assert obs2.name == "ctor"
    assert obs2.notify_count == 1
    assert obs2.observed.number_observer == 1
    obs2.observed.value = 4
    assert obs2.notify_count == 2
    assert obs2.last_value == 4
    # The observable alone loses observers that aren't in the pickle.
    o2 = pickle.loads(pickle.dumps(o))
    assert o2.value == 2
    assert o2.number_observer == 0
    c2 = pickle.loads(pickle.dumps(CacheExample(o)))
    assert c2.value == 4


def test_pickle_map():
    """An observer registered with add_observer (not in its constructor)
    is kept if it is in the same archive."""
    o = ObservableExample(1)
    obs = ObserverExample("plain")
    o.add_observer(obs)
    m = GenericObjectMap()
    m.set_generic("o", o)
    m.set_generic("obs", obs)
    m2 = pickle.loads(pickle.dumps(m))
    assert m2.o.number_observer == 1
    m2.o.value = 9
    assert m2.obs.notify_count == 1
    assert m2.obs.last_value == 9
