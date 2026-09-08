import ctypes
import gc
import io
import pickle
import sys
import tempfile
from pathlib import Path

import pytest

from skeleton import (
    DirectorExample,
    DirectorExampleNested,
    DirectorExampleUser,
    DirectorExampleWeakPtr,
    GenericObjectMap,
    PicklerWithSerializationSupport,
    SerializeSessionLock,
    UnpicklerWithSerializationSupport,
    Vector_DirectorExample,
    read_shelve,
    write_shelve,
)
import skeleton

# For testing, this calls the low level python c API for decreasing the reference
# count. Can use to try to sort out issues with lifetime
_decref = ctypes.pythonapi.Py_DecRef
_decref.argtypes = [ctypes.py_object]
_decref.restype = None


class DirectorPythonExample(DirectorExample):
    # Normally we don't want a mutable as a class attribute. But this
    # is a special case for unit testing, so tell ruff this is ok.
    deleted_list = set()  # noqa: RUF012

    def __init__(self, v1, v2, name):
        super().__init__(v1, name)
        self.v2 = v2
        print(f"Creating Python {self.name()}", flush=True)

    def __del__(self):
        self.deleted_list.add(id(self))
        print(f"Destroying Python {self.name()}", flush=True)

    def print_ref_count(self):
        # We have an extra 2 counts because this got passed as an
        # arg to getrefcount, and we got this as a arg.
        print(f"Ref count {self.name()} {sys.getrefcount(self) - 2}")

    def func(self, v):
        return super().func(v) + 2 * self.v2

    def desc(self):
        """Description to print for this object."""
        return f"""DirectorPythonExample:
   name:                {self.name()}
   ref_count:           {sys.getrefcount(self) - 2}
   value_to_add:        {self.value_to_add()}
   value_to_double_add: {self.v2}"""

    def print_obj():
        """Print a list of active DirectorPythonExample, and the first one if it
        exists. Useful when sorting through lifetimes issues."""
        olist = [
            obj for obj in gc.get_objects() if isinstance(obj, DirectorPythonExample)
        ]
        print(f"Found in active objects: {olist}", flush=True)
        if len(olist) > 0:
            print("First object in active objects:")
            if olist[0].this is not None:
                print(olist[0])
                print(gc.get_referents(olist[0].this), flush=True)
                print(sys.getrefcount(olist[0].this))
            else:
                print(sys.getrefcount(olist[0]))
            print("Stuff referring to this:")
            print(gc.get_referents(olist[0]), flush=True)


class DirectorPythonExampleWithException(DirectorPythonExample):
    def func(self, v):
        raise RuntimeError("Error, check if propagate")
        return super().func(v) + 2 * self.v2


class DirectorPythonExampleNest(DirectorPythonExample):
    def __init__(self, v1, v2, obj1, obname):
        super().__init__(v1, v2, obname)
        self.v2 = v2
        self.obj1 = obj1
        self.obj1_dup = obj1
        print(f"Creating Python {self.name()}", flush=True)


class DirectorPythonExampleNest2(DirectorPythonExample):
    def __init__(self, v1, v2, obj1, obj2, obname):
        super().__init__(v1, v2, obname)
        self.v2 = v2
        self.obj1 = obj1
        self.obj2 = obj2
        print(f"Creating Python {self.name()}", flush=True)


class PythonObjExample:
    def __init__(self, objname):
        self.objname = objname

    def same_object(self, obj2):
        return id(self) == id(obj2)


class DirectorPythonExampleBadPickle(DirectorPythonExample):
    """We have had an issue with serialization where problems get reported as
    a generic "Python error that I can't parse". We will first have a error case that
    cleanly triggers this, and then work through how to get more information available
    here."""

    def __init__(self, v1, v2, name):
        super().__init__(v1, v2, name)
        # A file handle can't be pickled, so this will trigger an error
        self.fh = tempfile.TemporaryFile()  # noqa: SIM115


