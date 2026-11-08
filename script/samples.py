#!/usr/bin/env python3
"""Write or check the .sample files from the live files they come from.

    script/samples.py [--check | --write]

The .sample files in cxx_python_rules/ are the project-specific files a
project such as geocal copies, with its own names. Each one is its live
file in lib/ or python/ with the names replaced (Skeleton -> GeoCal and so
on, see SUBSTITUTIONS). Edit the live file, then run --write; don't edit a
sample by hand.

Lines only a sample has (e.g. an include geocal still needs) go between
"// sample only:" and "// end sample only" ("#" in python), both lines
included. --check ignores those blocks; --write keeps each one, after the
same line as before.

--check (the default) exits 1 and shows a diff if a sample doesn't match.
"""

import argparse
import difflib
import sys
from pathlib import Path

TOP = Path(__file__).resolve().parent.parent

# live file -> sample, relative to the top of the repository.
PAIRS = [
    ("lib/generic_object.h", "cxx_python_rules/lib/generic_object.h.sample"),
    ("lib/generic_object.cc", "cxx_python_rules/lib/generic_object.cc.sample"),
    ("lib/generic_object_map.h", "cxx_python_rules/lib/generic_object_map.h.sample"),
    ("lib/generic_object_map.cc", "cxx_python_rules/lib/generic_object_map.cc.sample"),
    ("lib/printable.h", "cxx_python_rules/lib/printable.h.sample"),
    (
        "lib/blitz_array_serialize_support.h",
        "cxx_python_rules/lib/blitz_array_serialize_support.h.sample",
    ),
    (
        "lib/blitz_array_serialize_support.cc",
        "cxx_python_rules/lib/blitz_array_serialize_support.cc.sample",
    ),
    (
        "lib/skeleton_serialize_support.h",
        "cxx_python_rules/lib/serialize_support.h.sample",
    ),
    ("python/skeleton/swig_shelve.py", "cxx_python_rules/python/swig_shelve.py.sample"),
    (
        "python/tests/unit/swig_shelve_test.py",
        "cxx_python_rules/python/swig_shelve_test.py.sample",
    ),
]

# In this order (skeleton_cxx before skeleton).
SUBSTITUTIONS = [
    ("Skeleton", "GeoCal"),
    ("SKELETON", "GEOCAL"),
    ("skeleton_cxx", "geocal_cxx"),
    ("skeleton", "geocal"),
]


def substitute(text: str) -> str:
    for a, b in SUBSTITUTIONS:
        text = text.replace(a, b)
    return text


def is_begin(line: str) -> bool:
    return line.strip() in ("// sample only:", "# sample only:")


def is_end(line: str) -> bool:
    return line.strip() in ("// end sample only", "# end sample only")


def split_sample(
    lines: list[str], name: str
) -> tuple[list[str], list[tuple[int, list[str]]]]:
    """The sample without its sample-only blocks, and the blocks, each with
    the number of lines before it in the sample without blocks."""
    common: list[str] = []
    blocks: list[tuple[int, list[str]]] = []
    block: list[str] | None = None
    for i, line in enumerate(lines, 1):
        if block is None:
            if is_end(line):
                raise SystemExit(f"{name}:{i}: 'end sample only' without a start")
            if is_begin(line):
                block = [line]
            else:
                common.append(line)
        else:
            if is_begin(line):
                raise SystemExit(f"{name}:{i}: nested 'sample only'")
            block.append(line)
            if is_end(line):
                blocks.append((len(common), block))
                block = None
    if block is not None:
        raise SystemExit(f"{name}: 'sample only' block not closed")
    return common, blocks


def anchor(common: list[str], pos: int) -> tuple[str | None, int]:
    """The line before position pos, and which occurrence of that line it
    is (0 for the first). (None, 0) for the start of the file."""
    if pos == 0:
        return None, 0
    line = common[pos - 1]
    return line, common[:pos].count(line) - 1


def place_blocks(
    new: list[str],
    old_common: list[str],
    blocks: list[tuple[int, list[str]]],
    name: str,
) -> list[str]:
    """new with each block inserted after the line it followed in the old
    sample."""
    inserts: dict[int, list[str]] = {}
    for pos, block in blocks:
        line, n = anchor(old_common, pos)
        if line is None:
            at = 0
        else:
            found = [i for i, x in enumerate(new) if x == line]
            if len(found) <= n:
                raise SystemExit(
                    f"{name}: the line before a 'sample only' block is gone from "
                    f"the live file: {line!r}. Move the block by hand."
                )
            at = found[n] + 1
        inserts.setdefault(at, []).extend(block)
    res: list[str] = []
    for i in range(len(new) + 1):
        res.extend(inserts.get(i, []))
        if i < len(new):
            res.append(new[i])
    return res


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    g = parser.add_mutually_exclusive_group()
    g.add_argument("--check", action="store_true", help="check (the default)")
    g.add_argument("--write", action="store_true", help="write the samples")
    args = parser.parse_args()
    bad = 0
    for live, sample in PAIRS:
        new = substitute((TOP / live).read_text()).splitlines(keepends=True)
        sp = TOP / sample
        old = sp.read_text().splitlines(keepends=True) if sp.exists() else []
        old_common, blocks = split_sample(old, sample)
        if args.write:
            text = "".join(place_blocks(new, old_common, blocks, sample))
            if not sp.exists() or sp.read_text() != text:
                sp.write_text(text)
                print(f"wrote {sample}")
        elif old_common != new:
            bad += 1
            sys.stdout.writelines(
                difflib.unified_diff(
                    old_common,
                    new,
                    f"{sample} (without 'sample only' blocks)",
                    f"{live} (names replaced)",
                )
            )
    if bad:
        print(
            f"{bad} sample(s) differ from their live files: edit the live file, "
            "then run script/samples.py --write"
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
