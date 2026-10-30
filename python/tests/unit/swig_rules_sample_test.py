# Tests for the smaller rules in swig_rules/include, using
# lib/swig_rules_sample.h: boost::optional, boost::rational, boost::array,
# the %python_attribute variants, %pickle_init, %pickle_vector and
# %shared_ptr_value_traits. Like the other tests, these describe behavior
# that should survive the move to nanobind; comments mark SWIG-specific
# details.
import pickle

import numpy as np
import pytest

from skeleton import NumpySample, SwigRulesSample, ValueSample, Vector_ValueSample


def test_optional():
    t = SwigRulesSample()
    assert t.optional_int is None
    t.optional_int = 3
    assert t.optional_int == 3
    t.optional_int = None
    assert t.optional_int is None
    assert t.optional_double() is None
    t.optional_double(2.5)
    assert t.optional_double() == 2.5
    assert t.optional_double_ref() == 2.5
    t.optional_double(None)
    assert t.optional_double() is None
    assert t.optional_double_ref() is None
    # Ints are accepted for a double
    t.optional_double(2)
    assert t.optional_double() == 2.0
    # Values that can't be converted are a TypeError (they used to become
    # -1 silently)
    with pytest.raises(TypeError):
        t.optional_int = "x"
    with pytest.raises(TypeError):
        t.optional_double("x")


def test_optional_range():
    t = SwigRulesSample()
    assert t.optional_range_size(None) == -1
    assert t.optional_range_ref() is None
    r = NumpySample().maybe_range(False)  # a blitz::Range from C++
    assert t.optional_range_size(r) == 2
    t.optional_range(r)
    r2 = t.optional_range_ref()
    assert (r2.first(), r2.last()) == (0, 1)
    t.optional_range(None)
    assert t.optional_range_ref() is None
    with pytest.raises(TypeError):
        t.optional_range_size("x")


def test_rational():
    """boost::rational<int> returned from C++ and passed back in.

    Note swig_rational.i defines the python class rational_int, but no
    repository (skeleton, geocal, framework) builds swig_rational.i as a
    module, so python can't construct one directly; only values returned
    from C++ are usable. Recorded in claude_notes/implementation_plan.md."""
    t = SwigRulesSample()
    h = t.half()
    assert t.rational_value(h) == 0.5
    with pytest.raises(TypeError):
        t.rational_value(0.5)


def test_boost_array():
    t = SwigRulesSample()
    np.testing.assert_array_equal(t.position_, [1.0, 2.0, 3.0])
    t.position_ = [4, 5, 6]
    np.testing.assert_array_equal(t.position_, [4.0, 5.0, 6.0])
    t.position_ = np.array([7.0, 8.0, 9.0])
    np.testing.assert_array_equal(t.position_, [7.0, 8.0, 9.0])
    with pytest.raises(RuntimeError):
        t.position_ = [1, 2]
    assert t.sum_array([1, 2, 3]) == 6
    assert t.sum_array(np.array([1.0, 2.0, 3.0])) == 6
    assert t.first_element((5, 6, 7)) == 5
    # Wrong length. Records SWIG today: ValueError from the typemap for
    # const&, TypeError (no overload matched) for by value.
    with pytest.raises((ValueError, TypeError)):
        t.sum_array([1, 2])
    with pytest.raises((ValueError, TypeError)):
        t.first_element([1, 2, 3, 4])


def test_attribute_variants():
    t = SwigRulesSample(3, "x")
    assert t.count == 3
    assert t.name == "x"
    # %python_attribute2: python name differs from the C++ function
    assert t.count_squared == 9
    assert t.scale == 1.0
    t.scale = 2.5
    assert t.scale == 2.5
    # %python_attribute_nonconst and %python_attribute2_nonconst
    assert t.counter == 3
    d = t.data
    d[1, 0] = 5.0
    assert t.data_value(1, 0) == 5.0
    t.data[0, 1] = 6.0
    assert t.data_value(0, 1) == 6.0
    # read only attributes
    with pytest.raises(AttributeError):
        t.count = 4
    with pytest.raises(AttributeError):
        t.count_squared = 4


class PythonWeight(SwigRulesSample):
    def __init__(self):
        super().__init__(1, "py")
        self._w = 7.0

    def _v_weight(self, *args):
        # %python_attribute_with_set_virtual: the python override replaces
        # the renamed C++ virtual (SWIG specific name, _v_<name>).
        if len(args) == 0:
            return self._w
        self._w = args[0]
        return None


def test_attribute_virtual():
    t = SwigRulesSample()
    assert t.weight == 2.0
    t.weight = 3.0
    assert t.weight_set() == 3.0
    assert t.weight_from_cpp() == 2.0
    p = PythonWeight()
    assert p.weight == 7.0
    # C++ calling the virtual gets the python override
    assert p.weight_from_cpp() == 7.0
    p.weight = 8.0
    assert p.weight_from_cpp() == 8.0


def test_pickle_init():
    t = SwigRulesSample(3, "x")
    t2 = pickle.loads(pickle.dumps(t))
    assert isinstance(t2, SwigRulesSample)
    assert t2.count == 3
    assert t2.name == "x"
    v = pickle.loads(pickle.dumps(ValueSample(2.5)))
    assert v.v == 2.5


def test_vector_value_type():
    """std::vector<T> where T is wrapped with %shared_ptr but stored by value
    (%shared_ptr_value_traits)."""
    t = SwigRulesSample()
    vals = t.values()
    assert len(vals) == 2
    # Iteration gives the real objects, not untyped pointers
    assert [type(v) for v in vals] == [ValueSample, ValueSample]
    assert [v.v for v in vals] == [1.0, 2.0]
    assert vals[1].v == 2.0
    # A python list converts to the std::vector
    assert t.sum_values([ValueSample(1), ValueSample(2.5)]) == 3.5
    assert t.sum_values(vals) == 3.0
    assert t.sum_values([]) == 0
    with pytest.raises(TypeError):
        t.sum_values([1.0])


def test_vector_pickle():
    v = Vector_ValueSample()
    v.append(ValueSample(1))
    v.append(ValueSample(4))
    v2 = pickle.loads(pickle.dumps(v))
    assert [x.v for x in v2] == [1.0, 4.0]


def test_std_vector():
    t = SwigRulesSample()
    assert list(t.int_vector()) == [1, 2, 3]
    assert t.sum_int_vector([1, 2, 3]) == 6
    assert t.sum_int_vector((4, 5)) == 9
    assert t.sum_int_vector(t.int_vector()) == 6
    with pytest.raises(TypeError):
        t.sum_int_vector(["a"])


def test_str():
    assert str(SwigRulesSample(3, "x")) == "SwigRulesSample(3, x)"
    assert str(ValueSample(2.5)) == "ValueSample(2.5)"
