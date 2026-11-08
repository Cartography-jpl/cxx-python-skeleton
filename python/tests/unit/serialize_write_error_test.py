"""Archive writes report a failed write (review 2026-11-07 third, section
3.2). /dev/full accepts open and fails every write with "no space left on
device". The archive is written through a symlink to it, so the name can end
in ".gz". Each case runs in its own process with a timeout: a gzip write
error used to hang at 100% CPU."""

import subprocess
import sys
import textwrap
from pathlib import Path

import pytest

pytestmark = pytest.mark.skipif(
    not Path("/dev/full").exists(), reason="needs /dev/full"
)

_script = textwrap.dedent(
    """
    import sys
    from skeleton import (GenericObjectA, GenericObjectMap, serialize_write,
                          serialize_write_binary)
    fname, size, fmt = sys.argv[1], sys.argv[2], sys.argv[3]
    if size == "small":
        obj = GenericObjectA(1)
    else:
        obj = GenericObjectMap()
        for i in range(20000):
            obj[f"a{i}"] = GenericObjectA(i)
    write = serialize_write if fmt == "xml" else serialize_write_binary
    try:
        write(fname, obj)
    except RuntimeError as e:
        print("RuntimeError:", e)
        sys.exit(3)
    sys.exit(0)
    """
)


@pytest.mark.parametrize(
    "size,fmt,ext",
    [
        ("small", "xml", ""),
        ("small", "bin", ""),
        ("small", "xml", ".gz"),
        ("small", "bin", ".gz"),
        ("large", "xml", ""),
        ("large", "bin", ""),
        ("large", "xml", ".gz"),
        ("large", "bin", ".gz"),
    ],
)
def test_serialize_write_full_disk(tmp_path, size, fmt, ext):
    fname = tmp_path / f"full.{fmt}{ext}"
    fname.symlink_to("/dev/full")
    try:
        r = subprocess.run(
            [sys.executable, "-c", _script, str(fname), size, fmt],
            capture_output=True,
            text=True,
            timeout=20,
        )
    except subprocess.TimeoutExpired:
        pytest.fail("the write hung")
    assert r.returncode == 3, r.stdout + r.stderr
    assert "Trouble writing file" in r.stdout
    assert "No space left on device" in r.stdout
