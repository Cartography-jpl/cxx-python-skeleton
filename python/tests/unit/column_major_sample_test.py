# Column-major and strided Blitz arrays (ColumnMajorSample in
# lib/array_sample.h), phase S5. Records what the SWIG typemaps in
# swig_rules/include/swig_array.i do; the nanobind bridge (S6) must
# give the same results.
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


@pytest.mark.xfail(
    strict=True,
    raises=TypeError,
    reason="swig_array.i has no typemap for a non-const blitz::Array& argument",
)
@pytest.mark.parametrize("make", [lambda a: a, np.asfortranarray, lambda a: a[:, ::2]])
def test_scale_in_place(make):
    a = make(np.arange(12.0).reshape(3, 4))
    b = a.copy()
    ColumnMajorSample.scale_in_place(a, 2)
    np.testing.assert_array_equal(a, 2 * b)


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
