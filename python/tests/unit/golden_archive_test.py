# Check that we can read the golden archives in unit_test_data/golden through
# read_shelve. See the README.md there, and lib/golden_archive_test.cc for
# the C++ equivalent (which also covers the blitz arrays).
from pathlib import Path

import pytest
from skeleton.swig_shelve import read_shelve
from skeleton_swig.array_sample import ArraySample
from skeleton_swig.director_example import (
    DirectorExample,
    DirectorExampleNested,
    DirectorExampleUser,
    DirectorExampleWeakPtr,
)
from skeleton_swig.generic_object_map import GenericObjectMap
from skeleton_swig.generic_object_sample import (
    GenericObjectA,
    GenericObjectB,
    GenericObjectB2,
    GenericObjectC,
)

golden = Path(__file__).resolve().parents[3] / "unit_test_data" / "golden"


@pytest.fixture(params=["xml", "bin"])
def ext(request):
    return request.param


def get(name, ext):
    return read_shelve(str(golden / f"{name}.{ext}"))


def test_generic_objects(ext):
    a = get("generic_object_a", ext)
    assert isinstance(a, GenericObjectA)
    assert a.v == 7
    b = get("generic_object_b", ext)
    assert isinstance(b, GenericObjectB)
    assert b.a.v == 7
    b2 = get("generic_object_b2", ext)
    assert isinstance(b2, GenericObjectB2)
    assert b2.a.v == 8
    c = get("generic_object_c", ext)
    assert isinstance(c, GenericObjectC)
    assert c.a is None


def test_generic_object_map(ext):
    m = get("generic_object_map", ext)
    assert isinstance(m, GenericObjectMap)
    assert sorted(m.keys()) == ["a", "b", "b2", "c"]
    assert m.a.v == 1
    assert m.b2.a.v == 2
    # The shared A is one C++ object, check by changing it
    m.a.v = 5
    assert m.b.a.v == 5
    assert m.c.a.v == 5


def test_director_example(ext):
    e1 = get("director_example", ext)
    assert isinstance(e1, DirectorExample)
    assert e1.name() == "e1"
    assert e1.func(10) == 15
    e2 = get("director_example_nested", ext)
    assert isinstance(e2, DirectorExampleNested)
    assert e2.func(10) == 16
    assert e2.obj.name() == "e1"


def test_director_example_map(ext):
    m = get("director_example_map", ext)
    assert m.e1.func(10) == 15
    assert m.e2.obj.name() == "e1"
    assert isinstance(m.weak, DirectorExampleWeakPtr)
    assert m.weak.a().name() == "e1"
    m.e1.value_to_add(3)
    assert m.e2.obj.func(10) == 13
    assert m.weak.a().func(10) == 13


def test_director_example_user(ext):
    u = get("director_example_user", ext)
    assert isinstance(u, DirectorExampleUser)
    assert u.value().name() == "e1"
    assert [v.name() for v in u.value_vec()] == ["e1", "e2"]
    assert [[v.name() for v in vv] for vv in u.value_vec_vec()] == [
        ["e1"],
        ["e2", "e1"],
    ]
    assert list(u.value_vec_int()) == [1, 2, 3]
    assert [list(v) for v in u.value_vec_vec_int()] == [[4], [5, 6]]


def test_array_sample(ext):
    """Written with boost::array/boost::optional (S3.1), see the C++ test."""
    a = get("array_sample", ext)
    assert isinstance(a, ArraySample)
    assert list(a.position_) == [0.5, -1.25, 3e8]
    assert a.matrix.tolist() == [[1.5, -2, 3], [4, 5, -6.5]]
    assert a.count == 42
    assert a.scale == 2.5
    e = get("array_sample_empty", ext)
    assert e.count is None
    assert e.scale is None
    assert list(e.position_) == [1, 2, 3]


def test_observer(ext):
    """S5 samples, see the C++ test."""
    m = get("observer_map", ext)
    assert m.observable.value == 2.5
    assert m.observable.number_observer == 3
    assert m.observable.number_cache_observer == 1
    m.observable.value = 4
    assert m.ctor.notify_count == 2
    assert m.plain.notify_count == 2
    assert m.cache.value == 8
    c = get("observer_ctor", ext)
    assert c.name == "ctor"
    assert c.observed.number_observer == 2


def test_multi_base(ext):
    cam = get("simple_camera_like", ext)
    assert cam.focal_length == 10
    assert cam.number_band == 3
    d = get("diamond", ext)
    assert d.root_name() == "Diamond d"
    assert d.parameter_sum() == 3
