# Column-major and strided Blitz arrays (ColumnMajorSample in
# lib/array_sample.h), phase S5. Recorded what the SWIG typemaps in
# swig_rules/include/swig_array.i did; the nanobind bindings (S6) give
# the same results.
import numpy as np
import pytest

from skeleton import ColumnMajorSample


def expected(rows=2, cols=3):
    return np.array(
        [[10 * i + j for j in range(cols)] for i in range(rows)], dtype=float
    )


def test_column_major_return():
    s = ColumnMajorSample(2, 4)
    d = s.data
    np.testing.assert_array_equal(d, expected(2, 4))
    assert d.flags.f_contiguous
    assert not d.flags.c_contiguous
    # A view of the C++ memory, read only.
    assert not d.flags.writeable
    assert not d.flags.owndata
    # The view keeps the C++ data alive.
    del s
    np.testing.assert_array_equal(d, expected(2, 4))


def test_strided_view():
    s = ColumnMajorSample(2, 4)
    v = s.strided_view()
    np.testing.assert_array_equal(v, expected(2, 4)[:, ::2])
    assert not v.flags.f_contiguous
    assert not v.flags.c_contiguous
    assert np.shares_memory(v, s.data)


@pytest.mark.parametrize(
    "make,layout",
    [
        (lambda a: a, "row_major"),
        (np.asfortranarray, "column_major"),
        (lambda a: a[:, ::2], "strided"),
        (lambda a: a.T, "column_major"),
    ],
)
def test_input_layout(make, layout):
    """numpy arrays of float64 reach C++ without a copy, with their layout."""
    a = make(np.arange(12.0).reshape(3, 4))
    assert ColumnMajorSample.layout(a) == layout
    assert ColumnMajorSample.data_address(a) == a.ctypes.data
    assert ColumnMajorSample.sum(a) == a.sum()


@pytest.mark.parametrize("make", [lambda a: a, np.asfortranarray, lambda a: a[:, ::2]])
def test_scale_in_place(make):
    """A non-const blitz::Array& argument shares python's memory (under SWIG
    this was a TypeError, swig_array.i had no typemap for it)."""
    a = make(np.arange(12.0).reshape(3, 4))
    b = a.copy()
    ColumnMajorSample.scale_in_place(a, 2)
    np.testing.assert_array_equal(a, 2 * b)


@pytest.mark.parametrize(
    "make",
    [
        # read-only
        lambda: np.frombuffer(np.arange(12.0).tobytes()).reshape(3, 4),
        # converted copies: another dtype, a list, negative strides
        lambda: np.arange(12, dtype=np.int32).reshape(3, 4),
        lambda: np.arange(12.0).reshape(3, 4).tolist(),
        lambda: np.arange(12.0).reshape(3, 4)[::-1],
    ],
)
def test_scale_in_place_rejects_copy_or_read_only(make):
    """A non-const blitz::Array& argument C++ writes to doesn't take a
    read-only array or a converted copy (code review 2026-11-06 §2.7: C++
    wrote into read-only arrays, and writes into a copy were lost)."""
    a = make()
    b = np.array(a, copy=True)
    with pytest.raises(TypeError):
        ColumnMajorSample.scale_in_place(a, 2)
    np.testing.assert_array_equal(np.asarray(a), b)


def test_const_reference_takes_read_only():
    """A const reference still takes a read-only array (no copy)."""
    a = np.frombuffer(np.arange(6.0).tobytes()).reshape(2, 3)
    assert not a.flags.writeable
    assert ColumnMajorSample.sum(a) == 15.0
    assert ColumnMajorSample.data_address(a) == a.__array_interface__["data"][0]


def test_set_data():
    s = ColumnMajorSample()
    s.data = np.asfortranarray(np.arange(6.0).reshape(2, 3))
    np.testing.assert_array_equal(s.data, np.arange(6.0).reshape(2, 3))
    s.data = np.arange(12.0).reshape(3, 4)[:, ::2]
    np.testing.assert_array_equal(s.data, np.arange(12.0).reshape(3, 4)[:, ::2])
    assert s.data.flags.f_contiguous


def test_other_dtype_is_copied():
    """A different dtype is converted, so C++ gets a copy."""
    a = np.arange(6, dtype=np.int32).reshape(2, 3)
    assert ColumnMajorSample.sum(a) == 15
    assert ColumnMajorSample.data_address(a) != a.ctypes.data


def test_wrong_rank():
    with pytest.raises((TypeError, RuntimeError, ValueError)):
        ColumnMajorSample.sum(np.arange(6.0))
