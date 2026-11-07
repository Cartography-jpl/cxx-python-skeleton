"""A python subclass object in an archive is pickled, and if it holds a C++
object, pickling that object writes a second archive in the middle of the
first. The second write must not disturb the first one's record of which
objects are in it, or weak_ptrs written after it are lost."""

import pytest

from skeleton import (
    DirectorExample,
    GenericObjectA,
    GenericObjectC,
    GenericObjectMap,
    serialize_read_binary_generic_string,
    serialize_read_binary_generic,
    serialize_read_generic,
    serialize_write,
    serialize_write_binary,
    serialize_write_binary_string,
)


class DirectorWithCxx(DirectorExample):
    def __init__(self, v, name, extra):
        super().__init__(v, name)
        # A C++ object, pickled with serialize_write_binary_string
        self.extra = extra


def make_map(with_nested):
    a = GenericObjectA(5)
    m = GenericObjectMap()
    m.set_generic("a", a)
    if with_nested:
        m.set_generic("b", DirectorWithCxx(1, "b", GenericObjectA(7)))
    # "c" is written after "b" (the map is sorted by key)
    m.set_generic("c", GenericObjectC(a))
    return m


def check_map(m2, with_nested):
    a2 = m2.get_generic("a")
    c2 = m2.get_generic("c")
    assert c2.a is not None
    assert c2.a.v == a2.v
    if with_nested:
        b2 = m2.get_generic("b")
        assert isinstance(b2, DirectorWithCxx)
        assert b2.extra.v == 7


def round_trip(m, how, tmp_path):
    if how == "binary":
        return serialize_read_binary_generic_string(serialize_write_binary_string(m))
    if how == "binary_file":
        f = str(tmp_path / "m.bin")
        serialize_write_binary(f, m)
        return serialize_read_binary_generic(f)
    f = str(tmp_path / "m.xml")
    serialize_write(f, m)
    return serialize_read_generic(f)


@pytest.mark.parametrize("how", ["binary", "binary_file", "xml_file"])
def test_weak_ptr_without_nesting(how, tmp_path):
    check_map(round_trip(make_map(False), how, tmp_path), False)


@pytest.mark.parametrize("how", ["binary", "binary_file", "xml_file"])
def test_weak_ptr_after_nested_pickle(how, tmp_path):
    check_map(round_trip(make_map(True), how, tmp_path), True)
