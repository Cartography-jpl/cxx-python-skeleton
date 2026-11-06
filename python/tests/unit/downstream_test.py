# A second extension module using the main module's types
# (lib/downstream_nb.cc, claude_notes/implementation_plan.md S6.1 item 10).
import pickle

from skeleton_cxx import _skeleton_downstream as ds

from skeleton import GenericObjectA, GenericObjectB, GenericObjectMap


def test_downstream_creates_and_takes_main_types():
    a = ds.make_a(3)
    assert type(a) is GenericObjectA
    assert ds.a_value(a) == 3
    assert ds.a_value(GenericObjectA(4)) == 4


def test_downstream_class_derived_from_main_class():
    a = GenericObjectA(5)
    d = ds.DownstreamB(a, 7)
    assert isinstance(d, GenericObjectB)
    assert d.extra == 7
    assert d.a.v == 5
    # Through the main module's functions, as GenericObject and back
    m = GenericObjectMap()
    m["d"] = d
    assert m.d is d
    assert type(ds.as_generic(d)) is ds.DownstreamB
    assert type(ds.as_generic(GenericObjectA(1))) is GenericObjectA
    assert ds.as_generic(None) is None


def test_downstream_main_object_pickles():
    a2 = pickle.loads(pickle.dumps(ds.make_a(9)))
    assert a2.v == 9
