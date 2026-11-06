#!/usr/bin/env python3
"""Write compile_commands.json for clang-tidy/clangd from `make -n`.

Run from the build directory:

    python <srcdir>/script/make_compile_commands.py [--clangd] [make targets...]

The default targets are the skeleton's library and C++ unit test program
(libskeleton.la skeleton_test_all); other projects pass their own (geocal:
libgeocal.la geocal_test_all _swig_wrap.la). The same file is used in
swig-rules-skeleton and geocal.

--clangd adds the flags the conda clang needs to parse code built with the
conda gcc (the same ones script/run_clang_tidy.sh passes): the conda gcc
install and sysroot, and a shim for blitz's missing llvm config (written to
clangd_shim/ in the build directory). clang's builtin headers come from the
"clang-22" conda package. Use
this for clangd (an editor or the Claude Code clangd plugin), and link the
file into the source tree so clangd finds it:

    ln -sf $PWD/compile_commands.json <srcdir>/compile_commands.json This
doesn't need bear or intercept-build (bear's conda build needs a newer glibc
than some of our hosts have), it just reads the compile lines make would run.
"""
import json
import os
import re
import shlex
import subprocess
import glob
import sys

# gcc-only options that clang rejects with "unknown argument" (conda's debug
# flags have -fvar-tracking-assignments). Add more here if they show up.
GCC_ONLY = re.compile(r"^-f(no-)?var-tracking.*$")

args_in = sys.argv[1:]
clangd = "--clangd" in args_in
args_in = [a for a in args_in if a != "--clangd"]
targets = args_in or ["libskeleton.la", "skeleton_test_all"]

extra = []
if clangd:
    prefix = os.environ.get("CONDA_PREFIX")
    if not prefix:
        sys.exit("--clangd: run inside the pixi environment")
    gccdir = sorted(glob.glob(f"{prefix}/lib/gcc/x86_64-conda-linux-gnu/*"))[-1]
    shim = os.path.abspath("clangd_shim")
    os.makedirs(f"{shim}/blitz/llvm", exist_ok=True)
    with open(f"{shim}/blitz/llvm/bzconfig.h", "w") as fh:
        fh.write("#include <blitz/gnu/bzconfig.h>\n")
    extra = [
        f"--gcc-install-dir={gccdir}",
        f"--sysroot={prefix}/x86_64-conda-linux-gnu/sysroot",
        f"-isystem{shim}",
    ]
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
    # The compiler may have a prefix, e.g. conda's activated compiler is
    # x86_64-conda-linux-gnu-c++.
    compiler = os.path.basename(args[0])
    if not srcs or not compiler.endswith(("g++", "gcc", "c++", "cc", "clang", "clang++")):
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
        elif GCC_ONLY.match(a):
            # gcc options clang doesn't know; clang refuses the whole file.
            pass
        else:
            keep.append(a)
    f = os.path.abspath(srcs[-1])
    entries.append({"directory": cwd, "file": f,
                    "arguments": keep[:1] + extra + keep[1:]})
with open("compile_commands.json", "w") as fh:
    json.dump(entries, fh, indent=1)
print(f"wrote {len(entries)} entries to {cwd}/compile_commands.json")
