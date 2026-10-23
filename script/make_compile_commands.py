#!/usr/bin/env python3
"""Write compile_commands.json for clang-tidy/clangd from `make -n`.

Run from the build directory:

    python <srcdir>/script/make_compile_commands.py [make targets...]

The default targets are the library and the C++ unit test program. This
doesn't need bear or intercept-build (bear's conda build needs a newer glibc
than some of our hosts have), it just reads the compile lines make would run.
"""
import json
import os
import re
import shlex
import subprocess
import sys

targets = sys.argv[1:] or ["libskeleton.la", "skeleton_test_all"]
out = subprocess.run(
    ["make", "-n", "-B", *targets], capture_output=True, text=True, check=False
).stdout
out = out.replace("\\\n", " ")
cwd = os.getcwd()
entries = []
for line in out.splitlines():
    if " -c " not in line or "checking" in line:
        continue
    # Strip the libtool wrapper, and shell command substitution that
    # locates the source file (`test -f 'x.cc' || echo 'dir/'`x.cc).
    m = re.search(r"`test -f '([^']+)' \|\| echo '([^']*)'`(\S+)", line)
    if m:
        src = m.group(3)
        # make picks the build-dir copy if it exists, else the source dir.
        src = src if os.path.exists(src) else m.group(2) + src
        line = line[: m.start()] + src + line[m.end() :]
    line = re.sub(r"^.*?--mode=compile\s+", "", line)
    try:
        args = shlex.split(line)
    except ValueError:
        continue
    srcs = [a for a in args if re.search(r"\.(cc|cpp|cxx|c)$", a)]
    if not srcs or args[0] not in ("g++", "gcc", "c++", "cc") and "g++" not in args[0]:
        continue
    # Drop dependency tracking and output options clang doesn't need.
    keep, skip = [], 0
    for a in args:
        if skip:
            skip -= 1
        elif a in ("-MT", "-MF", "-o"):
            skip = 1
        elif a in ("-MD", "-MP"):
            pass
        else:
            keep.append(a)
    f = os.path.abspath(srcs[-1])
    entries.append({"directory": cwd, "file": f, "arguments": keep})
with open("compile_commands.json", "w") as fh:
    json.dump(entries, fh, indent=1)
print(f"wrote {len(entries)} entries to {cwd}/compile_commands.json")
