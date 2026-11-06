# Tests for the smaller rules (cxx_python_rules/nanobind; the SWIG
# swig_rules/include before phase S6), using lib/cxx_python_rules_sample.h:
# std::optional, boost::rational, std::array, properties, pickle_init,
# pickle_vector and a bound std::vector of values. Written under SWIG and
# kept for nanobind; comments mark details that came from SWIG.
import pickle

import numpy as np
import pytest

from skeleton import CxxPythonRulesSample, NumpySample, ValueSample, Vector_ValueSample


def test_optional():
    t = CxxPythonRulesSample()
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
    t = CxxPythonRulesSample()
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

    Under SWIG python couldn't construct a rational_int (swig_rational.i
    was never built as a module), so the test only uses values returned
    from C++. The nanobind bindings also bind the constructor,
    rational_int(numerator, denominator=1) (cxx_python_rules_nb.cc)."""
    t = CxxPythonRulesSample()
    h = t.half()
    assert t.rational_value(h) == 0.5
    with pytest.raises(TypeError):
        t.rational_value(0.5)


def test_boost_array():
    t = CxxPythonRulesSample()
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
    # Strided (non-contiguous) numpy views, through each copy path.
    t.position_ = np.arange(6.0)[::2]
    np.testing.assert_array_equal(t.position_, [0.0, 2.0, 4.0])
    assert t.sum_array(np.arange(6.0)[::2]) == 6
    assert t.first_element(np.arange(6.0)[1::2]) == 1
    # Wrong length. SWIG gave ValueError from the typemap for const&,
    # TypeError (no overload matched) for by value; either is accepted.
    with pytest.raises((ValueError, TypeError)):
        t.sum_array([1, 2])
    with pytest.raises((ValueError, TypeError)):
        t.first_element([1, 2, 3, 4])


def test_attribute_variants():
    t = CxxPythonRulesSample(3, "x")
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


class PythonWeight(CxxPythonRulesSample):
    def __init__(self):
        super().__init__(1, "py")
        self._w = 7.0

    def _v_weight(self, *args):
        # The python override replaces the C++ virtual under the name
        # _v_<name> (from SWIG's %python_attribute_with_set_virtual; the
        # nanobind trampoline keeps it).
        if len(args) == 0:
            return self._w
        self._w = args[0]
        return None


def test_attribute_virtual():
    t = CxxPythonRulesSample()
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
    t = CxxPythonRulesSample(3, "x")
    t2 = pickle.loads(pickle.dumps(t))
    assert isinstance(t2, CxxPythonRulesSample)
    assert t2.count == 3
    assert t2.name == "x"
    v = pickle.loads(pickle.dumps(ValueSample(2.5)))
    assert v.v == 2.5


def test_vector_value_type():
    """std::vector<T> where T is wrapped with %shared_ptr but stored by value
    (%shared_ptr_value_traits)."""
    t = CxxPythonRulesSample()
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
    t = CxxPythonRulesSample()
    assert list(t.int_vector()) == [1, 2, 3]
    assert t.sum_int_vector([1, 2, 3]) == 6
    assert t.sum_int_vector((4, 5)) == 9
    assert t.sum_int_vector(t.int_vector()) == 6
    with pytest.raises(TypeError):
        t.sum_int_vector(["a"])


def test_str():
    assert str(CxxPythonRulesSample(3, "x")) == "CxxPythonRulesSample(3, x)"
    assert str(ValueSample(2.5)) == "ValueSample(2.5)"
