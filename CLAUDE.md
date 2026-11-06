# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Current work: modernization (read first)

A modernization of this repository is in progress. It rehearses the steps planned for geocal (and later framework):
C++20 (now C++23), Boost → std, `std::shared_ptr`, and replacing SWIG with nanobind.
- **Plan of record and handoff notes: `claude_notes/implementation_plan.md`.** Start with its §0 ("Handoff: where
  to pick up"), which has the current status, the progress log, the next step, and the locations of related repos
  and build areas. Update the progress log there as steps complete.
- **Working on geocal (or framework) and reviewing this repository?** §0 of that plan has a "Notes for geocal"
  table of fixes found here that geocal should also make (e.g. `python-install` uninstall-before-install, the
  `"blah"` printing bug). Check each one against geocal and apply it there or record why not.
- Work on the **`modernize`** branch. Don't modify `master`.
- **Stage changes with `git add`; never commit or push.** The maintainer reviews and commits.
- Don't `git subtree push` to `cxx-python-rules-repo`; this repo's `swig_rules/` is allowed to drift from it.
- Phases S4–S6 are done: the bindings are nanobind now (SWIG was removed in S6.6). S7 (C++23, minimum still gcc 14;
  an `mdspan` sample for raw pointer plus size interfaces) is done too, see the plan. Use only library
  features gcc 14 has (no range formatting, no `std::mdspan`: use `Skeleton::md`). The repository
  was renamed from `swig-rules-skeleton` to `cxx-python-skeleton` (and `swig-rules` to `cxx-python-rules`); the names
  `swig_rules/` and `skeleton_swig` are kept for historical reasons.
- Code work starts only when the maintainer says so.

## What this repo is

This is a skeleton/example repository demonstrating use of the
[cxx-python-rules](https://github.jpl.nasa.gov/Cartography/cxx-python-rules) subtree — a shared, reusable set of
autoconf/automake rules and (since S6) nanobind support code for wrapping C++ in Python. The `swig_rules/` directory
is *not* standalone code owned by this repo; it's pulled in from the separate `cxx-python-rules` repository via
`git subtree` and is meant to be included in a larger C++/Python project (e.g. the `geocal` repository, a
"full" example of this pattern). The rest of the repo (`lib/`, `python/`) is sample/test code showing the
pattern in use, and is expected to be deleted/replaced when this skeleton is used to bootstrap a new project.

## Build system

Autotools (autoconf/automake) is the primary build. A small CMake reference build (`CMakeLists.txt`: library,
python module, tests) exists as an example for CMake projects such as framework.

```sh
./bootstrap                 # regenerate configure/Makefile.in from .am/.ac files (autoreconf)
./configure                 # standard autotools configure
make                         # build the C++ library and the nanobind python extension module
make check                   # C++ unit tests, then the python tests against the build tree (no install needed)
make skeleton-check           # C++-only unit tests (faster loop)
make installcheck             # the python tests against the installed package
```

Useful `configure` options:
- `--without-python-bindings` — build only the C++ library and its tests (also automatic without nanobind).
- `--without-documentation` — skip doxygen (the python docstrings then come out empty; they are generated from the
  doxygen XML of the headers by `swig_rules/nanobind/doxy2nanobind.py`).
- `--without-boost-serialization` — disable boost-serialization support code. This builds only the C++ library and
  its tests (the python extension needs serialization for pickling, so it isn't built).
- `--enable-python-editable-mode` — `make install` installs the pure python code in `python/` as an editable pip
  install (`pip install -e`) instead of copying it. Can also be selected for one run with
  `make install PYTHON_EDITABLE_MODE=t` (use `PYTHON_EDITABLE_MODE=` to force a normal install when configure enabled
  it). The extension modules (`.so`) are always a full install. The source tree must stay in place.

Building requires: boost (>=1.84), blitz, nanobind (found through the configured python, `config/m4/ac_nanobind.m4`),
mdspan (conda-forge `mdspan`, pinned snapshot) and doxygen for the docstrings. `configure` (via `AFIDS_PYTHON`,
`config/m4/afids_python.m4`) also requires numpy, scipy, matplotlib, sphinx, and sqlite3 to be importable by the
configured Python, plus a `pytest`/`py.test` program on `PATH`. A pixi-based dev environment is provided at
`env/linux/pixi.toml` as a convenient way to get a working toolchain.

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
| This repo | `/home/smyth/Local/cxx-python-skeleton` | `/data2/smythdata/cxx-python-skeleton` |
| Scratch build | `/home/smyth/Local/cxx-python-skeleton-build/build-pixi` | `/data2/smythdata/cxx-python-skeleton-build/build` |
| geocal | `/ssdata/smyth/geocal-repo` | `/data2/smythdata/geocal-repo` |
| framework | `/home/smyth/Local/framework` | `/data2/smythdata/framework` |

### Scratch build/test environment

The scratch build directory above is a pixi environment (outside this repo) that can be used to actually
configure/build/run tests against, e.g. (from inside it):
```sh
../../cxx-python-skeleton/configure --prefix=$CONDA_PREFIX --enable-maintainer-mode --enable-debug
```
It's fine to modify this environment's `pixi.toml`/add packages to it as needed to get a build working — it's
disposable scratch space, not something to be precious about.

## Formatting

C++ (`.h`, `.cc` in `lib/`, `swig_rules/lib/` and `swig_rules/nanobind/`) is formatted with clang-format, using `.clang-format` at the
repository root (clang-format 22 from the pixi env, pinned). Format what you change: `clang-format -i <files>`, or
`make format` for everything; `make format-check` only checks, and `make lint` runs clang-tidy with the checks in
`.clang-tidy` (fails on any finding; also writes the clangd `compile_commands.json`).
`.clang-format-ignore` excludes copies of upstream code (`swig_rules/lib/serialize_shared_ptr.h`,
`swig_rules/nanobind/gc_shared_ptr.h`), which keep their upstream style. `.sample` files aren't formatted by
clang-format; keep them in the same style by hand. Python uses `ruff format`.

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
`make check` runs the python suite against the build tree (the modules from `.libs`, put together in `nb_pkg/`), and
`make installcheck` against the installed package, both in parallel via `pytest -n $(PYTEST_NJOB)` against
`python/tests` (default 10 workers; override with `PYTEST_NJOB=`, extra flags via `PYTEST_EXTRAFLAGS=` e.g.
`--instafail` or `--timeout=20`).

The pure-python side of `python/` is laid out like a normal pip package rather than being listed file-by-file
in `.am` files: source lives in `python/skeleton/` (the importable `skeleton` package — glob-imports its own
`*.py` siblings plus `from skeleton_swig.all import *`), and tests live in `python/tests/` (`unit/` for the test
files, `fixtures/` for pytest fixtures wired in via `tests/conftest.py`'s `pytest_plugins`). `python/setup.py` +
`python/pyproject.toml` make `skeleton` pip-installable (`INSTALL_EXEC_HOOK` in `python.am` runs
`pip install --prefix=@prefix@ python` as part of `make install`); `python/Makefile` has dev-convenience
targets (`install`, `check`, `lint`, `format`, `mypy`) for working on the python code directly with pip/pytest
instead of going through the top-level autotools build.

## Architecture: the nanobind bindings

- The bindings are `lib/*_nb.cc`, one `init_<name>(nb::module_&)` per header group, called in dependency order from
  `lib/skeleton_nb_module.cc` (`skeleton_nb_init.inc`) to make one extension module, `skeleton_swig._swig_wrap`.
  Every binding file includes `lib/skeleton_nb_opaque.h` (the `nb::bind_vector` types), which includes
  `swig_rules/nanobind/swig_rules_nb.h` (the rules library). They were generated from the old `.i` files by
  `swig_rules/nanobind/i_to_nanobind.py` and are maintained by hand now. `lib/downstream_nb.cc` is a second module
  (`_skeleton_downstream`) that tests using the main module's types from another module.
- All C++ classes exposed to python should derive (usually virtually) from `GenericObject` (`lib/generic_object.h`),
  which has `std::enable_shared_from_this`. Each bound class calls `swig_rules_nb::register_type<T>()`: the type
  registry (`swig_rules/nanobind/type_registry.h`) and the forked `shared_ptr` caster (`gc_shared_ptr.h`) use it to
  return the most derived python type and to accept a derived object for a parameter typed as a secondary or virtual
  base. Never declare a virtual base as a nanobind base (crashes); see `swig_rules/nanobind/README.md`.
- Non-director classes are constructed with `nb::new_` + `std::make_shared`; director classes (python may subclass
  them) have a trampoline and `nb::init`.
- `Printable` (`lib/printable.h`): `print_to_string` is bound and used for `__str__`.
- numpy <-> `blitz::Array` goes through an `mdspan` view (`lib/skeleton_mdspan.h`, `lib/blitz_mdspan.h`, C++ tested in
  `blitz_mdspan_test.cc`; python side `swig_rules/nanobind/blitz_array.h`): zero copy for the right dtype, the blitz
  array keeps the numpy array alive. Index an mdspan with `m[i, j]` (C++23); inside a macro such as
  `BOOST_CHECK_EQUAL` it needs extra parentheses, `(m[i, j])`, since the comma splits the macro arguments.
- `lib/raster_sample.{h,cc}` is the sample for replacing a raw pointer plus sizes with an `mdspan` (geocal's
  `RasterImage::read_ptr` and `CalcRaster` patterns).
- Pickling uses boost serialization (`swig_rules_nb::pickle_serialization`, `__reduce__`); see
  `lib/skeleton_serialize_support.h` and the `*.sample` files under `swig_rules/lib/`.
- `swig_rules/lib/serialize_function.cc` and `weak_ptr_serialize_support.cc` are compiled into the C++ library with
  `-DSWIG_MAPPER_NAMESPACE=Skeleton`. The library stays free of python.

## Python subclasses of C++ classes

Read `swig_rules/lib/DirectorNotes.md` before touching anything related to trampolines, `shared_ptr` or `weak_ptr` in
the bindings. Summary:

- A python object owns its C++ object; passing it to C++ gives a `shared_ptr` that holds a python reference, so C++
  keeps the python object alive.
- A `weak_ptr` to an object owned only by python expires at once (no C++ owner); decided, see the notes.
- Python subclass objects can be in boost archives (`SWIG_RULES_NB_DIRECTOR_SERIALIZATION`); archives written by the
  SWIG bindings still load.
- `lib/director_example.{h,cc}`, `lib/director_example_nb.cc` and `python/tests/unit/director_example_test.py` are the
  reference example/tests; `nanobind_test.py` has the `weak_ptr` matrix.

## Pulling in swig_rules upstream changes

(Not done during the modernization: `swig_rules/` here is allowed to drift from `cxx-python-rules-repo`; geocal copies it
from this repository. Kept for reference.)

`swig_rules/` is a `git subtree`, not a submodule — its contents are actual files in this repo's history. To
sync with the upstream `cxx-python-rules` repo (added as remote `cxx-python-rules-repo`):

```sh
git subtree pull --prefix swig_rules cxx-python-rules-repo master     # pull upstream changes in
git subtree push --prefix swig_rules cxx-python-rules-repo master     # push local changes upstream
git fetch cxx-python-rules-repo && git diff HEAD:swig_rules cxx-python-rules-repo/master   # diff without merging
```
If subtree push complains "tip of current branch is behind" even after a pull, this is a known subtree quirk —
try `git push cxx-python-rules-repo $(git subtree split --prefix swig_rules master):master --force` (see
`README.git` for more detail/links).

## Claude-generated documentation

Place any documentation you generate (reviews, analysis notes, design writeups, etc.) in `claude_notes/`
unless the user directs you to put it somewhere else.
