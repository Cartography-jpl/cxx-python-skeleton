# Multiple inheritance and virtual diamonds (lib/multi_base_example.h,
# lib/virtual_diamond_example.h), the downcast and argument matrices of
# claude_notes/implementation_plan.md §12. Phase S5: recorded under SWIG,
# must keep passing under nanobind (S6).
import pickle

import numpy as np
import pytest
from skeleton import (
    CameraLike,
    Diamond,
    MultiBaseExample,
    ObservableCameraLike,
    ObserverCameraLike,
    SimpleCameraLike,
    VirtualDiamondExample,
    WithParameterLike,
)


@pytest.fixture
def cam():
    return SimpleCameraLike(10, 3)


@pytest.mark.parametrize(
    "func",
    ["camera", "camera_as_with_parameter", "camera_as_generic", "camera_as_observable"],
)
def test_downcast(cam, func):
    """Returned as any base, python gets the most derived type, and methods of
    every base work."""
    m = MultiBaseExample(cam)
    c = getattr(m, func)
    if callable(c):
        c = c()
    assert isinstance(c, SimpleCameraLike)
    assert c.focal_length == 10
    assert c.number_band == 3
    assert c.parameter_sum() == 10
    np.testing.assert_array_equal(c.parameter, [10])
    assert str(c) == "SimpleCameraLike 10 3"


def test_argument(cam):
    """The derived object can be passed for any base."""
    assert MultiBaseExample.number_band(cam) == 3
    assert MultiBaseExample.parameter_sum(cam) == 10
    assert MultiBaseExample.parameter_sum_ref(cam) == 10
    assert MultiBaseExample.type_name(cam) == "Skeleton::SimpleCameraLike"
    assert MultiBaseExample.type_name(None) == "None"


def test_isinstance(cam):
    assert isinstance(cam, CameraLike)
    assert isinstance(cam, ObservableCameraLike)


def test_isinstance_secondary_base(cam):
    """SWIG binds every base, so the secondary base is a python base class."""
    assert isinstance(cam, WithParameterLike)


def test_observer_and_parameter(cam):
    class PyCameraObserver(ObserverCameraLike):
        def __init__(self):
            super().__init__()
            self.focal = []

        def notify_update(self, c):
            self.focal.append(c.focal_length)

    obs = PyCameraObserver()
    cam.add_observer(obs)
    cam.parameter = [20]
    assert obs.focal == [20]
    assert cam.focal_length == 20


def test_pickle(cam):
    c2 = pickle.loads(pickle.dumps(cam))
    assert isinstance(c2, SimpleCameraLike)
    assert c2.focal_length == 10
    assert c2.number_band == 3


@pytest.fixture
def diamond():
    return Diamond("d", 1, 2)


@pytest.mark.parametrize(
    "func", ["diamond", "as_root", "as_nested", "as_with_parameter", "as_generic"]
)
def test_diamond_downcast(diamond, func):
    v = VirtualDiamondExample(diamond)
    d = getattr(v, func)
    if callable(d):
        d = d()
    assert isinstance(d, Diamond)
    assert d.name == "d"
    assert d.root_name() == "Diamond d"
    assert d.nested_name() == "Diamond nested d"
    assert d.parameter_sum() == 3
    assert str(d) == "Diamond d"


def test_diamond_argument(diamond):
    assert VirtualDiamondExample.root_name(diamond) == "Diamond d"
    assert VirtualDiamondExample.nested_name(diamond) == "Diamond nested d"
    assert VirtualDiamondExample.parameter_sum(diamond) == 3
    assert MultiBaseExample.parameter_sum(diamond) == 3
    assert MultiBaseExample.type_name(diamond) == "Skeleton::Diamond"


def test_diamond_parameter(diamond):
    diamond.parameter = [3, 4]
    np.testing.assert_array_equal(diamond.parameter, [3, 4])
    assert diamond.parameter_sum() == 7


def test_diamond_pickle(diamond):
    d2 = pickle.loads(pickle.dumps(diamond))
    assert isinstance(d2, Diamond)
    assert d2.name == "d"
    assert d2.parameter_sum() == 3
    v = VirtualDiamondExample(d2)
    assert pickle.loads(pickle.dumps(v.as_generic())).root_name() == "Diamond d"


def test_identity(cam):
    """Recorded: SWIG gives a new python wrapper each time the same C++
    object is returned (nanobind is expected to return the same object)."""
    m = MultiBaseExample(cam)
    assert m.camera is not m.camera
    assert m.camera.focal_length == m.camera.focal_length