def test_director_example(serial):
    print(
        "May want to valgrind this, to make sure we have lifetime etc. ok with memory usage."
    )
    print(
        "Do this with PYTHONMALLOC=malloc valgrind $(which python) $(which pytest) -s tests/unit/director_example_test.py"
    )
    print(
        "(Note PYTHONMALLOC is important, otherwise you get a ton of spurious errors)"
    )
    DirectorPythonExample.deleted_list.clear()
    e1 = DirectorExample(5, "e1")
    assert e1.func(10) == 5 + 10
    e1 = None
    e2 = DirectorPythonExample(5, 6, "e2")
    assert e2.func(10) == 5 + 10 + 2 * 6
    euser = DirectorExampleUser()
    euser.set_value(e2)
    e2 = None
    assert euser.apply_func(10) == 5 + 10 + 2 * 6
    print(euser.value())
    # Test a simple std::vector
    euser.set_vec_int([1, 2, 3])
    print(euser.value_vec_int())
    if True:
        # Test a std::vector of boost::shared_ptr
        v = [
            DirectorExample(5, "v1"),
            DirectorExample(6, "v2"),
            DirectorPythonExample(5, 6, "vp3"),
            DirectorPythonExample(10, 7, "vp4"),
        ]
    else:
        # Directly use std::vector. Shouldn't be needed any longer, passing
        # in a list works now. But leave this around in case we run into a
        # future issue we need to diagnose
        v = Vector_DirectorExample()
        v.push_back(DirectorExample(5, "v1"))
        v.push_back(DirectorExample(6, "v2"))
        v.push_back(DirectorPythonExample(5, 6, "vp3"))
        v.push_back(DirectorPythonExample(10, 7, "vp4"))
    euser.set_vec(v)
    v = None
    assert euser.apply_func(10, 0) == 5 + 10
    assert euser.apply_func(10, 1) == 6 + 10
    assert euser.apply_func(10, 2) == 5 + 10 + 2 * 6
    assert euser.apply_func(10, 3) == 10 + 10 + 2 * 7
    print(euser.value_vec()[0])
    print(euser.value_vec()[1])
    print(euser.value_vec()[2])
    print(euser.value_vec()[3])
    assert euser.value_vec()[0].__class__ == DirectorExample
    assert euser.value_vec()[1].__class__ == DirectorExample
    assert euser.value_vec()[2].__class__ == DirectorPythonExample
    assert euser.value_vec()[3].__class__ == DirectorPythonExample
    print("Should see e2 and v1 through vp4 deleted", flush=True)
    assert len(DirectorPythonExample.deleted_list) == 0
    euser = None
    assert len(DirectorPythonExample.deleted_list) == 3
    DirectorPythonExample.deleted_list.clear()
    print("All Done", flush=True)
    euser = DirectorExampleUser()
    v2 = [
        [DirectorExample(5, "v1"), DirectorPythonExample(5, 6, "vp2")],
        [
            DirectorExample(6, "v2"),
            DirectorPythonExample(10, 7, "vp4"),
            DirectorPythonExample(11, 7, "vp5"),
        ],
    ]
    euser.set_vec_vec(v2)
    v2 = None
    print(euser.value_vec_vec())
    assert euser.value_vec_vec()[0][0].__class__ == DirectorExample
    assert euser.value_vec_vec()[0][1].__class__ == DirectorPythonExample
    assert euser.value_vec_vec()[1][0].__class__ == DirectorExample
    assert euser.value_vec_vec()[1][1].__class__ == DirectorPythonExample
    assert euser.value_vec_vec()[1][2].__class__ == DirectorPythonExample
    assert euser.apply_func(10, 0, 0) == 5 + 10
    assert euser.apply_func(10, 1, 0) == 6 + 10
    assert euser.apply_func(10, 0, 1) == 5 + 10 + 2 * 6
    assert euser.apply_func(10, 1, 1) == 10 + 10 + 2 * 7
    assert euser.apply_func(10, 1, 2) == 10 + 11 + 2 * 7
    print("Should see v1 through vp5 deleted", flush=True)
    assert len(DirectorPythonExample.deleted_list) == 0
    euser = None
    assert len(DirectorPythonExample.deleted_list) == 3
    DirectorPythonExample.deleted_list.clear()
    print("All Done", flush=True)


