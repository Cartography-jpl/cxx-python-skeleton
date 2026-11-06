# The mdspan sample (lib/raster_sample.h, phase S7): geocal's RasterImage
# read_ptr pattern with an mdspan, and its CalcRaster pattern (a python
# subclass fills a writable numpy view of a C++ array in calc()).
import gc

import numpy as np
import pytest

from skeleton import (
    BufferRasterSample,
    CalcRasterSample,
    MemoryRasterSample,
    RasterSample,
    SubRasterSample,
    copy_raster,
)


def expected(lstart, sstart, nl, ns):
    # value 1000 * line + sample, the same as BufferRasterSample
    return np.array(
        [[1000 * (lstart + i) + sstart + j for j in range(ns)] for i in range(nl)],
        dtype=np.int32,
    )


class PyCalc(CalcRasterSample):
    def __init__(self, nl=7, ns=9):
        super().__init__(nl, ns)
        self.ncalc = 0

    def calc(self, lstart, sstart):
        self.ncalc += 1
        nl, ns = self.data.shape
        # Fill the C++ array in place, as geocal's python CalcRaster
        # subclasses do (self.data[...] = ...).
        self.data[:, :] = expected(lstart, sstart, nl, ns) + 0.4


def test_memory():
    m = MemoryRasterSample(expected(0, 0, 5, 6))
    assert (m.number_line, m.number_sample) == (5, 6)
    np.testing.assert_array_equal(m.read(1, 2, 3, 4), expected(1, 2, 3, 4))
    m.write(3, 4, np.array([[1, 2], [3, 4]], dtype=np.int32))
    assert m.data[4, 5] == 4
    # A list is converted.
    m.write(0, 0, [[7, 8, 9]])
    np.testing.assert_array_equal(m.data[0, :3], [7, 8, 9])
    # Strided input (every other column of a bigger array), no copy needed
    # on the python side.
    big = np.arange(20, dtype=np.int32).reshape(2, 10)
    m.write(0, 0, big[:, ::2])
    np.testing.assert_array_equal(m.read(0, 0, 2, 5), big[:, ::2])
    with pytest.raises(RuntimeError, match="outside the image"):
        m.read(4, 0, 2, 1)
    assert str(m) == "MemoryRasterSample 5 x 6"
    assert MemoryRasterSample(2, 3, 5).read(0, 0, 2, 3).sum() == 30


def test_buffer_contiguous_and_strided():
    b = BufferRasterSample(6, 8)
    np.testing.assert_array_equal(b.read(1, 2, 3, 4), expected(1, 2, 3, 4))
    assert b.number_copy == 0
    # read_into a writable numpy array: C order goes straight to the "C
    # library", a column slice is copied through a temporary.
    out = np.zeros((3, 4), dtype=np.int32)
    b.read_into(1, 2, out)
    np.testing.assert_array_equal(out, expected(1, 2, 3, 4))
    assert b.number_copy == 0
    big = np.full((3, 10), -1, dtype=np.int32)
    b.read_into(0, 0, big[:, 1:9:2])
    np.testing.assert_array_equal(big[:, 1:9:2], expected(0, 0, 3, 4))
    assert big[0, 0] == -1 and big[0, 2] == -1
    assert b.number_copy == 1
    f = np.zeros((3, 4), dtype=np.int32, order="F")
    b.read_into(1, 2, f)
    np.testing.assert_array_equal(f, expected(1, 2, 3, 4))
    assert b.number_copy == 2
    # Writes, contiguous and strided (a transpose).
    b.write(0, 0, np.array([[1, 2], [3, 4]], dtype=np.int32))
    assert b.number_copy == 2
    b.write(4, 6, np.array([[1, 2], [3, 4]], dtype=np.int32).T)
    assert b.number_copy == 3
    np.testing.assert_array_equal(b.read(4, 6, 2, 2), [[1, 3], [2, 4]])


def test_read_into_rejects():
    b = BufferRasterSample(6, 8)
    # The output must be a writable int32 array: anything else would be
    # converted to a copy, filled, and thrown away.
    with pytest.raises(TypeError):
        b.read_into(0, 0, np.zeros((2, 2), dtype=np.float64))
    ro = np.zeros((2, 2), dtype=np.int32)
    ro.flags.writeable = False
    with pytest.raises(TypeError):
        b.read_into(0, 0, ro)
    with pytest.raises(TypeError):
        b.read_into(0, 0, [[0, 0], [0, 0]])
    with pytest.raises(TypeError, match="negative strides"):
        b.read_into(0, 0, np.zeros((2, 2), dtype=np.int32)[::-1])
    with pytest.raises(RuntimeError, match="outside the image"):
        b.read_into(5, 0, np.zeros((2, 2), dtype=np.int32))


def test_read_with_pad():
    b = BufferRasterSample(6, 8)
    r = b.read_with_pad(-1, -2, 3, 4, -7)
    np.testing.assert_array_equal(
        r, [[-7, -7, -7, -7], [-7, -7, 0, 1], [-7, -7, 1000, 1001]]
    )
    np.testing.assert_array_equal(b.read_with_pad(10, 10, 2, 2, 3), np.full((2, 2), 3))


def test_sub_keeps_full_alive():
    s = SubRasterSample(BufferRasterSample(6, 8), 2, 3, 3, 4)
    gc.collect()
    np.testing.assert_array_equal(s.read(0, 0, 3, 4), expected(2, 3, 3, 4))
    assert isinstance(s.full, BufferRasterSample)
    assert isinstance(s, RasterSample)
    with pytest.raises(RuntimeError):
        SubRasterSample(BufferRasterSample(6, 8), 4, 0, 3, 1)


def test_python_calc():
    c = PyCalc()
    np.testing.assert_array_equal(c.read(1, 2, 3, 4), expected(1, 2, 3, 4))
    assert c.ncalc == 1
    np.testing.assert_array_equal(c.read_with_pad(-1, 0, 2, 2), [[0, 0], [0, 1]])
    # Read only.
    with pytest.raises(RuntimeError, match="read only"):
        c.write(0, 0, np.zeros((1, 1), dtype=np.int32))


def test_python_calc_through_cpp():
    # C++ holds the python object (SubRasterSample) and calls calc
    # through it, also after python dropped its own reference; and the
    # tiled copy calls it once per tile.
    s = SubRasterSample(PyCalc(), 1, 1, 5, 5)
    gc.collect()
    np.testing.assert_array_equal(s.read(1, 1, 2, 2), expected(2, 2, 2, 2))
    assert isinstance(s.full, PyCalc)
    c = PyCalc()
    m = MemoryRasterSample(7, 9)
    copy_raster(c, m, 3, 4)
    np.testing.assert_array_equal(m.data, expected(0, 0, 7, 9))
    assert c.ncalc == 9


def test_python_calc_errors():
    class NoCalc(CalcRasterSample):
        pass

    with pytest.raises(RuntimeError):
        NoCalc(2, 2).read(0, 0, 1, 1)

    class Raises(CalcRasterSample):
        def calc(self, lstart, sstart):
            raise ValueError("calc failed")

    # A python exception in calc comes back unchanged.
    with pytest.raises(ValueError, match="calc failed"):
        Raises(2, 2).read(0, 0, 1, 1)
