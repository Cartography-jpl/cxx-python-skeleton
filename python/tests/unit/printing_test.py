# Check str() and exception text for the C++ classes. These are the
# baseline strings later phases (C++20 Printable/Exception changes,
# nanobind) must reproduce, see claude_notes/implementation_plan.md.
import pytest

from skeleton import (
    DirectorExample,
    DirectorExampleUser,
    DirectorExampleWeakPtr,
    ExceptionSample,
    GenericObjectA,
    GenericObjectB,
    GenericObjectC,
    GenericObjectMap,
    IostreamSwig,
)
from skeleton_swig.skeleton_exception import Exception as SkeletonException


def test_str():
    a = GenericObjectA(1)
    # Short strings used to come out as "Alah", see printable_test.cc
    assert str(a) == "A"
    assert str(GenericObjectB(a)) == "B"
    assert str(GenericObjectC(a)) == "C"
    assert str(GenericObjectMap()) == "GenericObjectMap"
    assert str(IostreamSwig()) == "IostreamSwigTest"
    assert str(DirectorExampleUser()) == "DirectorExampleUser"
    e = DirectorExample(1, "e1")
    assert str(e) == "DirectorExample:\n  name:         e1\n  value_to_add: 1"
    assert str(DirectorExampleWeakPtr(e)) == (
        "----------------------\n"
        "DirectorExampleWeakPtr\n"
        "DirectorExample:\n  name:         e1\n  value_to_add: 1"
        "\n----------------------\n"
    )


class DescOverride(DirectorExample):
    def desc(self):
        return "From python"


def test_str_desc_override():
    """A python subclass overriding desc() changes str()."""
    assert str(DescOverride(1, "e1")) == "From python"


def test_exception_str():
    e = SkeletonException("x")
    assert e.what() == "x"
    assert str(e) == (
        "Skeleton Exception:\n=========================\nx\n=========================\n"
    )


def test_cpp_exception_to_python():
    """C++ exceptions become a python RuntimeError with the what() text."""
    t = ExceptionSample()
    assert str(t) == "ExceptionSample"
    with pytest.raises(RuntimeError) as e_info:
        t.throw_skeleton_exception("Skeleton message")
    assert str(e_info.value) == "Skeleton message"
    with pytest.raises(RuntimeError) as e_info:
        t.throw_std_exception("std message")
    assert str(e_info.value) == "std message"
    # Something not derived from std::exception
    with pytest.raises(RuntimeError) as e_info:
        t.throw_unknown()
    assert str(e_info.value) == "unknown C++ exception"
    assert t.checked_value(5) == 5
    with pytest.raises(RuntimeError) as e_info:
        t.checked_value(10)
    assert str(e_info.value).startswith("Out of range error in file")
    assert "Value:           10\n" in str(e_info.value)


class PythonRaises(ExceptionSample):
    def value(self):
        raise ValueError("raised in python")


def test_python_exception_seen_from_cpp():
    """When a python override raises and C++ catches the exception, what()
    has the python error and traceback. (The text is the SWIG
    PythonException format; the nanobind version should give the same
    information.)"""
    assert ExceptionSample().value_error_message() == ""
    msg = PythonRaises().value_error_message()
    assert msg.startswith("Python error occured:\n")
    assert "ValueError: raised in python" in msg
    assert "Traceback" in msg
    assert "in value" in msg
    # Propagated back to python, the original exception type comes through
    with pytest.raises(ValueError, match="raised in python"):
        PythonRaises().value_rethrow_copy()
