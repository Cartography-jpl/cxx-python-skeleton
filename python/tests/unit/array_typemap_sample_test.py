# Tests for blitz::Array <-> numpy in the bindings
# (cxx_python_rules/nanobind/blitz_array.h), using lib/array_typemap_sample.h.
# Written for the SWIG typemaps (swig_rules/include/swig_array.i) and kept
# for nanobind (claude_notes/implementation_plan.md S6.1 item 7). Where a test
# records what SWIG happened to do rather than a requirement, the comment
# says so; the nanobind bindings do the same.
import gc

import numpy as np
import pytest

from skeleton import ArrayTypemapSample

# (echo function, numpy dtype). All 9 element types the rules support.
ECHO_2D = [
    ("echo_double_2", np.float64),
    ("echo_float_2", np.float32),
    ("echo_int_2", np.int32),
    ("echo_uint_2", np.uint32),
    ("echo_bool_2", np.bool_),
    ("echo_short_2", np.int16),
    ("echo_ushort_2", np.uint16),
    ("echo_char_2", np.int8),
    ("echo_uchar_2", np.uint8),
]


@pytest.mark.parametrize(("fname", "dtype"), ECHO_2D)
def test_element_types(fname, dtype):
    t = ArrayTypemapSample()
    a = (
        (np.arange(6).reshape(2, 3) % 2 == 0)
        if dtype == np.bool_
        else np.arange(6, dtype=dtype).reshape(2, 3)
    )
    a = a.astype(dtype)
    r = getattr(t, fname)(a)
    assert isinstance(r, np.ndarray)
    assert r.dtype == dtype
    assert r.shape == (2, 3)
    np.testing.assert_array_equal(r, a)


@pytest.mark.parametrize(
    ("fname", "shape", "dtype"),
    [
        ("echo_double_1", (4,), np.float64),
        ("echo_double_3", (2, 3, 4), np.float64),
        ("echo_int_1", (4,), np.int32),
        ("echo_int_3", (2, 3, 4), np.int32),
        ("echo_bool_1", (4,), np.bool_),
    ],
)
def test_ranks(fname, shape, dtype):
    t = ArrayTypemapSample()
    a = (np.arange(np.prod(shape)) % 3).reshape(shape).astype(dtype)
    r = getattr(t, fname)(a)
    assert r.shape == shape
    assert r.dtype == dtype
    np.testing.assert_array_equal(r, a)


def test_conversion_from_other_types():
    """Input that isn't already a numpy array of the right type is converted
    (numpy.asarray with the element type)."""
    t = ArrayTypemapSample()
    # Nested list
    np.testing.assert_array_equal(t.echo_double_2([[1, 2], [3, 4]]), [[1, 2], [3, 4]])
    # Tuple, ints to double
    np.testing.assert_array_equal(t.echo_double_1((1, 2, 3)), [1.0, 2.0, 3.0])
    # Different dtype: converted (a copy)
    r = t.echo_double_2(np.array([[1, 2], [3, 4]], dtype=np.int64))
    assert r.dtype == np.float64
    # float to int: numpy's asarray(..., int32) truncates. Records what
    # SWIG did (nanobind does the same).
    np.testing.assert_array_equal(t.echo_int_1(np.array([1.7, -2.7])), [1, -2])


def test_wrong_rank_or_type():
    t = ArrayTypemapSample()
    with pytest.raises(TypeError):
        t.echo_double_2(np.zeros(3))
    with pytest.raises(TypeError):
        t.echo_double_1(np.zeros((2, 2)))
    with pytest.raises(TypeError):
        t.echo_double_1("not an array")
    with pytest.raises(TypeError):
        t.echo_double_1([1, "x"])
    # Pass by value and FORCE_COPY have their own typemaps. These used to
    # set the error but carry on with a null pointer (segfault).
    with pytest.raises(TypeError):
        t.sum_by_value(np.zeros(3))
    with pytest.raises(TypeError):
        t.sum_by_value("not an array")
    with pytest.raises(TypeError):
        t.set_value_copy(np.zeros(3), 1.0)
    with pytest.raises(TypeError):
        t.set_value_copy("not an array", 1.0)


