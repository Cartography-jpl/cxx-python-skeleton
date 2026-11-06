"""Python subclass used by the golden shelves with a python director object
(unit_test_data/golden/director_python_*.xml). Those were written with the
SWIG bindings (phase S5, 2026-11-06); the nanobind bindings must still read
them. The class has to stay importable under this name."""

from skeleton import DirectorExample


class GoldenPythonDirector(DirectorExample):
    def __init__(self, v1, v2, name):
        super().__init__(v1, name)
        self.v2 = v2

    def func(self, v):
        return super().func(v) + 2 * self.v2
