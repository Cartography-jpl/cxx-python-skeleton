# Behavior specific to the nanobind bindings (phase S6 of
# claude_notes/implementation_plan.md): the weak_ptr matrix (S6.1 item 4),
# docstrings from doxygen (S6.5), identity.
import gc

import pytest

from skeleton import (
    DirectorExample,
    DirectorExampleUser,
    DirectorExampleWeakPtr,
    GenericObjectA,
    GenericObjectC,
    GenericObjectMap,
    SimpleCameraLike,
)


class PyDirector(DirectorExample):
    def func(self, v):
        return 2 * v


def make(creator):
    if creator == "cpp":
        # Created by C++ (read from an archive, inside a map)
        m = GenericObjectMap()
        m["a"] = GenericObjectA(1)
        import pickle

        return pickle.loads(pickle.dumps(m)).a
    if creator == "python":
        return GenericObjectA(1)
    raise ValueError(creator)


# Expected-result matrix for a weak_ptr (GenericObjectC holds a weak_ptr to a
# GenericObjectA) set from python: (creator, a C++ owner exists) -> valid.
@pytest.mark.parametrize(
    ("creator", "cpp_owner", "valid"),
    [
        ("cpp", True, True),
        ("cpp", False, True),  # the python object holds the C++ shared_ptr
        ("python", True, True),
        # nb::new_ + make_shared: a python constructed plain class has a C++
        # control block, held by the python object.
        ("python", False, True),
    ],
)
def test_weak_ptr_matrix_plain(creator, cpp_owner, valid):
    a = make(creator)
    owner = None
    if cpp_owner:
        owner = GenericObjectMap()
        owner["a"] = a
    c = GenericObjectC(a)
    assert (c.a is not None) == valid
    if valid:
        assert c.a is a
    del owner


# Director classes (nb::init, so python subclasses work) and python
# subclasses: only valid while a C++ shared_ptr also owns the object.
@pytest.mark.parametrize("cls", [DirectorExample, PyDirector])
@pytest.mark.parametrize("cpp_owner", [True, False])
def test_weak_ptr_matrix_director(cls, cpp_owner):
    d = cls(1, "d")
    owner = None
    if cpp_owner:
        owner = DirectorExampleUser()
        owner.set_value(d)
    w = DirectorExampleWeakPtr(d)
    assert w.expired() == (not cpp_owner)
    if cpp_owner:
        assert w.a() is d
    del owner
    gc.collect()
    assert w.expired()


def test_director_kept_alive_by_cpp():
    """C++ keeps a python subclass object alive after python drops it."""
    u = DirectorExampleUser()
    u.set_value(PyDirector(0, "p"))
    gc.collect()
    assert u.apply_func(5) == 10
    assert type(u.value()) is PyDirector


def test_docstrings():
    """help() text comes from the doxygen comments in the C++ headers."""
    assert "Like geocal's Camera" in SimpleCameraLike.__mro__[1].__doc__
    assert "Sum of the parameters" in SimpleCameraLike.parameter_sum.__doc__
    assert (
        "Twice the observed value" in __import__("skeleton").CacheExample.value.__doc__
    )
