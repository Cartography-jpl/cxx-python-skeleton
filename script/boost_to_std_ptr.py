#!/usr/bin/env python
"""Rewrite boost smart pointers to the std ones in C++ source.

    script/boost_to_std_ptr.py [--dry-run] [--swig] PATH...

PATH can be files or directories (searched recursively). By default the C++
files are changed (.h .hh .hpp .cc .cpp .cxx and the same with .sample or .in
appended); --swig adds SWIG .i files. Generated SWIG wrappers (*_wrap.cc,
*_wrap.h) are always skipped.

Rewritten:
  boost::shared_ptr, weak_ptr, make_shared, allocate_shared,
  enable_shared_from_this, bad_weak_ptr, owner_less, and the
  dynamic/static/const/reinterpret_pointer_cast functions -> std::
  (also the ::boost:: spelling);
  #include <boost/shared_ptr.hpp> and the other smart_ptr headers ->
  #include <memory> (once per file).

Not rewritten, reported instead: boost::scoped_ptr, intrusive_ptr,
shared_array, BOOST_SP_* macros, and "using boost::shared_ptr". These need a
person to look at them.

Written for the skeleton's switch to std::shared_ptr, to be reused unchanged
for geocal and framework. Prints one line per
changed file and a summary; --dry-run changes nothing.
"""

import argparse
import re
import sys
from pathlib import Path

NAMES = [
    "shared_ptr",
    "weak_ptr",
    "make_shared",
    "allocate_shared",
    "enable_shared_from_this",
    "bad_weak_ptr",
    "owner_less",
    "dynamic_pointer_cast",
    "static_pointer_cast",
    "const_pointer_cast",
    "reinterpret_pointer_cast",
]
NAME_RE = re.compile(r"(?<![\w:])(::)?boost::(" + "|".join(NAMES) + r")\b")

HEADERS = [
    "shared_ptr.hpp",
    "weak_ptr.hpp",
    "make_shared.hpp",
    "enable_shared_from_this.hpp",
    "pointer_cast.hpp",
    "smart_ptr.hpp",
    "smart_ptr/shared_ptr.hpp",
    "smart_ptr/weak_ptr.hpp",
    "smart_ptr/make_shared.hpp",
    "smart_ptr/enable_shared_from_this.hpp",
]
INCLUDE_RE = re.compile(
    r'^[ \t]*#[ \t]*include[ \t]*[<"]boost/('
    + "|".join(re.escape(h) for h in HEADERS)
    + r')[>"][^\n]*\n',
    re.M,
)
MEMORY_RE = re.compile(r"^[ \t]*#[ \t]*include[ \t]*<memory>", re.M)
REPORT_RE = re.compile(
    r"boost::(scoped_ptr|scoped_array|intrusive_ptr|shared_array)\b|BOOST_SP_\w+|"
    r"using\s+boost::(" + "|".join(NAMES) + r")\b"
)

CXX_SUFFIXES = {".h", ".hh", ".hpp", ".cc", ".cpp", ".cxx"}


def wanted(p: Path, swig: bool) -> bool:
    name = p.name
    if name.endswith(("_wrap.cc", "_wrap.h", "_wrap.cxx")):
        return False
    for extra in (".sample", ".in"):
        if name.endswith(extra):
            name = name[: -len(extra)]
    suffix = Path(name).suffix
    return suffix in CXX_SUFFIXES or (swig and suffix == ".i")


def convert(text: str) -> tuple[str, int]:
    n = 0

    def repl(m):
        nonlocal n
        n += 1
        return f"{m.group(1) or ''}std::{m.group(2)}"

    text = NAME_RE.sub(repl, text)
    # Replace the first smart pointer include by <memory> (unless the file
    # already has it), drop the rest.
    first = True
    have_memory = MEMORY_RE.search(text) is not None

    def inc(m):
        nonlocal first, n
        n += 1
        if first and not have_memory:
            first = False
            return "#include <memory>\n"
        return ""

    text = INCLUDE_RE.sub(inc, text)
    return text, n


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("paths", nargs="+", type=Path)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--swig", action="store_true", help="also change .i files")
    args = ap.parse_args()
    files = []
    for p in args.paths:
        if p.is_dir():
            files.extend(
                f for f in sorted(p.rglob("*")) if f.is_file() and wanted(f, args.swig)
            )
        elif wanted(p, args.swig) or p.is_file():
            files.append(p)
    nfile = nchange = 0
    for f in files:
        text = f.read_text()
        new, n = convert(text)
        for ln, line in enumerate(new.splitlines(), 1):
            if REPORT_RE.search(line):
                print(f"{f}:{ln}: needs a look: {line.strip()}")
        if n:
            nfile += 1
            nchange += n
            print(f"{f}: {n} changes")
            if not args.dry_run:
                f.write_text(new)
    print(
        f"{nchange} changes in {nfile} of {len(files)} files"
        + (" (dry run)" if args.dry_run else "")
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
