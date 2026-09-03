# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

This is a skeleton/example repository demonstrating use of the
[swig-rules](https://github.jpl.nasa.gov/Cartography/swig-rules) subtree — a shared, reusable set of
autoconf/automake rules and SWIG `.i` include files for wrapping C++ in Python. The `swig_rules/` directory
is *not* standalone code owned by this repo; it's pulled in from the separate `swig-rules` repository via
`git subtree` and is meant to be included in a larger C++/Python project (e.g. the `geocal` repository, a
"full" example of this pattern). The rest of the repo (`lib/`, `python/`) is sample/test code showing the
pattern in use, and is expected to be deleted/replaced when this skeleton is used to bootstrap a new project.

## Build system

Autoconf/automake (autotools), not CMake for the primary build (a `CMakeLists.txt` also exists under
`swig_rules/` as an alternate packaging path, but autotools is the main build here).

```sh
./bootstrap                 # regenerate configure/Makefile.in from .am/.ac files (autoreconf)
./configure                 # standard autotools configure
make                         # build C++ library + SWIG python bindings
make check                   # build+run everything: C++ unit tests AND python pytest suite
make skeleton-check           # C++-only unit tests (faster loop when not touching SWIG/python)
```

Useful `configure` options:
- `--without-swig` — skip regenerating/compiling SWIG wrappers (faster edit/build loop when not touching `.i` files)
- `--without-documentation` — skip doxygen (also required if doxygen isn't installed and SWIG generation is on;
  doxygen output is fed into SWIG docstrings via `doxy2swig.py`)
- `--without-boost-serialization` — disable boost-serialization support code

Building requires: boost (>=1.46.0), blitz, SWIG (maintainer mode + `HAVE_SWIG`), and if generating SWIG
wrappers, doxygen. A pixi-based dev environment is provided at `env/linux/pixi.toml` (python, boost, blitz,
compilers, doxygen, swig<4.4.0) as a convenient way to get a working toolchain, e.g. for testing against a
different SWIG version.

After configure, `source script/setup_skeleton.sh` (generated from `script/setup_skeleton.sh.in`) sets
`PYTHONPATH`/`PATH`/`SKELETONTOP` so the built library and python bindings are importable — this is done
automatically by the test scripts below but is needed if running things manually.

## Tests

**C++ tests** (Boost.Test, binary `skeleton_test_all`, built from `lib/test_all.cc` + `*_test.cc` files):
```sh
lib/test_all.sh                              # runs all C++ unit tests (sources setup_skeleton.sh itself)
run_test=<expr> lib/test_all.sh               # run a subset (Boost.Test filter expression)
log_test=1 lib/test_all.sh                    # verbose per-test logging
valgrind=1 lib/test_all.sh                    # run under valgrind
gdb=1 lib/test_all.sh                         # run under gdb
```
Edit `lib/test_all.sh` locally for a fast inner loop, but it should be restored/left running all tests before
committing (per the comment header in that file).

**Python tests** (pytest, run from a directory with the environment sourced):
```sh
python/test.sh [pytest-args]                  # sets PYTHONPATH, runs py.test -rxXs --log-cli-level=info
```
`make check` runs the python suite in parallel via `pytest -n $(PYTEST_NJOB)` (default 10 workers; override with
`PYTEST_NJOB=`, extra flags via `PYTEST_EXTRAFLAGS=` e.g. `--instafail` or `--timeout=20`).

## Architecture: how SWIG wrapping is wired up

- Every `.i` file must `%include` (transitively via `skeleton_common.i`) the shared rules from
  `swig_rules/include/`. `skeleton_common.i` sets the SWIG module options (`directors="1"`,
  `allprotected="1"`) and must be included at the top of each interface file.
- All C++ classes exposed to python should derive (even indirectly) from `GenericObject`
  (`lib/generic_object.h`) — `shared_ptr_type_mapper.i` depends on this base class to support automatic
  downcasting to the most-derived python type. This is a convention, not a hard SWIG requirement — any class
  named `GenericObject` works, it doesn't have to be this exact sample.
- Classes that want C++ `operator<<` to work as python `__str__`/`print()` should derive from `Printable`
  (`lib/printable.h`), which swig_rules' `swig_print.i` maps `print_to_string` to `__str__`.
- `swig_type_mapper_base.cc` (from `swig_rules/lib/`) must be compiled into the parent library, with
  `-DSWIG_MAPPER_NAMESPACE=<YourNamespace>` defined.
- Optional serialization support: `lib/skeleton_serialize_support.h` + boost::serialization give python
  classes pickling support; see `*.sample` files under `swig_rules/lib/` for the pattern to copy into a new
  project.
- SWIG `.i` → `_wrap.cc` generation (only in maintainer mode with SWIG available, see `swig_rules/swig_rules.am`):
  for each `%module`, doxygen runs first (via `doxygen_single.cfg`) to extract docs, `doxy2swig.py` converts
  the doxygen XML to SWIG `%feature("docstring")` directives, then SWIG generates the wrapper `.cc`. All
  modules get compiled into **one** shared library, `_swig_wrap.la` (see the `moduleimport` override for
  SWIG >= 4 in `skeleton_common.i` — this is a deliberate departure from SWIG 4's default of one `.so` per
  module). `create_python_init.py` generates the combined `swig_wrap.cc` entry point from `swig_wrap.tmpl`.
- New source directories are declared in `configure.ac` via `SKELETON_SOURCE_DIRECTORY`
  (`config/m4_local/skeleton_source_directory.m4`) and wired into the build via `.am` files (see `lib/lib.am`,
  `python/python.am` for the pattern: `skeletoninc_HEADERS`, `libskeleton_la_SOURCES`, `SWIG_SRC`,
  `skeletonswiginc_HEADERS`).

## Director (python-subclass-of-C++-virtual-class) lifetime handling

This is the most subtle part of the codebase — read `swig_rules/lib/DirectorNotes.md` before touching
anything related to directors, shared_ptr, or weak_ptr in SWIG code. Summary:

- Python always owns director objects; lifetime is driven by the python object's refcount.
- When a director object is passed into C++, SWIG hands out a *second*, distinct `shared_ptr` (not the one
  behind the python `this`) that uses a custom deleter (`PythonRefPtrCleanup`) which decrements the python
  refcount instead of freeing the C++ object directly. This is implemented via a locally-patched
  `swig_rules/include/my_shared_ptr.i` (a fork of SWIG's `boost_shared_ptr.i` — diff against SWIG's version
  when upgrading SWIG to port forward any upstream fixes).
- Because a *different* `shared_ptr` is handed out each time, plain `weak_ptr` usage breaks (the "extra"
  shared_ptr hits refcount 0 even though the object is still alive via python). The `SHARED_PTR_NO_OWN`
  typemap trick exists specifically to get the shared_ptr backing a `weak_ptr` without breaking ownership —
  never persist a `SHARED_PTR_NO_OWN` value into a long-lived `shared_ptr` (this can segfault by design).
- `lib/director_example.{h,cc,i}` and `python/lib/director_example_test.py` are the reference example/test
  for this behavior — look here first when debugging a director lifetime issue.
- Recent history on this repo (see `git log`) has been about python exception propagation through directors
  (an exception thrown from a python override of a C++ virtual method) — when C++ calls into a director that
  raises, the original python exception should propagate, not get wrapped in a new one. Related code:
  `swig_rules/lib/python_exception.h`, `swig_rules/include/swig_exception.i`.

## Pulling in swig_rules upstream changes

`swig_rules/` is a `git subtree`, not a submodule — its contents are actual files in this repo's history. To
sync with the upstream `swig-rules` repo (added as remote `swig-rules-repo`):

```sh
git subtree pull --prefix swig_rules swig-rules-repo master     # pull upstream changes in
git subtree push --prefix swig_rules swig-rules-repo master     # push local changes upstream
git fetch swig-rules-repo && git diff HEAD:swig_rules swig-rules-repo/master   # diff without merging
```
If subtree push complains "tip of current branch is behind" even after a pull, this is a known subtree quirk —
try `git push swig-rules-repo $(git subtree split --prefix swig_rules master):master --force` (see
`README.git` for more detail/links).
