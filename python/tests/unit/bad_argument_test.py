# Bad arguments from python must raise a python exception, not crash the
# interpreter (code review 2026-11-06 §2.4 and §2.7). Each case runs in a
# subprocess, so a regression (a segmentation fault) fails that one test
# instead of killing the whole pytest run.
import subprocess
import sys
import textwrap

import pytest

CASES = {
    # None for a shared_ptr argument C++ dereferences: nanobind rejects it
    # (TypeError) or C++ checks it (RuntimeError).
    "number_band_none": "MultiBaseExample.number_band(None)",
    "parameter_sum_none": "MultiBaseExample.parameter_sum(None)",
    "root_name_none": "VirtualDiamondExample.root_name(None)",
    "nested_name_none": "VirtualDiamondExample.nested_name(None)",
    "diamond_parameter_sum_none": "VirtualDiamondExample.parameter_sum(None)",
    "cache_example_none": "CacheExample(None)",
    "thread_run_none": "ThreadExample.run(None, 1, 2)",
    "keep_reference_none": "ObservableExample(1).add_observer_and_keep_reference(None)",
    "sub_raster_none": "SubRasterSample(None, 0, 0, 1, 1)",
    "set_vec_none": "DirectorExampleUser().set_vec(None)",
    # No value set (a null member), and out-of-range indices.
    "apply_func_no_value": "DirectorExampleUser().apply_func(1)",
    "apply_func_index": "DirectorExampleUser().apply_func(1, 5)",
    "apply_func_index2": "DirectorExampleUser().apply_func(1, 0, 0)",
    "apply_func_negative_index": "DirectorExampleUser().apply_func(1, -1)",
    "generic_object_index": "generic_object_index(100)",
    "data_value": "CxxPythonRulesSample().data_value(5, 0)",
    "data_value_negative": "CxxPythonRulesSample().data_value(0, -1)",
    "stored_value": "ArrayTypemapSample().stored_value(0, 7)",
    "element": "ArrayTypemapSample().element(numpy.zeros((2, 2)), 2, 0)",
    "thread_run_zero": "ThreadExample.run(DirectorExample(1, 'a'), 1, 0)",
}

_SCRIPT = textwrap.dedent(
    """
    import numpy
    import skeleton
    globals().update({{k: getattr(skeleton, k) for k in dir(skeleton)}})
    try:
        {expr}
    except (TypeError, RuntimeError, IndexError, ValueError) as e:
        print("raised", type(e).__name__)
    else:
        print("no exception")
    """
)


@pytest.mark.parametrize("name", sorted(CASES))
def test_bad_argument_raises(name):
    r = subprocess.run(
        [sys.executable, "-c", _SCRIPT.format(expr=CASES[name])],
        capture_output=True,
        text=True,
        check=False,
        timeout=60,
    )
    assert r.returncode == 0, f"crashed ({r.returncode}): {r.stderr[-2000:]}"
    assert r.stdout.startswith("raised"), r.stdout + r.stderr


def test_none_still_allowed():
    """None stays allowed where C++ handles a null pointer."""
    from skeleton import (
        DirectorExampleUser,
        GenericObjectMap,
        MultiBaseExample,
        ObservableExample,
        ObserverExample,
    )

    u = DirectorExampleUser()
    u.set_value(None)
    assert u.value() is None
    m = GenericObjectMap()
    m.set_generic("a", None)
    assert m.get_generic("a") is None
    assert MultiBaseExample.type_name(None) == "None"
    o = ObserverExample("x", None)
    assert o.observed is None
    assert ObservableExample(1).number_observer == 0
