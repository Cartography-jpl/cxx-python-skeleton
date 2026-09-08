import pickle

from skeleton import (
    GenericObjectA,
    GenericObjectB,
    GenericObjectB2,
    GenericObjectC,
    GenericObjectMap,
)


def test_generic_object_map():
    m = GenericObjectMap()
    m["a"] = GenericObjectA(1)
    m["b"] = GenericObjectB(m.a)
    m["b2"] = GenericObjectB2(m.a)
    t = pickle.dumps(m)
    m2 = pickle.loads(t)
    assert m2.a.v == 1
    assert m2.b.a.v == 1
    assert m2.b2.a.v == 1
    # Check that m.b.a and m.a are the same. We can't directly compare, python
    # has two different shared ptr so the are "different". But we can verify
    # that the underlying C++ object is the same by changing one and making
    # sure the other is changed also
    m2.a.v = 2
    assert m2.a.v == 2
    assert m2.b.a.v == 2

    # Do the same, but without using a GenericObjectMap. In this case, the
    # object should really be different.
    a = GenericObjectA(1)
    b = GenericObjectB(a)
    t = pickle.dumps(a)
    a2 = pickle.loads(t)
    t = pickle.dumps(b)
    b2 = pickle.loads(t)
    assert a2.v == 1
    assert b2.a.v == 1
    a2.v = 2
    assert a2.v == 2
    assert b2.a.v == 1


def test_generic_object_map_weak_ptr():
    """Repeat previous test, but use weak_ptr and make sure this gets
    properly handled"""
    m = GenericObjectMap()
    m["a"] = GenericObjectA(1)
    m["c"] = GenericObjectC(m.a)
    t = pickle.dumps(m)
    m2 = pickle.loads(t)
    assert m2.a.v == 1
    assert m2.c.a.v == 1
    # Check that m.b.a and m.a are the same. We can't directly compare, python
    # has two different shared ptr so the are "different". But we can verify
    # that the underlying C++ object is the same by changing one and making
    # sure the other is changed also
    m2.a.v = 2
    assert m2.a.v == 2
    assert m2.c.a.v == 2

    # Do the same, but without using a GenericObjectMap. In this case,
    # the weak_ptr in C should be set to None.
    a = GenericObjectA(1)
    c = GenericObjectC(a)
    t = pickle.dumps(a)
    a2 = pickle.loads(t)
    t = pickle.dumps(c)
    c2 = pickle.loads(t)
    assert a2.v == 1
    assert c2.a is None
