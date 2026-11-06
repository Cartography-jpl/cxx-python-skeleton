# Check str() and exception text for the C++ classes. These are the
# baseline strings later phases (C++20 Printable/Exception changes,
# nanobind) must reproduce, see claude_notes/implementation_plan.md.
import pytest

from skeleton import (
    ArrayTypemapSample,
    DirectorExample,
    DirectorExampleNested,
    DirectorExampleUser,
    DirectorExampleWeakPtr,
    ExceptionSample,
    FileReferenceSample,
    GenericObjectA,
    GenericObjectB,
    GenericObjectC,
    GenericObjectMap,
    IostreamSwig,
)
from skeleton_swig import Exception as SkeletonException


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
    # The weak_ptr needs a C++ owner of e (nanobind, see
    # test_director_example_weak_ptr), here the DirectorExampleUser.
    owner = DirectorExampleUser()
    owner.set_value(e)
    assert str(DirectorExampleWeakPtr(e)) == (
        "----------------------\n"
        "DirectorExampleWeakPtr\n"
        "DirectorExample:\n  name:         e1\n  value_to_add: 1"
        "\n----------------------\n"
    )
    # Nested prints as its DirectorExample base.
    assert str(DirectorExampleNested(2, "n", e)) == (
        "DirectorExample:\n  name:         n\n  value_to_add: 2"
    )
    assert str(ArrayTypemapSample()) == "ArrayTypemapSample"


def test_str_file_reference(tmp_path):
    fname = tmp_path / "f.txt"
    fname.write_text("contents\n")
    assert str(FileReferenceSample(str(fname))) == f"FileReferenceSample({fname})"


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
    has the python error and traceback. (The format differs: SWIG's
    PythonException started with "Python error occured:", nanobind's
    python_error is the python traceback text.)"""
    assert ExceptionSample().value_error_message() == ""
    msg = PythonRaises().value_error_message()
    assert "ValueError: raised in python" in msg
    assert "Traceback" in msg
    assert "in value" in msg
    # Propagated back to python, the original exception type comes through
    with pytest.raises(ValueError, match="raised in python"):
        PythonRaises().value_rethrow_copy()


def test_exception_stacktrace():
    """A Skeleton::Exception records its source location and stack trace."""
    e = ExceptionSample.caught_exception("a message")
    assert e.what() == "a message"
    assert e.source_file().endswith("exception_sample.h")
    assert e.source_line() > 0
    assert "caught_exception" in e.source_function()
    st = e.stacktrace()
    assert st.startswith(" 0# ")
    assert len(st.splitlines()) > 1


def test_type_error():
    """Skeleton::TypeError (a library exception) becomes a python TypeError."""
    with pytest.raises(TypeError, match="wrong type"):
        ExceptionSample().throw_type_error("wrong type")


def test_raised_exception_stacktrace():
    """A Skeleton::Exception raised into python carries the C++ exception
    (with its stack trace) and source location (attributes cpp_exception
    and cpp_source)."""
    with pytest.raises(RuntimeError) as e_info:
        ExceptionSample().throw_skeleton_exception("with trace")
    e = e_info.value
    assert str(e) == "with trace"
    assert "exception_sample.h:" in e.cpp_source
    assert "throw_skeleton_exception" in e.cpp_source
    assert e.cpp_exception.what() == "with trace"
    assert e.cpp_exception.stacktrace().startswith(" 0# ")
