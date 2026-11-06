# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Current work: modernization (read first)

A modernization of this repository is in progress. It rehearses the steps planned for geocal (and later framework):
C++20, Boost → std, `std::shared_ptr`, and replacing SWIG with nanobind.
- **Plan of record and handoff notes: `claude_notes/implementation_plan.md`.** Start with its §0 ("Handoff: where
  to pick up"), which has the current status, the progress log, the next step, and the locations of related repos
  and build areas. Update the progress log there as steps complete.
- **Working on geocal (or framework) and reviewing this repository?** §0 of that plan has a "Notes for geocal"
  table of fixes found here that geocal should also make (e.g. `python-install` uninstall-before-install, the
  `"blah"` printing bug). Check each one against geocal and apply it there or record why not.
- Work on the **`modernize`** branch. Don't modify `master`.
- **Stage changes with `git add`; never commit or push.** The maintainer reviews and commits.
- Don't `git subtree push` to `swig-rules-repo`; this repo's `swig_rules/` is allowed to drift from it.
- From phase S4 until phase S6 the Python bindings are expected to be broken (build with `--without-swig`, run the
  C++ tests with `make skeleton-check`). This is intentional; see plan §8.
- Code work starts only when the maintainer says so.
- If the bindings move to nanobind, the names `swig-rules-skeleton`, `swig_rules/` and `skeleton_swig` are kept
  for historical reasons.

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
- `--without-boost-serialization` — disable boost-serialization support code. This builds only the C++ library and
  its tests (check with `make skeleton-check`); the Python extension needs serialization, so plain `make` fails.
- `--enable-python-editable-mode` — `make install` installs the pure python code in `python/` as an editable pip
  install (`pip install -e`) instead of copying it. Can also be selected for one run with
  `make install PYTHON_EDITABLE_MODE=t` (use `PYTHON_EDITABLE_MODE=` to force a normal install when configure enabled
  it). The SWIG wrappers (`.so`) are always a full install. The source tree must stay in place.

Building requires: boost (>=1.46.0), blitz, SWIG (maintainer mode + `HAVE_SWIG`), and if generating SWIG
wrappers, doxygen. `configure` (via `AFIDS_PYTHON`, `config/m4/afids_python.m4`) also requires numpy, scipy,
matplotlib, sphinx, and sqlite3 to be importable by the configured Python, plus a `pytest`/`py.test` program
on `PATH`. A pixi-based dev environment is provided at `env/linux/pixi.toml` (python, boost, blitz, compilers,
doxygen, swig, packaging, numpy, scipy, matplotlib, sphinx, pytest) as a convenient way to get a working
toolchain, e.g. for testing against a different SWIG version.

### Python version compatibility note

`config/m4/ac_python.m4`, `config/m4/ac_python_module_with_version.m4`, and `config/m4/afids_python.m4` used
to shell out to `distutils` (version comparison, `sysconfig`, numpy include dirs) — this breaks entirely on
Python >=3.12, which removed `distutils` from the stdlib. These were rewritten to use `sys.version_info`
(interpreter version check), `packaging.version.Version` (third-party module version checks — the actual
PEP 440-compliant replacement for `distutils.version.LooseVersion`), stdlib `sysconfig.get_path(...)`
(replacing `distutils.sysconfig.get_python_lib(...)`), and `numpy.get_include()` (replacing
`numpy.distutils.misc_util.get_numpy_include_dirs()`). `config/m4/ac_python_pytest.m4` was also updated to
look for `pytest` before falling back to the old `py.test` script name, since pytest >=6 no longer installs
`py.test`. `packaging` is now a required dependency for running `configure`; it's declared in
`env/linux/pixi.toml`. These `config/m4/*.m4` files are themselves a `git subtree` from a separate shared
repo (`general-autoconf`, remote `autoconf-repo`) — after editing them, re-run `./bootstrap` to regenerate
`configure`/`aclocal.m4`, and consider pushing the fix upstream via `git subtree push --prefix config/m4
autoconf-repo master` since other projects built on the same macros hit the identical distutils breakage.

After configure, `source script/setup_skeleton.sh` (generated from `script/setup_skeleton.sh.in`) sets
`PYTHONPATH`/`PATH`/`SKELETONTOP` so the built library and python bindings are importable — this is done
automatically by the test scripts below but is needed if running things manually.

### Machines and paths

Work happens on two machines with different paths: `rifle` (work) and `frenchpeas` (home). Tell them apart by
`hostname`, or just check which path exists, since only one is valid on each machine. Paths in `claude_notes/` were
mostly written on rifle; the full mapping (this repo, scratch builds, geocal, framework and their builds) is in the
Locations table in §0 of `claude_notes/implementation_plan.md`.

| | rifle | frenchpeas |
|---|---|---|
| This repo | `/home/smyth/Local/swig-rules-skeleton` | `/data2/smythdata/swig-rules-skeleton` |
| Scratch build | `/home/smyth/Local/swig-rules-skeleton-build/build-pixi` | `/data2/smythdata/swig-rules-skeleton-build/build` |
| geocal | `/ssdata/smyth/geocal-repo` | `/data2/smythdata/geocal-repo` |
| framework | `/home/smyth/Local/framework` | `/data2/smythdata/framework` |

### Scratch build/test environment

The scratch build directory above is a pixi environment (outside this repo) that can be used to actually
configure/build/run tests against, e.g. (from inside it):
```sh
../../swig-rules-skeleton/configure --prefix=$CONDA_PREFIX --enable-maintainer-mode --enable-debug
```
It's fine to modify this environment's `pixi.toml`/add packages to it as needed to get a build working — it's
disposable scratch space, not something to be precious about.

## Formatting

C++ (`.h`, `.cc` in `lib/` and `swig_rules/lib/`) is formatted with clang-format, using `.clang-format` at the
repository root (clang-format 22 from the pixi env, pinned). Format what you change: `clang-format -i <files>`, or
`make format` for everything; `make format-check` only checks, and `make lint` runs clang-tidy with the checks in
`.clang-tidy` (fails on any finding; also writes the clangd `compile_commands.json`).
`.clang-format-ignore` excludes the generated wrappers (`bindings/`) and copies of upstream code
(`swig_rules/lib/serialize_shared_ptr.h`, `python_lib_init.h`), which keep their upstream style. `.i` and `.sample` files
aren't formatted by clang-format; keep them in the same style by hand. Python uses `ruff format`.

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
# e.g. python/test.sh tests/unit/director_example_test.py
```
`make check` runs the python suite in parallel via `pytest -n $(PYTEST_NJOB)` against `python/tests` (default 10
workers; override with `PYTEST_NJOB=`, extra flags via `PYTEST_EXTRAFLAGS=` e.g. `--instafail` or `--timeout=20`).

The pure-python side of `python/` is laid out like a normal pip package rather than being listed file-by-file
in `.am` files: source lives in `python/skeleton/` (the importable `skeleton` package — glob-imports its own
`*.py` siblings plus `from skeleton_swig.all import *`), and tests live in `python/tests/` (`unit/` for the test
files, `fixtures/` for pytest fixtures wired in via `tests/conftest.py`'s `pytest_plugins`). `python/setup.py` +
`python/pyproject.toml` make `skeleton` pip-installable (`INSTALL_EXEC_HOOK` in `python.am` runs
`pip install --prefix=@prefix@ python` as part of `make install`); `python/Makefile` has dev-convenience
targets (`install`, `check`, `lint`, `format`, `mypy`) for working on the python code directly with pip/pytest
instead of going through the top-level autotools build.

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
- `lib/director_example.{h,cc,i}` and `python/tests/unit/director_example_test.py` are the reference example/test
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

## Claude-generated documentation

Place any documentation you generate (reviews, analysis notes, design writeups, etc.) in `claude_notes/`
unless the user directs you to put it somewhere else.