def test_director_pickle_shelve(isolated_dir, serial):
    print(
        "May want to valgrind this, to make sure we have lifetime etc. ok with memory usage."
    )
    print(
        "Do this with PYTHONMALLOC=malloc valgrind $(which python) $(which pytest) -s tests/unit/director_example_test.py"
    )
    print(
        "(Note PYTHONMALLOC is important, otherwise you get a ton of spurious errors)"
    )
    e1 = DirectorExample(5, "e1")
    assert e1.func(10) == 5 + 10
    # This is tiny, but go ahead and compress just to test the interface
    write_shelve("e1.xml.gz", e1)
    e1_s = read_shelve("e1.xml.gz")
    with open("e1.pkl", "wb") as fh:
        pickle.dump(e1, fh)
    e1_p = pickle.loads(Path("e1.pkl").read_bytes())
    e1 = None
    assert e1_s.func(10) == 5 + 10
    assert e1_p.func(10) == 5 + 10
    print(e1_s)
    print(e1_p)
    e1_s = None
    e1_p = None
    e2 = DirectorPythonExample(5, 6, "e2")
    assert e2.func(10) == 5 + 10 + 2 * 6
    write_shelve("e2.xml", e2)
    e2_s = read_shelve("e2.xml")
    with open("e2.pkl", "wb") as fh:
        pickle.dump(e2, fh)
    e2_p = pickle.loads(Path("e2.pkl").read_bytes())
    assert len(DirectorPythonExample.deleted_list) == 0
    e2 = None
    assert len(DirectorPythonExample.deleted_list) == 1
    DirectorPythonExample.deleted_list.clear()
    assert e2_s.func(10) == 5 + 10 + 2 * 6
    assert e2_p.func(10) == 5 + 10 + 2 * 6
    print(e2_s)
    print(e2_p)
    assert len(DirectorPythonExample.deleted_list) == 0
    e2_s = None
    e2_p = None
    assert len(DirectorPythonExample.deleted_list) == 2
    DirectorPythonExample.deleted_list.clear()

    euser = DirectorExampleUser()
    v = [
        DirectorExample(5, "v1"),
        DirectorExample(6, "v2"),
        DirectorPythonExample(5, 6, "vp3"),
        DirectorPythonExample(10, 7, "vp4"),
    ]
    euser.set_vec(v)
    v = None
    write_shelve("euser.xml", euser)
    euser_s = read_shelve("euser.xml")
    with open("euser.pkl", "wb") as fh:
        pickle.dump(euser, fh)
    euser_p = pickle.loads(Path("euser.pkl").read_bytes())
    assert len(DirectorPythonExample.deleted_list) == 0
    euser = None
    assert len(DirectorPythonExample.deleted_list) == 2
    DirectorPythonExample.deleted_list.clear()
    assert euser_s.apply_func(10, 0) == 5 + 10
    assert euser_s.apply_func(10, 1) == 6 + 10
    assert euser_s.apply_func(10, 2) == 5 + 10 + 2 * 6
    assert euser_s.apply_func(10, 3) == 10 + 10 + 2 * 7
    assert euser_p.apply_func(10, 0) == 5 + 10
    assert euser_p.apply_func(10, 1) == 6 + 10
    assert euser_p.apply_func(10, 2) == 5 + 10 + 2 * 6
    assert euser_p.apply_func(10, 3) == 10 + 10 + 2 * 7
    print(euser_s.value_vec()[0])
    print(euser_s.value_vec()[1])
    print(euser_s.value_vec()[2])
    print(euser_s.value_vec()[3])
    print(euser_p.value_vec()[0])
    print(euser_p.value_vec()[1])
    print(euser_p.value_vec()[2])
    print(euser_p.value_vec()[3])
    assert len(DirectorPythonExample.deleted_list) == 0
    euser_s = None
    euser_p = None
    assert len(DirectorPythonExample.deleted_list) == 4
    DirectorPythonExample.deleted_list.clear()
    print("All done", flush=True)


def test_director_pickle_error(isolated_dir, serial):
    e1 = DirectorPythonExampleBadPickle(5, 6, "e2")
    assert e1.func(10) == 5 + 10 + 2 * 6
    with pytest.raises(BaseException) as e_info, open("e1.pkl", "wb") as fh:
        pickle.dump(e1, fh)
    print(e_info.value)


def test_exception_propagate():
    # Check that when we throw an exception in python called by other code
    # through a director, we get back the original exception
    e2 = DirectorPythonExampleWithException(5, 6, "e2")
    euser = DirectorExampleUser()
    euser.set_value(e2)
    with pytest.raises(RuntimeError) as e_info:
        euser.apply_func(10)
    assert str(e_info.value) == "Error, check if propagate"


