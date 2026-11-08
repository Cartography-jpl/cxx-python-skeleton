"""Python side of the nanobind rules (cxx_python_rules/nanobind/cxx_python_rules_nb.h).

Installed as <package>._nanobind_support next to the extension module. The
functions here are what pickles refer to, so keep their names."""

import contextlib
import copyreg
import io
import os
import pickle
from collections.abc import Iterator
from typing import Any

# _bindings is imported inside the functions that use it, not here: the
# extension module imports this module while it is being initialized
# (init_statics in cxx_python_rules_nb.h), before _bindings can be
# imported.


def _new_from_serialization(data: bytes) -> Any:
    from . import _bindings

    return _bindings.serialize_read_binary_generic_string(data)


def _new_from_serialization_dir(directory: str, data: bytes) -> Any:
    from . import _bindings

    # Relative file names in the archive are relative to directory (the
    # directory the object was pickled in), through the archive base
    # directory rather than by changing the current directory.
    return _bindings.serialize_read_binary_generic_string(data, directory)


@contextlib.contextmanager
def archive_base_directory_scope(
    directory: str | os.PathLike[str],
) -> Iterator[None]:
    """Set the archive base directory (serialize_function.h) for this thread
    inside the with block: relative file names in archives loaded there,
    including pickles, are relative to directory."""
    from . import _bindings

    prev = _bindings.set_archive_base_directory(os.fspath(directory))
    try:
        yield
    finally:
        _bindings.set_archive_base_directory(prev)


def _new_from_init(cls: type, version: int, *args: Any) -> Any:
    """For use with pickle, covers the common case where we just store the
    arguments needed to create an object."""
    if cls.pickle_format_version() != version:  # type: ignore[attr-defined]
        raise RuntimeError(
            f"Class is expecting a pickled object with version number "
            f"{cls.pickle_format_version()}, but we found {version}"  # type: ignore[attr-defined]
        )
    return cls(*args)


def _new_vector(cls: type, version: int, lst: list[Any]) -> Any:
    """Create a vector from a list."""
    if cls.pickle_format_version() != version:  # type: ignore[attr-defined]
        raise RuntimeError(
            f"Class is expecting a pickled object with version number "
            f"{cls.pickle_format_version()}, but we found {version}"  # type: ignore[attr-defined]
        )
    inst = cls()
    for i in lst:
        inst.append(i)  # type: ignore[attr-defined]
    return inst


# Python subclasses of C++ classes inside a boost archive. The archive has
# the C++ part, plus a pickle of the python class and __dict__ (the
# "python_object" field). SWIG wrote that as a pickle of the object with
# its "this" set to None, i.e. copyreg._reconstructor(cls, object, None)
# plus the __dict__ as state. We write the same form, so archives written
# by either can be read.


class _DirectorState:
    def __init__(self, cls: type, state: dict[str, Any]) -> None:
        self.cls = cls
        self.state = state

    def __reduce__(self) -> Any:
        return (copyreg._reconstructor, (self.cls, object, None), self.state)  # type: ignore[attr-defined]


class _Placeholder:
    def __init__(self, cls: type) -> None:
        self.cls = cls
        self.state: dict[str, Any] = {}

    def __setstate__(self, state: dict[str, Any]) -> None:
        self.state = state


def _reconstructor(cls: type, base: type, state: Any) -> _Placeholder:
    return _Placeholder(cls)


class _Unpickler(pickle.Unpickler):
    def find_class(self, module: str, name: str) -> Any:
        if (module, name) == ("copyreg", "_reconstructor"):
            return _reconstructor
        return super().find_class(module, name)


def director_dumps(obj: Any) -> bytes:
    state = dict(getattr(obj, "__dict__", {}))
    return pickle.dumps(_DirectorState(type(obj), state))


def director_loads(data: bytes) -> tuple[type, dict[str, Any]]:
    p = _Unpickler(io.BytesIO(data)).load()
    state = dict(p.state or {})
    # Written by SWIG, always None
    state.pop("this", None)
    return p.cls, state
