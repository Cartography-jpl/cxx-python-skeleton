from io import BytesIO

import pytest

from skeleton import IostreamSwig


def test_iostream_write():
    """Test writing to a python file handle like an iostream. This checks the
    handling by swig of the mapping between the two."""
    iostream_swig = IostreamSwig("hello world")
    fh = BytesIO()
    fh.write(b"start string ")
    iostream_swig.write(fh)
    fh.write(b" end string")
    assert fh.getvalue() == b"start string hello world end string"


def test_iostream_read():
    """Test reading from a python file handle like an iostream. This checks the
    handling by swig of the mapping between the two."""
    iostream_swig = IostreamSwig("hello world", 2)
    start_string = b"start string "
    content = b"hello world"
    end_string = b"end string"
    fh = BytesIO(start_string + content + b" " + end_string)
    fh.read(len(start_string))
    assert iostream_swig.read(fh).encode("utf-8") == content
    assert fh.tell() == len(start_string) + len(content + b" ")
    assert fh.read() == end_string


def test_iostream_read_to_eof():
    """Regression test for python_fh_device::read: reading exactly to the
    end of the underlying python file handle used to report a full read
    even when python's read() returned fewer bytes (or none, at EOF),
    which could feed garbage bytes into the stream or leave the C++ side
    unable to see EOF at all. This reads a stream with no trailing
    content, so the last extraction has to read right up against EOF."""
    iostream_swig = IostreamSwig("hello world", 2)
    content = b"hello world"
    fh = BytesIO(content)
    assert iostream_swig.read(fh).encode("utf-8") == content
    assert fh.read() == b""


class _FailingFile:
    """A file object whose write and read raise, like a full disk."""

    def write(self, data):
        raise OSError("disk full")

    def read(self, n=-1):
        raise OSError("read failed")


def test_iostream_write_text_file(tmp_path):
    """A text-mode file wants str, not bytes: the TypeError from its write
    comes out of the C++ call (review 2026-11-07 third, section 3.1)."""
    iostream_swig = IostreamSwig("hello world")
    with open(tmp_path / "out.txt", "w") as fh:
        with pytest.raises(TypeError):
            iostream_swig.write(fh)


def test_iostream_write_error():
    iostream_swig = IostreamSwig("hello world")
    with pytest.raises(OSError, match="disk full"):
        iostream_swig.write(_FailingFile())


def test_iostream_read_error():
    iostream_swig = IostreamSwig("hello world", 2)
    with pytest.raises(OSError, match="read failed"):
        iostream_swig.read(_FailingFile())


def test_iostream_write_raw_file(tmp_path):
    """A raw file (buffering=0) returns the count it wrote."""
    content = "x" * 100_000
    with open(tmp_path / "out.bin", "wb", buffering=0) as fh:
        IostreamSwig(content).write(fh)
    assert (tmp_path / "out.bin").read_bytes() == content.encode()


class _ShortWriteFile(BytesIO):
    """Writes at most 1000 bytes per call, as a raw file may."""

    def write(self, data):
        return super().write(bytes(data[:1000]))


def test_iostream_write_short_writes():
    content = "".join(chr(ord("a") + i % 26) for i in range(10_000))
    fh = _ShortWriteFile()
    IostreamSwig(content).write(fh)
    assert fh.getvalue() == content.encode()


class _ZeroWriteFile:
    def write(self, data):
        return 0


def test_iostream_write_nothing_written():
    """A write that writes nothing raises instead of looping forever."""
    with pytest.raises(RuntimeError, match="wrote 0 of"):
        IostreamSwig("hello world").write(_ZeroWriteFile())
