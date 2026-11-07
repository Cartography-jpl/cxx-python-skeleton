#!/usr/bin/env python
"""Write a text snapshot of the python API of nanobind extension modules.

    api_snapshot.py skeleton_cxx._bindings skeleton_cxx._skeleton_downstream > api.txt

For every class and function in each module this writes, sorted: the kind
(class, function, method, static method, property), the nanobind signatures
(``__nb_signature__``: the signature text, the docstring and the default
values for each overload) and, for properties, the signatures of the getter
and setter. Comparing the snapshot before and after a change of the binding
code shows every change to the python API: names, keyword argument names,
defaults, types and docstrings.

The modules have to be importable (e.g. PYTHONPATH set to the build tree's
nb_pkg directory, or source setup_skeleton.sh).
"""

from __future__ import annotations

import importlib
import inspect
import sys
from typing import Any


def describe_callable(obj: Any) -> list[str]:
    sig = getattr(obj, "__nb_signature__", None)
    if sig is None:
        try:
            return [f"  signature: {inspect.signature(obj)}"]
        except (TypeError, ValueError):
            return [f"  doc: {obj.__doc__!r}"]
    res = []
    for text, doc, defaults in sig:
        res.append(f"  overload: {text}")
        if doc:
            res.append(f"    doc: {doc!r}")
        if defaults:
            res.append(f"    defaults: {[repr(d) for d in defaults]!r}")
    return res


def describe_member(name: str, obj: Any) -> list[str]:
    if isinstance(obj, property):
        res = [f" property {name}"]
        if obj.__doc__:
            res.append(f"  doc: {obj.__doc__!r}")
        for part in ("fget", "fset"):
            f = getattr(obj, part)
            if f is not None:
                res.append(f"  {part}:")
                res.extend("  " + s for s in describe_callable(f))
        return res
    if isinstance(obj, staticmethod):
        return [f" staticmethod {name}", *describe_callable(obj.__func__)]
    if isinstance(obj, classmethod):
        return [f" classmethod {name}", *describe_callable(obj.__func__)]
    if callable(obj):
        return [f" method {name}", *describe_callable(obj)]
    return [f" attribute {name} = {obj!r}"]


def describe_class(cls: type) -> list[str]:
    bases = ", ".join(b.__qualname__ for b in cls.__bases__)
    res = [f"class {cls.__qualname__}({bases})"]
    if cls.__doc__:
        res.append(f" doc: {cls.__doc__!r}")
    for name in sorted(vars(cls)):
        if name in ("__module__", "__doc__", "__dict__", "__weakref__"):
            continue
        res.extend(describe_member(name, vars(cls)[name]))
    return res


def snapshot(module_name: str) -> list[str]:
    m = importlib.import_module(module_name)
    res = [f"module {module_name}"]
    for name in sorted(vars(m)):
        if name.startswith("__"):
            continue
        obj = vars(m)[name]
        if inspect.isclass(obj):
            if obj.__module__ != m.__name__ and not obj.__module__.startswith(
                m.__name__.rsplit(".", 1)[0]
            ):
                res.append(f"imported class {name} from {obj.__module__}")
                continue
            res.extend(describe_class(obj))
        elif callable(obj):
            res.append(f"function {name}")
            res.extend(describe_callable(obj))
        elif inspect.ismodule(obj):
            res.append(f"module attribute {name} -> {obj.__name__}")
        else:
            res.append(f"attribute {name} = {obj!r}")
    return res


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__, file=sys.stderr)
        return 1
    for module_name in argv[1:]:
        print("\n".join(snapshot(module_name)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
