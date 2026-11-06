# Fixed size array and optional members as python properties, and pickling.
# ArraySample had boost::array/boost::optional members until phase S3.2
# changed them to std::array/std::optional; these tests passed before and
# after (see claude_notes/implementation_plan.md, phase S3).
import pickle

import numpy as np
import pytest
from skeleton import ArraySample
from skeleton_swig.swig_boost_array import Array_bool_20, Array_double_3


def test_defaults():
    a = ArraySample()
    assert str(a) == "ArraySample"
    np.testing.assert_array_equal(a.position_, [1, 2, 3])
    np.testing.assert_array_equal(a.matrix, [[1, 2, 3], [4, 5, 6]])
    assert a.count == 7
    assert a.scale is None


@pytest.mark.parametrize(
    "value", [[4, 5, 6], (4, 5, 6), np.array([4.0, 5.0, 6.0]), np.arange(4.0, 10.0)[::2]]
)
def test_position(value):
    a = ArraySample()
    a.position_ = value
    np.testing.assert_array_equal(a.position_, np.asarray(value, dtype=float))


def test_position_wrong_size():
    a = ArraySample()
    with pytest.raises(RuntimeError):
        a.position_ = [1, 2]


def test_matrix():
    a = ArraySample()
    a.matrix = [[-1, -2, -3], [7, 8, 9.5]]
    np.testing.assert_array_equal(a.matrix, [[-1, -2, -3], [7, 8, 9.5]])
    a.matrix = np.arange(6.0).reshape(2, 3)
    np.testing.assert_array_equal(a.matrix, np.arange(6.0).reshape(2, 3))
    with pytest.raises(RuntimeError):
        a.matrix = np.zeros((3, 2))


def test_optional():
    a = ArraySample()
    a.count = 3
    a.scale = 0.25
    assert a.count == 3
    assert a.scale == 0.25
    a.count = None
    a.scale = None
    assert a.count is None
    assert a.scale is None


def test_pickle():
    a = ArraySample()
    a.position_ = [0.5, -1.25, 3e8]
    a.matrix = [[1.5, -2, 3], [4, 5, -6.5]]
    a.count = None
    a.scale = 2.5
    b = pickle.loads(pickle.dumps(a))
    assert isinstance(b, ArraySample)
    assert list(b.position_) == [0.5, -1.25, 3e8]
    assert b.matrix.tolist() == [[1.5, -2, 3], [4, 5, -6.5]]
    assert b.count is None
    assert b.scale == 2.5


def test_std_array_return():
    """A std::array return value is a new numpy array (a copy)."""
    a = ArraySample()
    a.position_ = [0.5, -1.25, 3e8]
    for v in (a.position(), a.position_ref()):
        assert isinstance(v, np.ndarray)
        assert v.dtype == np.float64
        assert v.tolist() == [0.5, -1.25, 3e8]
    v = a.position()
    v[0] = 99
    assert a.position_[0] == 0.5


def test_boost_array_print_to_string():
    """print_to_string of the wrapped boost::array classes returns a str
    (it used to return an opaque, leaked std::string*)."""
    a = Array_double_3()
    a.set([1.5, 2, -3])
    assert a.print_to_string() == "1.5 2 -3 "
    b = Array_bool_20()
    b.set([True] + [False] * 19)
    assert b.print_to_string() == "1 " + "0 " * 19
