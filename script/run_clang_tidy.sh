#!/bin/sh
# Run clang-tidy from the conda/pixi environment on files in the skeleton.
#
#   cd <build dir>
#   python <srcdir>/script/make_compile_commands.py
#   <srcdir>/script/run_clang_tidy.sh --checks='modernize-use-nullptr' file.cc ...
#
# The conda clang needs a little help to parse code built for the conda gcc:
#  * point it at the gcc sysroot and the gcc builtin headers (stddef.h etc.)
#  * the conda blitz package only has blitz/gnu/bzconfig.h, and blitz picks
#    blitz/llvm/bzconfig.h for clang, so supply a shim that forwards to gnu.
# Extra arguments are passed to clang-tidy. The shim is created in a temp dir.
set -e
: "${CONDA_PREFIX:?run inside the pixi environment}"
gccdir=$(ls -d "$CONDA_PREFIX"/lib/gcc/x86_64-conda-linux-gnu/* | tail -1)
shim=$(mktemp -d)
trap 'rm -rf "$shim"' EXIT
mkdir -p "$shim/blitz/llvm"
echo '#include <blitz/gnu/bzconfig.h>' > "$shim/blitz/llvm/bzconfig.h"
# --gcc-install-dir: otherwise clang may pick a newer gcc installed on the
# host for the C++ standard library headers (seen with gcc 16 on Gentoo),
# which need a newer glibc than the conda sysroot has.
exec clang-tidy -p . \
  --extra-arg=--gcc-install-dir="$gccdir" \
  --extra-arg=--sysroot="$CONDA_PREFIX/x86_64-conda-linux-gnu/sysroot" \
  --extra-arg=-isystem"$gccdir/include" \
  --extra-arg=-isystem"$gccdir/include-fixed" \
  --extra-arg=-isystem"$shim" "$@"