def test_director_lifetime_shelve(isolated_dir, serial):
    DirectorPythonExample.deleted_list.clear()
    print("Should see creation message", flush=True)
    e1 = DirectorPythonExample(5, 6, "e1")
    e1_id = id(e1)
    write_shelve("e1.xml", e1)
    print("Should see deletion message", flush=True)
    assert e1_id not in DirectorPythonExample.deleted_list
    e1 = None
    assert e1_id in DirectorPythonExample.deleted_list
    DirectorPythonExample.deleted_list.clear()
    print("All done with e1", flush=True)
    e1_s = read_shelve("e1.xml")
    e1_s_id = id(e1_s)
    print(e1_s)
    print("Should see deletion message", flush=True)
    assert e1_s_id not in DirectorPythonExample.deleted_list
    e1_s = None
    assert e1_s_id in DirectorPythonExample.deleted_list
    DirectorPythonExample.deleted_list.clear()
    print("All done with e1_s", flush=True)
    print("Should see creation message", flush=True)
    euser = DirectorExampleUser()
    euser.set_vec(
        [
            DirectorPythonExample(5, 6, "e2"),
        ]
    )
    # We can't easily get the id DirectorPythonExample, because it
    # gets copied when we return it. So just count the number of
    # items in the deletion set, that should be pretty accurate
    # e2_vec_id = id(euser.value_vec()[0])
    write_shelve("euser.xml", euser)
    print("Should see deletion message", flush=True)
    assert len(DirectorPythonExample.deleted_list) == 0
    euser = None
    assert len(DirectorPythonExample.deleted_list) == 1
    DirectorPythonExample.deleted_list.clear()
    print("All done with euser")
    euser_s = read_shelve("euser.xml")
    print(euser_s.value_vec()[0])
    print("Should see deletion message", flush=True)
    assert len(DirectorPythonExample.deleted_list) == 0
    euser_s = None
    assert len(DirectorPythonExample.deleted_list) == 1
    DirectorPythonExample.deleted_list.clear()
    print("All done with euser_s")


def test_director_example_object_tracking(isolated_dir, serial):
    """Check that both the C++ objects and python objects correctly handle
    duplicates in pickling and shelving"""
    # Check C++
    e1 = DirectorExample(5, "e1")
    e1_2 = DirectorExampleNested(5, "e1_2", e1)
    e1_3 = DirectorExampleNested(5, "e1_3", e1)
    e1_nest = DirectorPythonExampleNest2(5, 6, e1_2, e1_3, "e1_nest")
    assert e1_nest.obj1.obj.same_object(e1_nest.obj2.obj)
    # Check python
    p = PythonObjExample("e2_python")
    e2_1 = DirectorPythonExampleNest(5, 6, p, "e2_1")
    e2_2 = DirectorPythonExampleNest(5, 6, p, "e2_2")
    e2_nest = DirectorPythonExampleNest2(5, 6, e2_1, e2_2, "e2_nest")
    assert not e2_nest.obj1.same_object(e2_nest.obj2)
    assert e2_nest.obj1.obj1.same_object(e2_nest.obj2.obj1)

    # Test pickle C++
    with PickleWithSwigSupport() as p, open("e1_nest.pkl", "wb") as fh:
        pickle.dump(e1_nest, fh)
    with PickleWithSwigSupport() as p:
        e1_nest_p = pickle.loads(Path("e1_nest.pkl").read_bytes())
    # assert e1_nest_p.obj1.obj.same_object(e1_nest_p.obj2.obj)

    # Test pickle python
    with PickleWithSwigSupport() as p, open("e2_nest.pkl", "wb") as fh:
        pickle.dump(e2_nest, fh)
    with PickleWithSwigSupport() as p:
        e2_nest_p = pickle.loads(Path("e2_nest.pkl").read_bytes())
    assert not e2_nest_p.obj1.same_object(e2_nest_p.obj2)
    # assert e2_nest_p.obj1.obj1.same_object(e2_nest_p.obj2.obj1)

    # Test shelve C++
    write_shelve("e1_nest.xml", e1_nest)
    e1_nest_s = read_shelve("e1_nest.xml")
    # assert e1_nest_s.obj1.obj.same_object(e1_nest_s.obj2.obj)

    # Test shelve python
    write_shelve("e2_nest.xml", e2_nest)
    e2_nest_s = read_shelve("e2_nest.xml")
    assert not e2_nest_s.obj1.same_object(e2_nest_s.obj2)
    # assert e2_nest_s.obj1.obj1.same_object(e2_nest_s.obj2.obj1)


