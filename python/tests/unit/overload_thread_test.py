# Overload dispatch (OverloadExample) and C++ worker threads
# (ThreadExample), claude_notes/implementation_plan.md S6.1 items 7 and 11.
# Phase S5 recorded the SWIG results; comments note where nanobind differs.
import subprocess
import sys
import textwrap

import numpy as np
import pytest

from skeleton import DirectorExample, NumpySample, OverloadExample, ThreadExample


@pytest.mark.parametrize(
    "arg,expect",
    [
        (np.zeros((2, 2)), "array"),
        (np.zeros((2, 2), dtype=np.float32), "array"),
        (NumpySample(), "NumpySample"),
        ("a file name", "string"),
    ],
)
def test_overload(arg, expect):
    assert OverloadExample.which(arg) == expect


def test_thread_cpp_object():
    d = DirectorExample(1, "threads")
    assert ThreadExample.run(d, 1, 4) == 14


# Run in a separate process, a python call on a thread without the GIL
# can crash the interpreter.
_director_script = textwrap.dedent(
    """
    from skeleton import DirectorExample, ThreadExample

    class Py(DirectorExample):
        def func(self, v):
            if v == 3:
                raise ValueError("python error on worker")
            return 2 * v

    p = Py(0, "py")
    print("sum", ThreadExample.run(p, 0, 3))
    try:
        ThreadExample.run(p, 0, 4)
    except ValueError as e:
        print("caught", e)
    """
)


def run_director_script():
    return subprocess.run(
        [sys.executable, "-c", _director_script],
        capture_output=True,
        text=True,
        check=False,
        timeout=60,
    )


def test_thread_python_override():
    """A python override called on C++ worker threads, and a python exception
    raised there, reaching the caller. Under SWIG (no -threads, no GIL
    handling) this segfaulted (recorded in S5). nanobind's trampoline takes
    the GIL; ThreadExample.run releases it while it waits."""
    r = run_director_script()
    assert r.returncode == 0, r.stderr
    assert "sum 6" in r.stdout
    assert "caught python error on worker" in r.stdout
