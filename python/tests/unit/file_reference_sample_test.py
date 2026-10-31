# Tests for %pickle_serialization_dir (lib/file_reference_sample.h), the
# downcast of shared_ptr<GenericObject> to the most derived python type,
# and pickling through multiprocessing.
import multiprocessing
import os
import pickle
from pathlib import Path

import pytest

from skeleton import (
    DirectorExample,
    FileReferenceSample,
    GenericObjectA,
    GenericObjectB,
    GenericObjectB2,
    GenericObjectC,
    GenericObjectMap,
    generic_object_index,
    generic_object_list,
)


def test_pickle_serialization_dir(isolated_dir):
    """A pickle of an object with a relative file name can be loaded from a
    different directory: %pickle_serialization_dir records the directory it
    was pickled in, and loads from there."""
    os.mkdir("data")
    Path("data/f.txt").write_text("file contents\n")
    os.chdir("data")
    t = FileReferenceSample("f.txt")
    assert t.contents == "file contents"
    d = pickle.dumps(t)
    os.chdir("..")
    t2 = pickle.loads(d)
    assert t2.file_name == "f.txt"
    assert t2.contents == "file contents"
    assert os.path.basename(os.getcwd()) != "data"
    with pytest.raises(RuntimeError):
        FileReferenceSample("f.txt")


def test_downcast():
    """A C++ function returning shared_ptr<GenericObject> gives python the
    most derived type, and methods work on it."""
    expect = [GenericObjectA, GenericObjectB, GenericObjectB2, GenericObjectC]
    for i, cls in enumerate(expect):
        t = generic_object_index(i)
        assert type(t) is cls
    lst = generic_object_list()
    assert [type(t) for t in lst] == expect
    assert lst[0].v == 1
    assert lst[1].a.v == 1
    assert lst[2].a.v == 1
    # C holds a weak_ptr to the same A, which is still alive (lst[0] and
    # B hold it)
    assert lst[3].a.v == 1


# multiprocessing pickles the arguments and results. geocal uses this in
# production (parallel_process.py).
def _work(obj):
    if isinstance(obj, GenericObjectMap):
        return obj.a.v + obj.b.a.v
    return obj.func(10)


class DirectorPythonMp(DirectorExample):
    def __init__(self, v1, v2, name):
        super().__init__(v1, name)
        self.v2 = v2

    def func(self, v):
        return super().func(v) + self.v2


@pytest.mark.parametrize("method", ["fork", "spawn"])
def test_multiprocessing(method):
    m = GenericObjectMap()
    m["a"] = GenericObjectA(3)
    m["b"] = GenericObjectB(m.a)
    jobs = [m, DirectorExample(1, "e1"), DirectorPythonMp(1, 100, "e2")]
    ctx = multiprocessing.get_context(method)
    with ctx.Pool(2) as pool:
        res = pool.map(_work, jobs)
    assert res == [6, 11, 111]


def _return_object(i):
    m = GenericObjectMap()
    m["a"] = GenericObjectA(i)
    m["b"] = GenericObjectB(m.a)
    return m


def test_multiprocessing_return():
    """C++ objects created in the worker come back, with sharing kept."""
    with multiprocessing.get_context("spawn").Pool(2) as pool:
        res = pool.map(_return_object, [1, 2])
    assert [r.a.v for r in res] == [1, 2]
    res[0].a.v = 5
    assert res[0].b.a.v == 5