@pytest.mark.skip
def test_director_example_weak_ptr(isolated_dir, serial):
    m = GenericObjectMap()
    m["e1"] = DirectorPythonExample(5, 6, "e1")
    m["e1_weak"] = DirectorExampleWeakPtr(m.e1)
    print(m.e1)
    print(m.e1_weak)
    assert m.e1.func(10) == 5 + 10 + 2 * 6
    assert m.e1_weak.a().func(10) == 5 + 10 + 2 * 6
    write_shelve("e1.xml", m)
    m_s = read_shelve("e1.xml")
    assert m_s.e1.func(10) == 5 + 10 + 2 * 6
    assert m_s.e1_weak.a().func(10) == 5 + 10 + 2 * 6

    # Check that m_s.b.a and m_s.a are the same. We can't directly compare, python
    # has two different shared ptr so the are "different". But we can verify
    # that the underlying C++ object is the same by changing one and making
    # sure the other is changed also
    m_s.e1.value_to_add(3)
    assert m_s.e1.func(10) == 3 + 10 + 2 * 6
    assert m_s.e1_weak.a().func(10) == 3 + 10 + 2 * 6
    m_s.e1.v2 = 7
    assert m_s.e1.func(10) == 3 + 10 + 2 * 7
    assert m_s.e1_weak.a().func(10) == 3 + 10 + 2 * 7

    # Do the same, but without using a GenericObjectMap. In this case,
    # the weak_ptr in C should be set to None.
    a = DirectorPythonExample(5, 6, "e1")
    c = DirectorExampleWeakPtr(a)
    write_shelve("a.xml", a)
    a2 = read_shelve("a.xml")
    write_shelve("c.xml", c)
    c2 = read_shelve("c.xml")
    assert a2.func(10) == 5 + 10 + 2 * 6
    assert c2.a() is None

    # Also check handling of weak_ptr
    assert c.a() is not None
    assert not c.expired()
    assert c.use_count() == 1
    a = None
    assert c.expired()
    assert c.use_count() == 0
    assert c.a() is None

class SampleClass:
    def __init__(self, val):
        self.val = val

def test_pickle_with_serialization_support(isolated_dir):
    t = SampleClass(5)
    tp = skeleton.loads(skeleton.dumps(t))
    assert tp.val == t.val
    with open("sample.pkl", "wb") as fh:
        p = PicklerWithSerializationSupport(fh)
        s = None
        try:
            s = SerializeSessionLock(p, None)
            p.dump(t)
            sdump = p.serialization_dumps(t)
        finally:
            if s is not None:
                s.clear()
    with open("sample.pkl", "rb") as fh:
        up = UnpicklerWithSerializationSupport(fh)
        s = None
        try:
            s = SerializeSessionLock(None, up)
            tp = up.load()
            tp2 = up.serialization_loads(sdump)
        finally:
            if s is not None:
                s.clear()
    assert tp.val == t.val
    assert tp2.val == t.val
    assert id(t) != id(tp)
    assert id(tp) == id(tp2)

    p = PythonObjExample("e2_python")
    e2_1 = DirectorPythonExampleNest(5, 6, p, "e2_1")
    e2_2 = DirectorPythonExampleNest(5, 6, p, "e2_2")
    e2_nest = DirectorPythonExampleNest2(5, 6, e2_1, e2_2, "e2_nest")
    assert not e2_nest.obj1.same_object(e2_nest.obj2)
    assert e2_nest.obj1.obj1.same_object(e2_nest.obj2.obj1)
    with open("e2_nest.pkl", "wb") as fh:
        p = PicklerWithSerializationSupport(fh)
        s = None
        try:
            s = SerializeSessionLock(p, None)
            p.dump(e2_nest)
        finally:
            if s is not None:
                s.clear()
    
    with open("e2_nest.pkl", "rb") as fh:
        up = UnpicklerWithSerializationSupport(fh)
        s = None
        try:
            s = SerializeSessionLock(None, up)
            e2_nest_p = up.load()
        finally:
            if s is not None:
                s.clear()
    assert not e2_nest_p.obj1.same_object(e2_nest_p.obj2)
    assert e2_nest_p.obj1.obj1.same_object(e2_nest_p.obj2.obj1)
    