def test_non_contiguous():
    """Strided views (a slice, a transpose) are read with the right element
    order, without the caller having to copy."""
    t = ArrayTypemapSample()
    base = np.arange(20.0).reshape(4, 5)
    v = base[::2, 1::2]
    assert not v.flags.c_contiguous
    np.testing.assert_array_equal(t.echo_double_2(v), v)
    assert t.sum_double_2(v) == v.sum()
    assert t.element(v, 1, 1) == v[1, 1]
    tr = base.T
    np.testing.assert_array_equal(t.echo_double_2(tr), tr)
    assert t.element(tr, 3, 1) == tr[3, 1]


def test_fortran_order():
    t = ArrayTypemapSample()
    f = np.asfortranarray(np.arange(6.0).reshape(2, 3))
    assert f.flags.f_contiguous and not f.flags.c_contiguous
    np.testing.assert_array_equal(t.echo_double_2(f), f)
    assert t.element(f, 1, 2) == f[1, 2]
    assert t.sum_by_value(f) == f.sum()


def test_by_value():
    t = ArrayTypemapSample()
    assert t.sum_by_value([[1, 2], [3, 4]]) == 10
    assert t.sum_by_value(np.ones((3, 3))) == 9


def test_shared_memory_and_force_copy():
    """A numpy array of the right type is passed to C++ without a copy, so
    C++ writes are seen by python. FORCE_COPY gives C++ a copy."""
    t = ArrayTypemapSample()
    a = np.zeros((2, 2))
    t.set_value(a, 5.0)
    assert a[0, 0] == 5.0
    a = np.zeros((2, 2))
    t.set_value_copy(a, 5.0)
    assert a[0, 0] == 0.0
    # Input that needs conversion is a temporary copy, the original is
    # unchanged.
    a = np.zeros((2, 2), dtype=np.int64)
    t.set_value(a, 5.0)
    assert a[0, 0] == 0


def test_output_arguments():
    t = ArrayTypemapSample()
    a = np.arange(6.0).reshape(2, 3)
    row0, col0 = t.split(a)
    np.testing.assert_array_equal(row0, a[0, :])
    np.testing.assert_array_equal(col0, a[:, 0])
    res, total = t.scaled(a, 2.0)
    np.testing.assert_array_equal(res, a * 2)
    assert total == (a * 2).sum()


def test_vector_of_arrays():
    t = ArrayTypemapSample()
    r = t.echo_vector([np.array([1.0, 2.0]), [3.0], (4.0, 5.0, 6.0)])
    assert isinstance(r, list)
    assert len(r) == 3
    np.testing.assert_array_equal(r[0], [1.0, 2.0])
    np.testing.assert_array_equal(r[2], [4.0, 5.0, 6.0])
    assert t.echo_vector([]) == []
    assert t.vector_size([np.zeros((2, 2), dtype=np.int32)] * 3) == 3
    with pytest.raises(TypeError):
        t.echo_vector([np.zeros((2, 2))])
    with pytest.raises(TypeError):
        t.echo_vector("abc")


def test_returned_array_lifetime():
    """A returned array that shares memory with the C++ object keeps it
    valid after the C++ object is gone."""
    t = ArrayTypemapSample()
    s = t.stored
    del t
    gc.collect()
    np.testing.assert_array_equal(s, [[1, 2, 3], [4, 5, 6]])


def test_returned_reference():
    """A non-const reference return is a writable view of the C++ data."""
    t = ArrayTypemapSample()
    r = t.stored_ref()
    assert r.flags.writeable
    r[0, 0] = 10
    assert t.stored_value(0, 0) == 10
    # The const reference return shares the same data (records what SWIG
    # did, nanobind does the same: the property is a view, not a copy)
    assert t.stored[0, 0] == 10


class PythonTransform(ArrayTypemapSample):
    def transform(self, a):
        return a + 1


class PythonTransformBadReturn(ArrayTypemapSample):
    def transform(self, a):
        return "not an array"


class PythonTransformWrongRank(ArrayTypemapSample):
    def transform(self, a):
        return np.zeros((2, 2))


def test_director():
    """A python override of a virtual function taking and returning
    blitz::Array."""
    a = np.array([1.0, 2.0, 3.0])
    np.testing.assert_array_equal(ArrayTypemapSample().apply_transform(a), a * 2)
    p = PythonTransform()
    np.testing.assert_array_equal(p.apply_transform(a), a + 1)
    np.testing.assert_array_equal(p.apply_transform([1, 2]), [2, 3])
    # A bad return from the python override used to segfault
    with pytest.raises(TypeError):
        PythonTransformBadReturn().apply_transform(a)
    with pytest.raises(TypeError):
        PythonTransformWrongRank().apply_transform(a)
