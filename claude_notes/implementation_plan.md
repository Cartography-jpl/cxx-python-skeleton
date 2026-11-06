# cxx-python-skeleton: implementation plan for the modernization

**Date:** 2026-10-23 (reviewed and updated 2026-10-30: geocal sync §2.5, test coverage phase ST §4a; 2026-11-05:
geocal/framework sync §2.6)
**Status:** In progress. Phase S0 is done; the next phase is ST (test hardening); see §0.
**Purpose:** Work through every modernization step planned for geocal (and later framework) on this small repository
first, so the problems are found and solved here before touching the large repositories.

Related documents:
* `claude_notes/modernization_ideas.md`, `claude_notes/serialization_options.md`, `claude_notes/build_layout_review.md`,
  `claude_notes/swig_rules_review.md`: the earlier discussion in this repository. Section 11 of
  `modernization_ideas.md` lists what has changed since; where they disagree, this plan wins.
* `/ssdata/smyth/geocal-repo/claude_documents/cpp_modernization_and_library_plan.md`: the geocal plan this mirrors
  (referred to below as "the geocal plan").
* `<geocal repo>/claude_documents/resampler_threading_plan.md`: geocal's Resampler speed-up and `ThreadPool` work
  (2026-11-03); the parts that affect this plan are in §2.6.
* framework (`/home/smyth/Local/framework`): surveyed read-only; it will follow later with the same steps.

## 0. Handoff: where to pick up (read this first)

**Status (2026-11-06, later):** **Phase S7 is done** (progress log; §11): C++23 with gcc 14 still the minimum
(decision 22), a raw pointer plus size → `mdspan` sample (`lib/raster_sample.{h,cc}`), and the old bootstrap warning
fixed. **Then the rename** (maintainer, 2026-11-06; "Naming" below). Both staged, not committed (back to the
normal commit policy after S4–S6). CMake set aside until framework. Next: geocal's own phases using the "Notes for
geocal" table.

**Naming (2026-11-06).** The repository no longer tests SWIG rules, so the SWIG-era names were changed (decision 23).
**This plan's history (everything above the rename row in the progress log, and the other `claude_notes/` files)
keeps the old names**; read them with this table:

| Old | New |
|---|---|
| repository `swig-rules-skeleton` | `cxx-python-skeleton` |
| `swig_rules/` (upstream repository `swig-rules`, remote `swig-rules-repo`) | `cxx_python_rules/` (`cxx-python-rules`, `cxx-python-rules-repo`) |
| python package `skeleton_swig`, extension module `_swig_wrap` | `skeleton_cxx`, `skeleton_cxx._bindings` |
| `swig_rules/nanobind/swig_rules_nb.{h,cc}`, namespace `swig_rules_nb`, macros `SWIG_RULES_NB_*` | `cxx_python_rules_nb.{h,cc}`, `cxx_python_rules_nb`, `CXX_PYTHON_RULES_NB_*` |
| `swig_rules/swig_rules.am`, autoconf `@swigrules@`, automake `pkgpythonswig*` | `cxx_python_rules.am`, `@cxxpythonrules@`, `pkgpythoncxx*` |
| `SWIG_MAPPER_NAMESPACE`, `SWIG_HAVE_BOOST_SERIALIZATION`, other `SWIG_RULES_*` macros | `CXX_PYTHON_RULES_NAMESPACE`, `CXX_PYTHON_RULES_HAVE_BOOST_SERIALIZATION`, `CXX_PYTHON_RULES_*` |
| sample `swig_rules_sample.h`, class `SwigRulesSample` | `cxx_python_rules_sample.h`, `CxxPythonRulesSample` |

Not renamed: `swig_shelve` (`python/skeleton/swig_shelve.py` and its sample; a public module name in geocal) and
`iostream_swig` (C++ classes); historical references to removed SWIG files (`swig_rules/include/...`); the
`SwigDirector_` class names inside the golden shelves written by SWIG (data). The GitHub repositories were renamed
on 2026-11-06 (`Cartography-jpl/swig-rules-skeleton` → `cxx-python-skeleton`, `Cartography-jpl/swig-rules` →
`cxx-python-rules`; GitHub redirects the old URLs), and `origin` here points at the new URL. Other clones (rifle,
geocal's `swig-rules-repo` remote if it has one) can be updated with `git remote set-url`. The local checkouts and scratch
builds were renamed to match (Locations table). The python package is `skeleton_cxx` rather than the
`skeleton._skeleton` suggested in chat: with `--enable-python-editable-mode`, `skeleton` is imported from the source
tree, so an extension installed into `site-packages/skeleton/` wouldn't be found.

*Earlier (2026-11-06):* **Phases S5, S4 and S6 are done** (in that order, progress log), committed on `modernize` at
the maintainer's request (the earlier phases were staged for the maintainer to commit). The python bindings are
nanobind; SWIG is gone. Suggested tags: `phase5-samples` and `swig-final` at the S5 commit, `phase4-std-shared-ptr`,
`nanobind-1` at the S6.6 commit. Next is **Phase S7** (§11), or geocal's own phases using the "Notes for geocal" table.

*Earlier (2026-11-05, later):* Phases S0, ST and S1–S3 were done (suggested tags `phase1-cxx20`, `phase2-boost-std`,
`phase3-std-types`).

*Earlier (2026-11-05):* Phase S0 is done, and ST steps 1–11 are done (progress log). What is left of ST is its
exit checklist: all four builds were run on 2026-11-05. gcc 15, gcc 14 and `--without-boost-serialization` are green;
ASan finds no memory errors but one leak in an error path. Both findings (A, B in the progress log) are decided
(§13 decisions 19, 20), so **ST is done**; the maintainer may tag it (`phaseT-tests`). Next is **Phase S1**.

*Earlier status (2026-10-30):* Phase S0 is done (all 8 steps; see the progress log). A review on 2026-10-30 measured the
test coverage (§4a) and found that the tests don't cover enough to catch regressions in S1–S6, plus a few real bugs. So
the next step is the new **Phase ST** (test hardening, §4a), done **before** S1. It also brings in the `swig_rules`
changes made in geocal since 2026-10-23 (§2.5). After ST comes **Phase S1** (C++20, tooling, hygiene), including the
clang-format trial (S1.6, using `claude_notes/clang_format_trial.clang-format`). The maintainer has to say "go" before
ST starts (approved 2026-10-30, §13 item 11; code work still waits for "go"); if unsure, ask.

**Review 2026-11-05** (§2.6): geocal found that `make check` can test the *installed* library instead of the one just
built (conda RPATH). geocal wants the fix tried here first, so it is now ST step 11. The threading work in geocal adds
GIL, worker-thread and overload-dispatch tests to S6.1 (items 7, 11). framework has not changed. ST.11 was done the
same day: a per-target rpath in `lib/lib.am`, accepted by the maintainer (§13 decision 18); see the progress log.

**Rules for working in this repository** (maintainer decisions):
* Work on the **`modernize`** branch (already checked out). Don't touch `master` (it keeps the set-aside pickling work).
* **Stage changes with `git add`, never commit.** The maintainer reviews and commits. Phase "tags" in this plan are
  suggestions for the maintainer to create after committing.
* Don't push, and don't `git subtree push` to `cxx-python-rules-repo` (it's left alone and allowed to drift).
* Put generated documentation in `claude_notes/`.
* **The Python bindings may be broken from S4 until S6** (decided): S4 converts C++ and serialization to
  `std::shared_ptr` and is verified by the C++ tests and golden archives only; the SWIG side is not ported. See §8.

**Locations:** work happens on two machines with different paths. Tell them apart by `hostname` (`rifle` or
`frenchpeas`), or just check which path exists, since only one is valid on each machine. Paths elsewhere in these
notes were mostly written on rifle; on frenchpeas, use the matching frenchpeas path from this table.

| What | rifle (work) | frenchpeas (home) |
|---|---|---|
| This repo | `/home/smyth/Local/cxx-python-skeleton` (same as `/ldata/smyth/cxx-python-skeleton`) | `/data2/smythdata/cxx-python-skeleton` |
| Scratch build + pixi env (disposable; fine to modify its `pixi.toml`) | `/home/smyth/Local/cxx-python-skeleton-build/build-pixi`; also `build-pixi-gcc14` (its own env: `build-pixi`'s `pixi.toml` plus `gcc`/`gxx` `14.*` and `python = "3.13.*"`, since a fresh solve gives Python 3.15, which configure can't link against), and `build-coverage`, `build-noser`, `build-asan` (all use `build-pixi`'s env; recreated 2026-11-06 after the rename) | `/data2/smythdata/cxx-python-skeleton-build/build`; also `build-coverage` (uses `build`'s env) `build-rpath` (its own env: `build`'s lock plus `gxx_linux-64`/`gcc_linux-64`, i.e. conda's compiler activation, for ST.11), and `build-gcc14` (min-compiler env, gcc/g++ 14.4.0; `compilers` 1.11 there, so it has conda's compiler activation too), `build-noserial` and `build-asan` (both use other dirs' envs: `build` and `build-rpath`), `build-cmake` (CMake reference build, uses `build`'s env); all recreated 2026-11-06 after the rename |
| geocal repo | `/ssdata/smyth/geocal-repo` | `/data2/smythdata/geocal-repo` |
| geocal build | `/ldata/smyth/geocal-build/build-pixi` | `/data2/smythdata/geocal-build/build-pixi` |
| framework (follows later; read-only survey) | `/home/smyth/Local/framework` | `/data2/smythdata/framework` |
| framework build | `/home/smyth/Local/framework-build` | `/data2/smythdata/framework-build/build_conda`, `build_conda_debug` |

Configure the skeleton scratch build from inside its build directory with
`../../cxx-python-skeleton/configure --prefix=$CONDA_PREFIX --enable-maintainer-mode --enable-debug`. The coverage
build recipe is in §4a "Coverage builds".

**After moving or renaming a checkout or build directory** (done 2026-11-06 for the rename): configured build
directories and pixi envs hold absolute paths, so recreate them rather than move them. For a pixi env,
`rm -rf .pixi && pixi install --frozen` (same packages, from `pixi.lock`). For a build directory, delete its contents
and run `configure` (or `cmake`) again with the arguments at the top of the old `config.log` (`CMakeCache.txt`),
paths changed. Run from a fresh shell: a `pixi shell` started before the move keeps the old paths in its
environment (`PATH`, `CONDA_PREFIX`).

| What | Where |
|---|---|
| nanobind probe sources (reference for S6.1) | `claude_notes/nanobind_probes/` (see its README) |
| geocal plan (the large repo this rehearses) | `<geocal repo>/claude_documents/cpp_modernization_and_library_plan.md` |
| Earlier discussion (superseded where it disagrees) | `claude_notes/modernization_ideas.md` (§11), `serialization_options.md` (§9), `build_layout_review.md`, `swig_rules_review.md` |

**Notes for geocal (and framework)** — things found here that geocal should also change. A Claude session working
on geocal and pointed at this repository should read this list, check each item against geocal's current code, and
apply it there (or record why not) in geocal's plan:
| Item | Found | geocal/framework action |
|---|---|---|
| `python-install`: uninstall from the prefix first, and `--ignore-installed` instead of `--force-reinstall` | 2026-10-30, ST.7 (commit `5fc4847`), decision 17 | geocal's `python/python.am` has the same rule. Two problems with `--force-reinstall`: (1) switching from a normal to an editable install leaves the old copied package in `site-packages`, which shadows the editable one; (2) it uninstalls the package from *whatever environment pip sees*, not only the prefix, so a `--prefix=/tmp/x` build removes the copy in the conda env. Copy the skeleton's rule (in `python/python.am`): `if ls -d @prefix@/@pythondir@/<pkg>-*.dist-info; then PYTHONPATH=@prefix@/@pythondir@ $(PYTHON) -m pip uninstall -y -q <pkg>; fi`, then `pip install --prefix=@prefix@ --ignore-installed --no-deps [-e] ...`. The guard matters: without it, pip uninstall goes on to remove the package from the env. Verify both switching directions as in geocal's `python_editable_install_plan.md`, and that the env's own install survives. |
| `"blah"` printing bug, and `Exception` copy constructor | 2026-10-30, §4a bug 3, decision 12, ST.7 (commit `76cdf37`) | `print_to_string` / `Exception` seed an `ostringstream` with `"blah"`, so outputs shorter than 4 characters come out wrong (`'Alah'`). Same code in geocal `lib/GeoCalCore/Support/printable.h`, `geocal_exception.h`, `lib/Msp/msp.h`, `lib/swig_quaternion.i`; framework `lib/Support/printable.h`, `fp_exception.h`. Use a plain `std::ostringstream`; add a test with a 1-character `print`. **Also** geocal's `Exception(const Exception&)` sets the copy with `s_.str(cp)`, which leaves the stream at position 0, so a later `<<` overwrites the copied message (framework already uses `s_ << E.s_.str()`, which is right). Tests: the skeleton's `lib/printable_test.cc` and `lib/skeleton_exception_test.cc`. |
| `.bin`/`.json` shelves | ST.5/6 (commit `f1d57f1`) | Skeleton-only bug (its `serialize_function.i` had drifted from the sample). Nothing to do in geocal, but geocal's `sqlite_shelf.py` gets the same jsonpickle `DeprecationWarning`; pass `keys=True` to `jsonpickle.encode`/`decode` as the skeleton now does. |
| **`swig_array.i` typemaps crash on bad input** | ST.3 (commit `d161c6b`) | Three typemaps set a python error and carried on: by-value `blitz::Array<T,D>` and `FORCE_COPY` (wrong rank aborts python with an uncaught C++ exception, a non-array segfaults), and `directorout` (a python override returning a non-array or wrong rank segfaults). geocal and framework have the same code. Comes with the next `swig_rules` copy (the file is otherwise identical to geocal's); regenerate the wrappers. geocal has `FORCE_COPY` in `auto_derivative.i` and 9 directors, several returning Blitz arrays. Test: the skeleton's `python/tests/unit/array_typemap_sample_test.py`. |
| `skip_weak_ptr_handling(true)` used stale marks | ST.5 (commit `d264924`) | Fixed in `swig_rules/lib/serialize_function.cc` (`mark_pointer` clears before returning). Nobody calls it in geocal/framework; comes along with the next `swig_rules` copy. |
| `doxygen_single.cfg` `DOT_MULTI_TARGETS` | ST.1 | Obsolete in doxygen 1.18 (warning on every module). Removed here; geocal's copy (doxygen 1.13) will need the same when its doxygen is updated. |
| Test coverage measurement | 2026-10-30, §4a | Recipe in §4a "Coverage builds" (`gcovr`; the system `lcov` 1.14 can't read gcc 15 data). geocal should measure its own coverage before its Phase 5/6, especially for the `swig_rules` typemaps (table in §4a). |
| `swig_rules` changes flow both ways for now | decision 15 | Before copying `swig_rules/` or `config/m4/` in either direction, diff the trees and merge file by file. geocal-only settings (e.g. `swig_rules/__init__.py` `if True:`) must not be overwritten. |
| **`make check` tests the installed library** (geocal plan §6.1, open item 5) | 2026-11-05, ST.11 | Reproduced here and fixed with a per-target rpath (option 5 below), not one of geocal's four options. In `lib/lib.am`: `UNINSTALLED_PROGRAM_LINK_FLAGS = -no-install -Wl,-rpath,$(abs_top_builddir)/.libs`, used as `*_LDFLAGS` of every never-installed program linked to the library (`skeleton_test_all`, `golden_archive_generator`). Automake puts per-target LDFLAGS before `$(LDFLAGS)`, so `.libs` comes before conda's env rpath. geocal: do the same for `geocal_test_all` and any other `check_PROGRAMS`/`noinst_PROGRAMS` (keep `-no-install` if it is there; replacing it brings back a libtool wrapper). Not for installed programs (the build dir would end up in their rpath); geocal's build-dir libtool wrappers such as `picmtch5` are a separate case. Then drop the `LD_PRELOAD` workaround from `resampler_threading_plan.md`. |
| `--enable-debug` with no flags; Blitz flags leak `-DNDEBUG -O2` | 2026-11-05, S1.2 | Both in shared `config/m4/` files, so copy them. **`ac_blitz.m4`:** geocal has the same leak (`BLITZ_CFLAGS = -DNDEBUG -D_FORTIFY_SOURCE=2 -O2 -pthread -I... -isystem ...` in `geocal-build/build-pixi`, from the afids-channel `blitz.pc`), so geocal debug builds have `NDEBUG` too; a geocal release build won't change. **`ac_enable_debug.m4`:** only matters in an env without conda's compiler activation (geocal's env has it). Re-run `./bootstrap` after copying. The warning flags (`skeleton_common.m4`) and the `-Wno-cast-function-type` in `swig_rules.am` come with the `swig_rules/` copy / need the same line in geocal's local macro. |
| `Exception` (S1.5) | 2026-11-05 | Copy `lib/skeleton_exception.{h,cc}` patterns: the format constructor (`ExceptionFormat`), `source_location`, stack trace as stored addresses with the Boost.Stacktrace code in a `.cc` (+ `-ldl`), `capture_stacktrace(bool)`, and `operator<<` keeping stream state (framework needs it). geocal's `backtrace.hpp` base goes. Keep the doxygen `\cond` on the template constructor, or doxy2swig breaks the Python constructor docstring. `binutils` in the env and the configure check give readable traces. |
| **Formatting: clang-format, adopted for skeleton and geocal** | 2026-11-05, S1.6 revisit, §5b, decision 21 | Done in geocal (staged, uncommitted): the skeleton's `.clang-format`, `.clang-format-ignore` (`bindings/**`, `thirdparty/**`, `boost_fix/**`, `lib/GeoCalCore/Support/boost/**`), `.clang-tidy`, `script/make_compile_commands.py` and `run_clang_tidy.sh` (copied, identical), `make format`/`format-check`/`lint` in `geocal.am`, `clang-tools = "22.*"` in `env/pixi.toml` and `env/linux/pixi.toml` (lock refreshed through a scratch copy, not `make update-lock`; frenchpeas's build env is older than the committed lock), `lib/` reformatted, one wrapper docstring regenerated. No `.git-blame-ignore-revs` (maintainer, 2026-11-05: this branch changes a lot anyway). `make lint` reports the S1.3 checks geocal hasn't had yet (its §8 cleanup). framework follows the same way when modernized. |
| Boost → std in `swig_rules` (S2) | 2026-11-05 | Comes with the next `swig_rules/` copy: `serialize_function.cc` (no `boost::regex`/`boost::filesystem`), `swig_type_mapper_base.h` (`type_index` without `boost::operators`, still name-based on purpose), `swig_boost_array.i` (no `BOOST_FOREACH`), `generic_object_map.h.sample`. geocal still needs `boost_regex`/`boost_filesystem`/`boost_process` itself, so only drop them from its link line once its own uses are gone. |
| `std::optional` / `std::array` (S3) | 2026-11-05 | Comes with the next `swig_rules/` copy: the optional typemap macro (both types), the `std::array` typemaps and `%python_attribute_std_array`, the `print_to_string` fix in `swig_boost_array.i`, and the **Boost #319 workaround** in `serialize_shared_ptr.h` (needed as soon as geocal serializes a `std::optional`). geocal's own `geocal_serialize_support.h` needs the same change as `skeleton_serialize_support.h`: always include `boost/serialization/array.hpp` (the `std::array` serializer). A returned `std::array` is a numpy array in python, not an `Array_double_N` object: record in `RELEASE_NOTES_2.0.md` when geocal converts. |
| **Observer registered in `load()` added twice** | 2026-11-06, S5 | framework (and geocal, when it serializes observers): `FP_OBSERVER_SERIALIZE` loads `olist` directly. An observer that calls `add_observer(*this)` in its `load()` (5 such in framework) and is first reached through the Observable's `olist` registers before its own entry is read, so it is in `olist` twice and notified twice. The skeleton's `observer_serialize_support.h` splits `Observable::serialize` and merges (`merge_loaded`, archive order kept, same archive format). Copy it; test `observer/serialization` in `lib/observer_test.cc`. |
| SWIG pin | decision 14 | geocal already pins `4.4.*`; the skeleton pins `4.5.*`. Keep each pinned to its committed wrappers. |
| `std::shared_ptr` migration (S4) | 2026-11-06 | Run `script/boost_to_std_ptr.py` (`--dry-run` first; `--swig` would also do `.i` files, not needed if the bindings break until nanobind as here). Then fix what it reports (`BOOST_SP_*`, `scoped_ptr`, ...) and missing standard includes that Boost headers had pulled in (here one `<vector>`). Copy `serialize_shared_ptr.h` (same archive format) and `null_deleter.h`'s aliasing `to_ptr`/`const_pointer`. Check with the golden archives *and* a byte-identical rewrite of each (`golden_archive/rewrite_identical` in `lib/golden_archive_test.cc`). |
| **nanobind bindings (S6)** | 2026-11-06 | Copy `swig_rules/nanobind/` and `swig_rules/lib/director_hook.h`, `config/m4/ac_nanobind.m4`, the `swig_rules/*.py` package files; drop `swig_rules/include` and the SWIG helpers (list in the S6.6 progress row). Add `std::enable_shared_from_this` to `GenericObject`. Generate the bindings with `i_to_nanobind.py --namespace GeoCal --header-dir lib/... --prefix geocal_nb --out-dir ... <all .i>`, keep the pristine output, then hand-fix the `HAND CONVERT` items (`%extend`, `%pythoncode`, `%typemap`). Things found here that apply there: (1) compile the binding files **without `-fvisibility=hidden`** (else director serialization breaks reading archives of the base class); (2) `nb::arg().none()` on `shared_ptr` arguments; (3) don't use `nb::new_` for director classes; (4) a director class's binding must call the C++ implementation non-virtually for python subclasses (`super()` ran the override twice); (5) a function that runs python callbacks on C++ worker threads (geocal's `ThreadPool` users) needs `nb::call_guard<nb::gil_scoped_release>()`, else it deadlocks; (6) nanobind crashes on a null docstring; (7) `make check` against the build tree needs `LD_PRELOAD` of the built library (RPATH, ST.11); (8) remove an old `<pkg>_swig/` install directory before installing (stale `__init__.pyi`). geocal's `RELEASE_NOTES_2.0.md` can take the "Python API changes" list in §10 (S6.6). |
| `Exception::stacktrace()` with addr2line | 2026-11-06, S6 | Boost.Stacktrace's addr2line backend gives wrong file:line for code in shared libraries (it looks every frame up in the main executable) and runs addr2line per frame (11 s per trace from python). `lib/skeleton_exception.cc` now looks up the module of each frame with `dladdr` and runs addr2line once per module; copy it with the S1.5 `Exception` pattern. Also: from python, a library exception carries `cpp_exception` (a copy, so the trace is resolved only if asked for) and `cpp_source`. |
| Golden shelves with python subclasses | 2026-11-06, S5/S6 | Before geocal's SWIG bindings go, write a few shelves with python subclass objects (as `unit_test_data/golden/director_python*.xml` here) and check the nanobind build reads them. |
| **C++23 and `mdspan` for `read_ptr` (S7)** | 2026-11-06 | `AX_CXX_COMPILE_STDCXX([23], [noext], [mandatory])`; gcc 14 is fine (only gcc 14 library features: no range formatting, no `std::mdspan` before gcc 16). Kokkos mdspan in C++23 mode indexes with `m[i, j]` only (`m(i, j)` is off; drop any `MDSPAN_USE_PAREN_OPERATOR` define), and `m[i, j]` inside a macro (`BOOST_CHECK_EQUAL`) needs extra parentheses. Don't name a member `range_check`: `geocal_exception.h` has a `range_check` macro. For `RasterImage::read_ptr` and friends, `lib/raster_sample.h` is the worked pattern: a `layout_stride` mdspan virtual (`read_into`), `read_ptr`/`write_ptr` kept as non-virtual wrappers so callers move over gradually, a contiguity check plus temporary for C-library backends (VICAR, GDAL), `read_with_pad` through `submdspan`, `CalcRaster`'s writable `self.data` for python subclasses (a "publicist" to bind the protected `data_ptr`), and python `read_into` of a writable numpy array (`noconvert`). Not wrapped by geocal's SWIG, so it is a C++-only change there. Bootstrap: `AC_REQUIRE([AC_PROG_AWK])` at the top of the common macro silences `AC_PROG_AWK was expanded before it was required` (check whether geocal's `afids_common.m4` has the same warning). |
| **Rename `swig_rules` → `cxx_python_rules` (2026-11-06)** | §0 "Naming" | When geocal next copies `cxx_python_rules/` (replacing its `swig_rules/`), rename the same names in geocal's own files: `-DSWIG_MAPPER_NAMESPACE=GeoCal` → `-DCXX_PYTHON_RULES_NAMESPACE=GeoCal`, `SWIG_HAVE_BOOST_SERIALIZATION` → `CXX_PYTHON_RULES_HAVE_BOOST_SERIALIZATION` (both in many geocal headers), `@swigrules@` → `@cxxpythonrules@`, `pkgpythonswig*` → `pkgpythoncxx*`; the python package `geocal_swig` → `geocal_cxx`, module `geocal_cxx._bindings` (`geocal/__init__.py`, mypy overrides, any `geocal_swig` imports in scripts). The perl substitutions used here are in the rename progress row (order matters: longest names first). Before installing, remove the old `geocal_swig/` directory from `site-packages`. Pickles written before break (they name the old module); shelves and boost archives don't. |
| **`write_shelve` with a `pathlib.Path`** | 2026-11-06, rename | `swig_shelve.py`'s `write_shelve` passes `f` unconverted to `serialize_write`/`serialize_write_binary` for `.xml`/`.bin`; the C++ side takes a `std::string`, so a `Path` gives `TypeError`. Use `str(f)` (as the skeleton does now) and add a test with a `Path`. |

**Progress log** (update as phases complete; one line per step):
| Phase / step | State | Notes |
|---|---|---|
| Planning | done | 2026-10-23 |
| S0.1 baseline | done | 2026-10-23. C++: 1 test program (`skeleton_test_all`) passes. Python: 16 passed, 1 skipped (`test_director_example_weak_ptr`). `test_director_example_object_tracking` passed (no `NameError`; its identity asserts were commented out). Env: gcc 15.3, Boost 1.90, SWIG 4.5.0. |
| S0.2 set aside pickling | done | Asserts uncommented; `object_tracking` is `xfail(strict=True)` (the `e1_nest_p` identity assert fails). `test_director_example_weak_ptr` **passes when unskipped** (5 of 5 runs), so it is now a normal test, not an `xfail`. |
| S0.3 golden archives | done | `unit_test_data/golden/` (52 files), `make golden-archives`, `lib/golden_archive_test.cc`, `python/tests/unit/golden_archive_test.py`. Findings: (a) `serialize_function.i` lacks `serialize_read_binary_generic` (the sample has it), so `.bin` through `read_shelve` fails: 5 strict `xfail`s, fix when convenient. (b) `blitz::Array<float/short/unsigned/ushort>` are instantiated in the `.cc` but lack `BOOST_SERIALIZATION_SPLIT_FREE` in the header, so can't be serialized; left alone. |
| S0.4 tooling in pixi | done | `clang-tools` and `nanobind` in `env/linux/pixi.toml` (+ lock) and in both scratch envs. `clang-format`/`clang-tidy` 22.1.8 and `nanobind` 3.1.0 work. **`bear` and `intercept-build` do not run on this host, so `bear` was removed from all three pixi setups** (glibc 2.28; the conda `bear-driver` needs 2.39 and the `intercept-build` preload crashes). This is a host problem, not a gcc version one, so a gcc 16 env would not help and the gcc 14 env is no different. Workaround, tested: `script/make_compile_commands.py` (builds `compile_commands.json` from `make -n`, run in the build dir) and `script/run_clang_tidy.sh` (adds the conda sysroot and gcc builtin headers, and a shim `blitz/llvm/bzconfig.h` -> `gnu`, because the conda blitz has no llvm config and clang can't parse blitz otherwise). Use these for S1.3 instead of `bear -- make`. First look: `modernize-use-nullptr` finds 3 hits in `swig_rules/lib` headers, `modernize-use-override` finds the `virtual` functions in `director_example.h`. |
| S0.5 min-compiler env | done | `/home/smyth/Local/cxx-python-skeleton-build/build-pixi-gcc14` (gcc/g++ 14.4.0, SWIG 4.5.0). Configure with `--prefix=$CONDA_PREFIX --enable-maintainer-mode --enable-debug`, `make`, `make check` (1/1), `make install`, `make installcheck` (21 passed, 6 xfailed): same as the gcc 15 results. Run `pixi run` from inside that directory (it warns about `PIXI_PROJECT_MANIFEST` pointing at `build-pixi`; the local manifest is the one used). |
| S0.6 `--without-boost-serialization` | done | Builds and C++ tests pass (`--without-swig --without-documentation`). Fixed: 3 `.cc` files guarded, `swig_rules/lib/serialize_shared_ptr.h` body guarded, `-DSWIG_HAVE_BOOST_SERIALIZATION` only when serialization is on (new `HAVE_BOOST_SERIALIZATION` conditional), map tests skip, a stale `GEOCAL_` macro in the blitz test, `ac_boost.m4` no longer requires `libboost_serialization` in that case. Geocal reference: the same pattern. |
| S0.7 housekeeping | done | No `*~` files were tracked. `.gitignore` extended (egg-info, pytest_tmp, test xml). |
| S0.8 sanitizer | done | See "Sanitizer builds" below. |
| Review 2026-10-30 | done | geocal changes since 2026-10-23 reviewed (§2.5). Coverage measured (§4a): C++ `lib/` + `swig_rules/lib/` 52% of lines, Python `skeleton` package 42% (`swig_shelve.py` 38%); most `swig_rules` typemaps are never exercised. Bugs found that no test catches: §4a "Bugs found". `gcovr` and `pytest-cov` added to the scratch `build-pixi` env (not yet to `env/linux/pixi.toml`). Phase ST added before S1. |
| Review 2026-11-05 | done | geocal `2193f1953`..`f3d94e93` and framework (no change since 2026-10-22) reviewed (§2.6). New: ST step 11 (`make check` must test the just-built library), S6.1 items 7 and 11, S6.3 notes, §13 open items. |
| ST.1–ST.8 | done | 2026-10-30, commits `5bfa4c4`..`8c2f015` (see `git log`; ST.3 also added the 4 missing Blitz element types to serialization and golden archives, bug 5). |
| ST.9 coverage | done | 2026-11-05, `build-coverage` on frenchpeas, `make coverage`. **C++ `lib/` + `swig_rules/lib/`: 91.7% of lines (905 of 987) with `--merge-lines`, 82% (2123 of 2582) without** (the 2026-10-30 figure of 52% was without). Functions 90.7%, branches 51.2%. Python `skeleton` package **94%** (`swig_shelve.py` 94%). Target (≥ 80%, no untested `swig_rules` feature) met: every row of the §4a feature table now has a test. Lowest: `swig_rules/lib/python_lib_init.h` 61% (module-init error paths), `swig_type_mapper_base.h` 75%, `swig_to_python.h` 86%; `lib/generic_object.cc` 0% (2 lines, the empty `GenericObject` destructor and registration). `gcovr` and `pytest-cov` had to be added to the frenchpeas scratch env (`build/pixi.toml`, with `clang-tools` and `nanobind`, matching `env/linux/pixi.toml`). |
| ST.10 SWIG reference | done | 2026-11-05, `build` (gcc 15.3, SWIG 4.5.0, Boost 1.90, Python 3.13). **C++: 1/1 test program, 25 of 25 test cases, 2025 assertions. Python (`make installcheck`): 89 passed, 1 xfailed, 0 skipped.** The one strict `xfail` is `director_example_test.py::test_director_example_object_tracking` ("nested C++/Python identity: redesigned in S6"). The 5 golden `.bin` `xfail`s from S0.3 are gone (fixed in ST.5). This is what S6 has to reproduce (plus the S5 samples). |
| ST.11 `make check` RPATH | done | 2026-11-05; accepted (§13 decision 18). **Reproduced** in `build-rpath` (conda compiler activation, `--prefix=$CONDA_PREFIX`): after `make install`, `skeleton_test_all`'s RPATH was `<env>/lib:.libs`, `ldd` showed the installed `libskeleton`, and with `GenericObjectA::serialize` deliberately broken `make check` **passed** (`LD_PRELOAD` of `.libs` made the same binary fail). The build-tree `_swig_wrap.so` already had `.libs` first; only programs are affected. Option 4 (`-Wl,--enable-new-dtags`) alone **doesn't fix it**: RUNPATH keeps the same order, and nothing sets `LD_LIBRARY_PATH` because `-no-install` programs have no libtool wrapper. **Fix:** `UNINSTALLED_PROGRAM_LINK_FLAGS` in `lib/lib.am` (see "Notes for geocal"), used by `skeleton_test_all` and `golden_archive_generator` (which had the same problem, so `make golden-archives` would have written the corpus with the installed library). Checked: the broken library now makes `make check` fail; clean source passes `make check` before and after `make install` and `make installcheck` (89 passed, 1 xfailed) in both `build-rpath` and `build`; the installed `libskeleton.so.0` has no build directory in its RPATH. |
| ST exit checklist | done, with 2 findings | 2026-11-05, frenchpeas. **gcc 15** (`build`) and **gcc 14** (`build-gcc14`: g++ 14.4.0, SWIG 4.5.0, Boost 1.90, Python 3.13; `--prefix=$CONDA_PREFIX --enable-maintainer-mode --enable-debug`) both green: `make check` 1/1 (25 test cases) before and after `make install`, `make installcheck` 89 passed, 1 xfailed. The gcc 14 env has conda's compiler activation, so it also confirms the ST.11 fix. Warnings identical to gcc 15 (125: 119 `-Wunused-parameter`, 5 `-Wcast-function-type`, 1 `-Wsign-compare`; for S1.2). **`--without-boost-serialization`** (`build-noserial`, `--without-swig --without-documentation`): `make skeleton-check` passes, 15 of 15 test cases (the serialization ones are compiled out). Finding A below. **ASan** (`build-asan`): C++ tests all pass with no memory errors; Python tests (`LD_PRELOAD` libasan, `detect_leaks=0`) 89 passed, 1 xfailed, no ASan errors. Finding B below. |
| ST finding A: Python extension needs serialization | decided | `--without-boost-serialization` builds only the C++ library and its tests: `--without-swig` still compiles the committed wrappers, and `%skeleton_director_serialization` (`swig_rules/include/director_serialization.i`, since 2023) puts unguarded Boost.Serialization code in `director_example_wrap.cc`, so plain `make` fails. **Maintainer, 2026-11-05: that is the intended scope** (§13 decision 19). Documented in `CLAUDE.md`; the phase-end check stays `make skeleton-check`. |
| ST finding B: leak on a truncated archive | accepted | LeakSanitizer fails `make skeleton-check` in the ASan build with 176 bytes in 4 indirect allocations, all from the truncated-archive read in `serialize_function/directory_restored` (added in ST.5, after the clean S0.8 run). Every normal read is clean. **Cause (Boost, not our code):** in Boost 1.90 `iserializer.hpp`, `heap_allocation()` allocates the object and releases ownership at once, and `pointer_iserializer::load_object_ptr` only guards `load_construct_data`; the data load after it has no try/catch. So if the archive ends partway through an object loaded through a pointer (here the top-level `GenericObjectMap`), nothing owns that object, and what it already holds (a map node, a `shared_ptr` to `GenericObjectA`) goes with it. **Tried:** `basic_iarchive::delete_created_pointers()` in a catch in `serialize_read_generic` (through the `polymorphic_iarchive` base; it is private on the concrete archive). No double free, but the leak is unchanged: it only deletes objects whose load finished, not the one in progress. Freeing the object ourselves isn't safe in general: we can't tell whether it was constructed (a director's `load_construct_data` can throw too), and the pointer isn't yet adjusted to the base type. **Decision (maintainer, 2026-11-05): accept and note** (§13 decision 20). A load-only error path; small, one-off. For the ASan C++ check, either set `ASAN_OPTIONS=detect_leaks=0` or add a narrow LSan suppression (`leak:directory_restored`, needs `fast_unwind_on_malloc=0` for full stacks). Look again if it turns up outside a deliberately broken archive, or after S4 (`std::shared_ptr`). |
| ASan on frenchpeas | note | The S0.8 recipe fails at configure in the `build` env: with `-fsanitize=address` the non-activated conda gcc links the host's `/lib64/librt.so.1` (Gentoo glibc, `undefined reference to __libc_unwind_link_get@GLIBC_PRIVATE`). Use an env with conda's compiler activation (`build-rpath`) and append to its `LDFLAGS`: `LDFLAGS="$LDFLAGS -fsanitize=address"`, and preload `$($CXX -print-file-name=libasan.so)`. |
| S1.1 C++20 | done | 2026-11-05. `AX_CXX_COMPILE_STDCXX([20], [noext], [mandatory])` in `config/m4_local/skeleton_common.m4` (was `[17], [ext]`), `./bootstrap` (autoconf 2.72, automake 1.17, libtool 2.5.4; only `configure`, `Makefile.in` and `lib/skeleton_config.h.in` change, `HAVE_CXX17` → `HAVE_CXX20`, unused in the code). Compiles as `-std=c++20`; nothing needed GNU extensions and no code changes were needed. `config/m4/afids_common.m4` (shared) still says `[17], [ext]`; the skeleton doesn't use that macro, left alone. Results: gcc 15 (`build`, and `build-rpath` with `-Wall -Wextra`) and gcc 14 (`build-gcc14`) all `make check` 25/25, `make installcheck` 89 passed, 1 xfailed; `--without-boost-serialization` `make skeleton-check` 15/15. Committed SWIG wrappers unchanged. **Correction (found in S1.2):** these S1.1 builds were incremental, so objects whose dependencies didn't change (e.g. `swig_std_wrap.cc`) weren't recompiled and were still C++17. The warning count (125 → 120) was an artifact of that, not of C++20: the 5 `-Wcast-function-type` warnings are there in C++20 too. The first fully C++20 build and test run is the clean S1.2 one (all green). After changing compiler flags, `make clean` first. |
| S1.2 warnings | done | 2026-11-05. **Build flags:** (a) `config/m4_local/skeleton_common.m4`: debug builds (`--enable-debug`, gcc) add `-Wall -Wextra -Wdeprecated`. (b) `config/m4/ac_enable_debug.m4`: in a conda env without compiler activation, `DEBUG_CXXFLAGS` is empty and `--enable-debug` gave **no flags at all** (no `-g`); it now falls back to the gcc branch (`-ggdb -DBZ_DEBUG -fbounds-check`), so the `build` env now also has Blitz bounds checking. (c) `config/m4/ac_blitz.m4`: `BLITZ_CFLAGS` keeps only the include directories, as `-isystem`. The conda-forge `blitz.pc` `Cflags` carry Blitz's own build flags (`-DNDEBUG -D_FORTIFY_SOURCE=2 -O2 -pthread`), which were in every build, so even debug builds had `NDEBUG`. (d) `swig_rules/swig_rules.am`: `-Wno-cast-function-type` for the wrapper library only (5 warnings from SWIG's own generated `std::ios` manipulator casts in `swig_std_wrap.cc`). **Code:** commented-out names for unused parameters (`lib/blitz_array_serialize_support.cc` 84, `lib/generic_object.cc`, `swig_rules/lib/python_ref_ptr_cleanup.h`, `python_lib_init.h`, and in `.i` code copied into the wrappers: `swig_iostream_inc.i` `seek`, `director_serialization.i` `version`); a signed/unsigned loop compare in `swig_to_python.h`; `[[maybe_unused]]` on the parameters of the no-serialization stubs in `swig_rules/lib/serialize_function.cc`; `lib/serialize_function_test.cc` helpers inside the serialization `#ifdef`, and `is_gz_name` uses `ends_with` (C++20). The 12 regenerated wrappers differ only in those parameter names. **Result: 0 warnings** (was 125; *correction found in S1.3: this count was from `make`, which doesn't build the test programs; they had 2 more `-Wsign-compare`, in `golden_archive_test.cc`/`golden_archive_generator.cc`, fixed in S1.3. Count warnings from `make` plus `make skeleton_test_all golden_archive_generator`*) from clean builds in `build` (gcc 15, no activation), `build-rpath` (gcc 15, activation) and `build-gcc14`, and from `--without-boost-serialization` (was 16 there). All green: `make check` 25/25, `make installcheck` 89 passed, 1 xfailed, no pytest warnings; no-serialization 15/15. The pytest half of S1.2 (`--strict-markers`, a specific `filterwarnings` list) was already done in ST.7. No Blitz `-Wdeprecated-copy`/`-Wvolatile` noise (geocal saw ~13k): the Blitz headers are system headers here. |
| S1.3 clang-tidy | done | 2026-11-05, clang-tidy 22.1.8. **How:** `compile_commands.json` from `script/make_compile_commands.py libskeleton.la skeleton_test_all golden_archive_generator _swig_wrap.la` (36 files; the wrappers are included so headers only they reach, e.g. `array_typemap_sample.h`, `swig_to_python.h`, are checked), `--header-filter` on `lib/` and `swig_rules/lib/`, fixes exported per file, **diagnostics in the generated wrappers dropped**, then `clang-apply-replacements`. **Tool fix:** `script/run_clang_tidy.sh` now passes `--gcc-install-dir` (conda gcc); otherwise clang picked frenchpeas's host gcc 16 C++ headers, which need a newer glibc than the conda sysroot (2 of 36 files failed to parse). **Per check** (findings outside the wrappers → result): `use-override` 24 → all automatic; `use-nullptr` 33 → automatic (it rightly leaves the `NULL` varargs sentinel of `PyObject_CallFunctionObjArgs`); `use-noexcept` 2 → automatic in `skeleton_exception.h`, **plus 2 by hand** in `swig_rules/include/swig_array_inc.i` (`ArrayConversionException`, `throw()`/`throw ()`): code in `.i` files is only seen inside the wrappers, so clang-tidy can't fix it; `use-equals-default` 29 → automatic, **plus 1 by hand** (`GlobalFixture` destructor: clang-tidy gives no fix when the body has a comment); `make-shared` 0 (it only knows `std::shared_ptr`; recheck after S4); `loop-convert` 0; `deprecated-headers` 2 → automatic (`<errno.h>`, `<string.h>` in `serialize_function.cc`). Afterwards 0 findings outside the wrappers. **By hand, at the maintainer's request:** `for` loops that cast `size()` to `int` now use a `std::size_t` index instead (6: `blitz_array_serialize_support_test.cc`, `golden_archive_generator.cc`, `golden_archive_test.cc`, `swig_to_python.h` ×2, the `swig_array.i` vector typemap; none can be a range-based `for`, since the index is also a value or a `PyList_SetItem` position), and the 3 element-copy loops in `swig_boost_array.i`/`swig_boost_array_inc.i` use `std::copy` (Blitz iterators follow strides; new strided-view cases in `test_boost_array`). **SWIG:** forced regeneration of every wrapper (touch the `.i` files) after the header changes: no SWIG errors, and wrappers byte-identical, so SWIG 4.5 parses `override`, `= default`, `noexcept` and the Python interface is unchanged. Wrapper changes come only from `.i` edits (`noexcept`, the loops). **Results:** clean builds, test programs included: 0 warnings in `build`, `build-rpath`, `build-gcc14`, no-serialization; `make check` 25/25, `make installcheck` 89 passed, 1 xfailed; no-serialization 15/15. **Patches, one per check, in order** (apply on top of `8251150`, the S1.2 commit): `/data2/smythdata/cxx-python-skeleton-build/s13_patches/` `00` tool fix, `01` override, `02` nullptr, `03` noexcept, `03b` its wrappers, `04` equals-default, `07` deprecated-headers, `08` loops, `08b` their wrappers (checked: the series reproduces the staged tree exactly). For geocal: the per-check approach, the wrapper-diagnostic filter, and "check `.i` code by hand" carry over (§8 of the geocal plan). |
| S1.4 `Printable<T>` | done | 2026-11-05. `lib/printable.h` and `swig_rules/lib/printable.h.sample` (still identical apart from namespace and include guard): `static_cast<const T*>(this)` instead of the C-style casts (which also cast away `const`); `print_to_string` calls `print_wrapper` directly (a `Printable` member, no cast needed). The `"blah"`/`reserve(1000)` workaround was already removed in ST.7. **New:** concept `PrintableObject` (has `print_wrapper`: a `Printable<T>` class or one derived from it, the same set `<<` works for) and a `std::formatter<T, char>` for it that reuses the `std::string_view` formatter, so `std::format("{}", x)` equals `<<` and width/fill/alignment work (`"{:>20}"`). SWIG never parses `printable.h` (the `.i` files declare `print_to_string`), so no `#ifndef SWIG` is needed. New C++ test `printable/format`: format == `<<` for short, empty, `desc()`, derived and sample classes, specs, and `static_assert`s that `std::string`/`int` aren't matched. `desc()`, the virtual `GenericObject` base and the class name are unchanged. Results: 0 warnings; `make check` 26/26 test cases, `make installcheck` 89 passed, 1 xfailed in `build`, `build-rpath`, `build-gcc14`; no-serialization 16/16. **Later (C++23):** `{:?}` debug format comes for free from the `string_view` base. But C++23 range formatting calls `set_debug_format` on element formatters that have it, so a `std::vector` of Printable objects would print quoted (`["A", "B"]`); decide then whether to hide that. Recommended out of scope while on C++20 (raised by the maintainer 2026-11-05; confirm). |
| S1.5 `Exception` | done | 2026-11-05. `lib/skeleton_exception.h`, new `lib/skeleton_exception.cc`. **Message:** a `std::string` (no `ostringstream` member, no `mutable` scratch; `what()` is just `c_str()`; copy and assignment are `= default`, so a copy no longer silently drops the message on `bad_alloc`). `operator<<` formats each value in a fresh stream but **carries the stream state over** (flags, precision, width, fill), because framework relies on it: `e << std::setprecision(8) << ...` in `absco_aer.cc`/`absco_hdf.cc`. **Format constructor:** `Exception("x = {}", x)`, through `ExceptionFormat<Args...>` (a `consteval` helper holding `std::format_string<Args...>` plus a defaulted `std::source_location`: the usual trick, since a defaulted argument can't follow the variadic ones). Constrained to `sizeof...(Args) > 0`, so `Exception("50% {}")` with no arguments is still taken literally, as before. Format errors are compile errors. **`source_location()`** for both constructors (for derived classes: where their constructor called ours). **Stack trace:** Boost.Stacktrace as a member, not a base (geocal plan §3.2), but only the frame addresses are stored; capture and turning them into text are in the `.cc`, so the header doesn't include Boost.Stacktrace and only `libskeleton` needs `-ldl` (added in `skeleton.am`; `dladdr` is in libdl before glibc 2.34). `stacktrace()` returns one frame per line; `Exception::capture_stacktrace(bool)` turns capture off (on by default). Cost (gcc 15, `-O1`): throw+catch 1.7 µs without capture, 4–5 µs with; symbol lookup happens only when `stacktrace()` is called. **addr2line (maintainer request):** `binutils` added to `env/linux/pixi.toml` (+ lock; only binutils changed) and the `build` scratch env. `configure` (`skeleton_common.m4`) finds `addr2line` on `PATH` and defines `SKELETON_ADDR2LINE` (absolute path; only plain paths are accepted, since Boost turns the macro back into a string); `skeleton_exception.cc` then uses Boost's addr2line backend (`BOOST_STACKTRACE_ADDR2LINE_LOCATION`, Boost doesn't search `PATH`), giving function names and file:line; without it the default backend gives addresses and exported names only. **Unchanged on purpose:** `print()` (so Python `str()` is the S1.7 baseline; geocal's `print()` also shows the trace, the skeleton could add it later) and the `.i` (S5's `ExceptionSample` exposes the stack trace to Python). **Docstring:** the template constructor is in doxygen `\cond`/`\endcond`, because doxy2swig gives all constructor overloads one docstring and this C++-only one replaced the Python `__init__` text (with a garbled `Exception(sizeof...(Args) > 0)` signature). **Also:** renamed `UNINSTALLED_PROGRAM_LDFLAGS` → `UNINSTALLED_PROGRAM_LINK_FLAGS` (automake warned about a `*_LDFLAGS` with no program of that name; the warning had been in every bootstrap since ST.11). A second bootstrap warning, `configure.ac:11: AC_PROG_AWK was expanded before it was required`, is older; not looked at. New C++ tests: `format_constructor`, `manipulators`, `source_location`, `stacktrace`. Results: 0 warnings; `make check` 30/30 test cases, `make installcheck` 89 passed, 1 xfailed in `build`, `build-rpath`, `build-gcc14` (each found an `addr2line`); no-serialization 20/20. |
| clangd setup | done | 2026-11-05, for the Claude Code `clangd-lsp` plugin (installed by the maintainer) or any editor. `script/make_compile_commands.py --clangd` adds the flags the conda clang needs (same as `run_clang_tidy.sh`: `--gcc-install-dir`, `--sysroot`, gcc builtin headers, a Blitz shim in `<builddir>/clangd_shim/`). Run it in the build directory, then link the result into the source tree: `ln -sf $PWD/compile_commands.json <srcdir>/compile_commands.json` (in `.gitignore`; machine specific, so make it on each machine). Include `_swig_wrap.la` in the targets so headers only the wrappers use get flags too. `clangd --check=lib/skeleton_exception.cc`: 0 errors (without it: `skeleton_config.h` not found, no C++20). A running clangd only finds the database when it starts, so restart it after creating the link. `clangd` 22.1.8 comes with `clang-tools` in `env/linux/pixi.toml`. |
| S1.6 formatting | done | 2026-11-05. **clang-format trial, not adopted** (§13 decision 21). With `claude_notes/clang_format_trial.clang-format` on the 45 C++ files: 44 files change, ~11% of lines ignoring whitespace (geocal: 14%). Findings: (1) its `Standard: c++03` misparses C++20 (`requires`/`consteval` in S1.5 code glued together and misindented); it would need `c++20`, which also turns `> >` into `>>`; (2) `ar & SKELETON_NVP_(x);` becomes `ar& ...` (11 lines; read as a declaration, no option fixes it, `StatementMacros` doesn't help); (3) `{ body }` on the line after a signature is always expanded to 3 lines; (4) `EmptyLineBeforeAccessModifier: Leave`, `FixNamespaceComments: false`, `ConstructorInitializerIndentWidth: 2` were needed to match the existing code. Trial configs and diffs: `/data2/smythdata/cxx-python-skeleton-build/s16_clang_format_trial/`. **Instead, a hand cleanup** (`claude_notes/style_cleanup.py`, run once from the repo root) over `lib/` and `swig_rules/lib/` C++ and all our `.i` files, keeping the majority forms (checked against geocal): tabs → spaces (8-column stops, alignment kept), no trailing whitespace, `if(`/`for(`/`while(`/`switch(`/`catch(` (geocal 150:2 for `catch(`), `template<`, `Type& x`/`Type* x`, `Archive& ar` (the skeleton's `Type& x` rule; geocal itself mostly has `Archive & ar`, 583:33), no space before `)`, function `{` on its own line, one declarator per line for pointers. Left alone: `> >` (used everywhere, `>>` nowhere), cast spacing (both forms common), SWIG's `< TYPE >` spacing in `.i` macros. **Not touched:** the upstream forks `my_shared_ptr.i` (SWIG) and `serialize_shared_ptr.h` (Boost), so they stay diffable; `python_lib_init.h` (Python C-API docs style) and `swig_vector_shared_ptr.i` (SWIG traits style) got whitespace cleanup only. Result: 69 files, 41 of them sources; ignoring whitespace, 22 files and ~30 lines. Regenerated wrappers differ only in the copied `.i` code (mostly the `swig_exception.i` handler, once per wrapped function) and one trailing space in a pickle docstring. 0 warnings; `make check` 30/30, `make installcheck` 89 passed, 1 xfailed in `build`, `build-rpath`, `build-gcc14`; no-serialization 20/20. |
| S1.6 revisit: clang-format | done (skeleton), geocal under review | 2026-11-05. Procedure, inputs and commands: §5b. Maintainer asked to try [`clang-format-auto-infer`](https://github.com/ammen99/clang-format-auto-infer) (genetic search for the `.clang-format` that changes the existing code least, fitness = `git diff --shortstat` insertions+deletions). Run on throwaway repos holding copies of the C++ files only (the tool runs `clang-format -i` and `git restore .` on the repo it is given, so never on a working tree with uncommitted work), with `TMPDIR` in scratch space and its deps (PyYAML, nevergrad) in a scratch venv. Start: `s16_clang_format_trial/trial-v2.clang-format`; forced by hand for correctness: `Standard: c++20`, `SortIncludes: Never`, `QualifierAlignment: Leave`, `UseTab: Never`. **Skeleton** (45 files, after the hand cleanup): 1909 → **1727** changed lines (4,108 evaluations, all 4 islands converged); it changed 5 options (`AllowShortFunctionsOnASingleLine: InlineOnly`, short ifs/loops never on one line, `SplitEmptyFunction: true`, `BreakTemplateDeclarations: MultiLine`). Result: 824 of 4,560 lines change (18%), 343 ignoring whitespace (7.5%). **geocal** (652 `lib/` files, 109k lines, not yet hand-cleaned): starting from the skeleton's optimum the search found nothing better (73,220, 2,018 evaluations), so **one config for both**: 36,529 lines (33%), 13,925 ignoring whitespace (13%); whitespace cleanup alone (tabs, trailing) would already change 14,044. **Not reachable by any option:** `//---` doxygen blocks inside a class are indented to the member level (`CommentPragmas` doesn't help), `/****//**` banners become `/****/ /**` (doxygen output unchanged, see below), `{ body }` after a signature is expanded, `ar & X` becomes `ar& X`, geocal's aligned parameter lists are repacked. **Skeleton reformatted** (`clang-format -i` on `lib/` and `swig_rules/lib/` `.h`/`.cc`; `.clang-format-ignore` excludes `bindings/**` and the upstream copies `serialize_shared_ptr.h`, `python_lib_init.h`; `.i` and `.sample` files aren't formatted). Checks: 0 warnings, `make check` 31/31, `make installcheck` 101 passed, 1 xfailed, no-serialization 20/20; **forced regeneration of every wrapper (32 SWIG + doxygen runs) gave byte-identical wrappers and `.py` docstrings**, so the banner change doesn't change what doxygen extracts. **Maintainer approved the skeleton's clang-format (2026-11-05), and then geocal's** after reviewing its reformat. **geocal checks** (`geocal-build/build-pixi`, gcc 15, maintainer mode): full rebuild 0 new warnings (only the known Blitz `-Warray-bounds`/`-Wmaybe-uninitialized` false positives); forced regeneration of all wrappers (450 SWIG + doxygen runs) changed **one** docstring, `TimeTable` in `time_table_wrap.cc`, for the better (trailing whitespace in the source comment had made a stray line break); C++ `geocal_test_all` 623/623 (one transient failure of `rsm_nitf/basic_test`, which calls an external program through `boost_pcall`, passed alone and in a rerun of the whole suite); Python `pytest -n 10 tests` 218 passed, 118 skipped (frenchpeas lacks MSP, ISIS, `/raid` data). Tool outputs: `/tmp/.../scratchpad/cfai_work` (not kept); the final config is the committed `.clang-format`. |
| `make format` / `format-check` / `lint` | done | 2026-11-05, both repositories (same `.clang-format`, `.clang-tidy`, scripts). `format`: `clang-format -i` on the tracked C++ (file list from `git ls-files`; `.clang-format-ignore` applies). `format-check`: `--dry-run --Werror`. `lint`: `make_compile_commands.py --clangd` (library, tests, generator and `_swig_wrap.la`, so the same database serves clangd), then `run_clang_tidy.sh --config-file=<srcdir>/.clang-tidy --warnings-as-errors='*'` in parallel on the source-tree files only (files generated in the build directory have no `.clang-tidy` above them, and the wrappers are skipped). `.clang-tidy` enables the S1.3 checks. clang-format is pinned (`clang-tools = "22.*"`) because its output differs between versions. No wrapper script is needed for clang-format: unlike clang-tidy it doesn't compile anything, so it needs no compile flags. Checked on the skeleton: `format-check` and `lint` pass, and both fail (exit 2, right file and line) on a deliberately bad line. **Fixed after the first geocal run**, which said "no findings" because it had checked **nothing**: (1) `make_compile_commands.py` only recognised compilers named `g++`/`gcc`/`c++`/`cc`, so conda's activated `x86_64-conda-linux-gnu-c++` gave 0 entries (now matched by suffix); (2) `make lint` now fails if there are no files to check, and prints how many it checked; (3) conda's debug flags have the gcc-only `-fvar-tracking-assignments`, which makes clang refuse the file, so the script drops `-f(no-)var-tracking*`. Skeleton `make lint` now passes in both env kinds (`build`, plain gcc; `build-rpath`, conda's prefixed gcc with its debug flags): no findings in 21 files. (4) The second geocal run had 80 parse errors from gcc's `mmintrin.h`: `run_clang_tidy.sh` (since S0.4) put gcc's builtin header directory on clang's include path, because conda's `clang-tools` has no clang resource headers, and gcc's x86 intrinsics headers call gcc-only builtins. **Fix:** add **`clang-22 = "22.*"`** to the env (it owns `lib/clang/22/include`; the `clang` meta package would also bring the compiler driver and, in geocal, a binutils 2.44 → 2.46.1 linker upgrade) and drop gcc's header directories from both scripts; `run_clang_tidy.sh` now stops with a clear message if the headers are missing. Lock changes: only `clang-22` and `compiler-rt22`. Both scripts are identical in the two repositories. **geocal `make lint` (2026-11-05, frenchpeas): 397 files, 0 parse errors, 2,016 unique findings in 273 files** (1,500 `use-override`, 395 `use-equals-default`, 81 `use-nullptr`, 16 `loop-convert`, 12 `use-noexcept`, 12 `deprecated-headers`); headers are counted once, although clang-tidy reports them for every file that includes them (93,935 lines). That is the size of geocal's S1.3 (its §8 cleanup). geocal: `format-check` passes; `lint` reports the S1.3 findings geocal hasn't fixed yet. |
| S1.7 mechanical tests | done | 2026-11-05. C++ tests were added with S1.4 (`printable/format`: formatter == `<<`) and S1.5 (`skeleton_exception/format_constructor`, `source_location`, `manipulators`, `stacktrace`, copy). **Python `str()` unchanged:** built the pre-S1 commit `75e734f` in `/data2/smythdata/cxx-python-skeleton-build/pre_s1` and printed `str()` of all 16 exposed classes plus the `RuntimeError` text of a C++ `Skeleton::Exception` and a `std::exception`, from both builds (checked by module path which build each run imported; `python -I` would have ignored `PYTHONPATH`): **identical**. Added exact `str()` asserts for the three classes that had none (`DirectorExampleNested`, `ArrayTypemapSample`, `FileReferenceSample`) to `printing_test.py`; the updated file also passes against the pre-S1 build. Found: `GenericObjectB2` has no `__str__` in Python (its `.i` doesn't declare `print_to_string`; C++ prints `"B2"`); unchanged, left alone. Python: 90 passed, 1 xfailed. |
| **S1 exit** | **done** | 2026-11-05. gcc 15 (`build`, `build-rpath`) and gcc 14 (`build-gcc14`): C++20, 0 warnings (library, wrappers and test programs), `make check` 30/30 test cases, `make installcheck` 90 passed, 1 xfailed; `--without-boost-serialization` 20/20; **ASan** (`build-asan`, clean rebuild): C++ 30/30 with no memory errors, the only LeakSanitizer report is the accepted truncated-archive leak (decision 20; all other tests clean with leak checking on); Python under ASan 90 passed, 1 xfailed, no errors. Suggested tag after committing: `phase1-cxx20`. |
| S2 Boost → std | done | 2026-11-05. `swig_rules/lib/serialize_function.cc`: `boost::regex_search(Fname, regex("\\.gz$"))` → `Fname.ends_with(".gz")` (2×; the only difference: Perl `$` also matched before a final newline), `boost::filesystem::path` → `std::filesystem::path` (2×, only `parent_path()`). `DirChange` kept as is: it was never Boost (POSIX `open`/`chdir`/`fchdir`), and restoring through the saved directory handle still works if the old directory was renamed meanwhile, which a saved `std::filesystem::current_path()` wouldn't. `lib/generic_object_map.h` and `swig_rules/lib/generic_object_map.h.sample`: `boost::copy(*this \| adaptors::map_keys, ...)` → `std::ranges::copy(std::views::keys(*this), ...)`; dropped the unused `boost/assign.hpp`. `swig_rules/lib/swig_type_mapper_base.h`: `type_index` loses its `boost::totally_ordered` base (now `operator==` + `operator<=>`), but still compares type **names**; not replaced by `std::type_index`, which on libc++ can compare `type_info` addresses, and those can be duplicated across shared libraries such as Python extension modules (comment updated). `swig_rules/include/swig_boost_array.i`: `BOOST_FOREACH` → range `for`; `<boost/foreach.hpp>` dropped from `swig_boost_array_inc.i` (no geocal or framework `.i` uses `BOOST_FOREACH`). No `noncopyable`/`scoped_ptr` here. `skeleton.am`: `libskeleton` now links only `BOOST_IOSTREAMS_LIB` and `BOOST_SERIALIZATION_LIB` (was also regex, date_time, filesystem and, through `ac_boost.m4`, process); `readelf -d libskeleton.so.0` NEEDED: `libboost_iostreams`, `libboost_serialization` (no-serialization build: iostreams only). Shared `config/m4/ac_boost.m4` unchanged (geocal needs those libraries). Kept for later: `boost::optional`, `boost::array` (S3), `shared_ptr`/`weak_ptr`/`make_shared` (S4), `boost::rational`, Serialization, iostreams, Test, Stacktrace (kept by decision). Wrapper diffs: only the 5 `BOOST_FOREACH` loops and the dropped include. **Found (old bug, fix in S3):** `print_to_string()` of the SWIG `boost::array` classes (`Array_double_3` etc.) returns an opaque `std::string *` that SWIG reports as leaked, because `swig_boost_array.i` has no `std::string` typemaps; same in the pre-S1 build. Not tested anywhere; S3 rewrites the file for `std::array`, so fix and test it there. Results: 0 warnings; `make check` 30/30, `make installcheck` 90 passed, 1 xfailed in `build`, `build-rpath`, `build-gcc14`; no-serialization 20/20. Suggested tag: `phase2-boost-std`. |
| S3.1 golden archives with `boost::` types | done | 2026-11-05. The plan assumed no sample; ST.3's `SwigRulesSample` has `boost::array`/`boost::optional` members, but it isn't serialized, so nothing serialized used them. New **`ArraySample`** (`lib/array_sample.{h,cc,i}`, the sample S5 extends): `boost::array<double,3> position_` (public, `%python_attribute_boost_array`), `boost::array<boost::array<double,3>,2>` (property `matrix`, a 2 x 3 numpy array), `boost::optional<int> count` (set by default) and `boost::optional<double> scale` (empty), serialized, `%pickle_serialization`. Golden archives `array_sample` (all members changed from the defaults, both optionals set) and `array_sample_empty` (both empty, which also checks that loading clears `count`), XML and binary, written with the `boost::` types **before** S3.2. `make golden-archives` regenerates the whole corpus, and **every existing golden file came out byte-identical**: after S1 and S2 the old objects are still written exactly as before. Checks: C++ `golden_archive/array_sample` (values, and *rewriting what was read gives the golden file byte for byte*, XML and binary, so S3.2 has to keep the layout, not only read it); Python `golden_archive_test.py::test_array_sample`; new `array_sample_test.py` (properties from list/tuple/numpy/strided input, wrong sizes raise, `None` for empty optionals, pickle round trip). Boost 1.90 serializes `std::optional` with the same code as `boost::optional` (`detail::save_impl`, class version 1 for both) and `std::array` like `boost::array` (`elems`), so S3.2 should be layout compatible; the byte-identical check decides. Results: 0 warnings; `make check` 31/31 test cases, `make installcheck` 101 passed, 1 xfailed in `build`, `build-rpath`, `build-gcc14`; no-serialization 20/20. |
| S3.2 `std::optional`, `std::array` | done | 2026-11-05. **Typemaps** (`swig_rules/include`): `swig_boost_optional.i` is now one macro, `%swig_optional_typemaps(OPT)`, used for `boost::optional` and `std::optional` (`.get()` → `*`; file name kept so `%import`s don't change). `swig_boost_array.i` gets `%std_array_template(TYPE, LEN, PRECEDENCE)` for `std::array` (in: anything numpy converts, of the right length; out, by value or const&: a **new 1-d numpy array**, not a wrapped class like `boost::array`'s `Array_double_3`), instantiated for the same types/lengths as the boost ones, and `%python_attribute_std_array` (same as the boost macro, which works for any container); helpers `to_std_array`, `std_array_to_numpy` in `swig_boost_array_inc.i`. The `boost::` typemaps stay (geocal uses them). **Code:** `ArraySample`, `SwigRulesSample`, `NumpySample::maybe_range` now use `std::optional`/`std::array`; `ArraySample` gains `position()`/`position_ref()` returning `std::array` (tests the out typemaps). No `boost::optional`/`boost::array` left in the skeleton's own code, so the `boost::array` *argument* typemaps are now exercised only by geocal's tests (the optional ones share the macro text). **Two needed fixes:** (1) `skeleton_serialize_support.h` (and `serialize_support.h.sample`) included `boost/serialization/array.hpp` only when `boost_array.hpp` didn't exist; it holds the `std::array` serializer, so it is now always included. (2) **Boost bug** ([boostorg/serialization #319](https://github.com/boostorg/serialization/issues/319), since Boost 1.84, still in 1.90): serializing `std::optional` doesn't compile when `shared_ptr_helper.hpp` is included (it declares `load(Archive&, SPT< class U >&, ...)` at namespace scope, `class U` being an incomplete class, and overload resolution for a `std::optional` load instantiates `std::optional<U>`, an error with gcc 15's libstdc++). Workaround in `swig_rules/lib/serialize_shared_ptr.h`: define `BOOST_NO_MEMBER_TEMPLATE_FRIENDS` just around that include (Boost then leaves the declaration out and makes the helper's members public). **Old bug fixed** (found in S2): `swig_boost_array.i` lacked `std_string.i`, so `print_to_string()` of `Array_double_3` etc. returned a leaked `std::string*`; now a `str` (test added). **Golden archives:** the S3.1 files (written with the `boost::` types) load, rewriting them gives the same bytes, and `make golden-archives` with the `std::` code changes **no** golden file. Python-visible changes: docstrings, the two new methods. New tests: `test_std_array_return`, `test_boost_array_print_to_string`. Results: 0 warnings; `make check` 31/31, `make installcheck` 103 passed, 1 xfailed in `build`, `build-rpath`, `build-gcc14`; no-serialization 20/20; `make format-check`, `make lint` clean. |
| **S3 exit** | **done** | 2026-11-05. Suggested tag: `phase3-std-types`. |
| S5 samples | done | 2026-11-06. New in `lib/` (C++ tests, SWIG `.i`, Python tests): **`observer.h`** (framework's `Observer`/`Observable`/`CacheInvalidatedObserver`, `std::` algorithms, `null_deleter.h`, `observer_serialize_support.h` with `SKELETON_EXPORT_OBSERVER_KEY`/`SKELETON_OBSERVER_SERIALIZE`); **`ObservableExample`, `ObserverExample`** (registers in its constructor and again in `load()`), **`CacheExample`**; a director on the template instance `Observer<ObservableExample>` (`ObserverObservableExample`, and `ObserverCameraLike`); **`WithParameterLike`, `CameraLike`/`SimpleCameraLike`, `MultiBaseExample`** (Printable + Observable + second base; returns as every base, static functions taking every base); **`NestedLike`, `RootLike`, `Diamond`, `VirtualDiamondExample`** (virtual diamond); **`ColumnMajorSample`** (in `array_sample.h`; separate from `ArraySample` and not serialized, because the Blitz serialization doesn't record storage order); **`OverloadExample`** (array / `shared_ptr<NumpySample>` / string overloads, S6.1 item 7); **`ThreadExample`** (calls `func` on `std::thread` workers, rethrows the first `exception_ptr`; answers the §13 threading question: added); `ExceptionSample::caught_exception` plus `Exception.stacktrace()`/`source_file()`/`source_line()`/`source_function()` in Python. Golden archives `observer_map`, `observer_ctor`, `simple_camera_like`, `diamond` (old files unchanged), and a new C++ test `golden_archive/rewrite_identical`: every object archive, read and written again, is byte-identical (XML and binary). **Bug found in the copied pattern:** an observer that registers in `load()`, read from inside an Observable's `olist` (the Observable read first), ended up in the list twice and was notified twice. Fixed in `Observable::load` (`merge_loaded`: archive order kept, so the archive format and rewrite are unchanged); framework's `FP_OBSERVER_SERIALIZE` has the bug ("Notes for geocal"). |
| S5 SWIG results (`swig-final` reference) | done | 2026-11-06, `build` (gcc 15.3, SWIG 4.5.0, Boost 1.90, Python 3.13). **C++: 45 test cases pass. Python (`make installcheck`): 148 passed, 5 xfailed.** The strict `xfail`s: `test_director_example_object_tracking` (as before); `column_major_sample_test.py::test_scale_in_place` ×3 (SWIG has no typemap for a non-const `blitz::Array&`; nanobind should pass); `overload_thread_test.py::test_thread_python_override` (a Python override called on a C++ worker thread **segfaults** under SWIG: no `-threads`, no GIL; nanobind should pass). Recorded SWIG behavior the S6 tests must revisit: a new wrapper per return (`test_identity`), `isinstance(cam, WithParameterLike)` is true (every base is a Python base), `const blitz::Array&` returns are read-only views keeping C++ alive, column-major returns are F-contiguous, numpy float64 input of any layout is zero-copy, other dtypes are copied, `float32` arrays pick the array overload. `--without-boost-serialization`: 29/29. `make format-check`, `make lint` clean. |
| **S5 exit** | **done** | 2026-11-06, committed (maintainer asked for commits for S5/S4/S6). Suggested tags: `phase5-samples`, and `swig-final` (last state with working SWIG bindings). |
| S4 `std::shared_ptr` | done | 2026-11-06. **`script/boost_to_std_ptr.py`** (for reuse in geocal/framework): rewrites `boost::`/`::boost::` `shared_ptr`, `weak_ptr`, `make_shared`, `allocate_shared`, `enable_shared_from_this`, `bad_weak_ptr`, `owner_less`, the four `*_pointer_cast`, and the smart_ptr includes → `<memory>` (once per file); reports what it won't touch (`scoped_ptr`, `intrusive_ptr`, `shared_array`, `BOOST_SP_*`, `using boost::...`); skips `*_wrap.*`; `.i` files only with `--swig`; `--dry-run`. On `lib/` and `swig_rules/lib/`: 327 changes in 34 files. Hand fixes: `BOOST_SP_NOEXCEPT` → `noexcept` (reported by the script), and a missing `<vector>` in `generic_object_map.h` (and its `.sample`), which Boost's headers had pulled in. **`serialize_shared_ptr.h`**: same code on `std::shared_ptr` (`our_shared_ptr` derives from it; version 1, `track_never`, `px`); its director hook is unchanged. **Aliasing constructor** (step 4): `to_ptr`/`const_pointer` (`null_deleter.h`, so also `Observer::this_obj` and `CacheInvalidatedObserver::this_obj_`) and the temporary null-deleter pointer in `our_shared_ptr::reset` are built from a `shared_ptr<void>`; new test `observer/shared_from_this_owner` (with `enable_shared_from_this` on a test class: `shared_from_this()` is the real owner; a stack observer has none). The `.i` files and the committed wrappers are not changed (§8), so plain `make` fails until S6; use `make skeleton-check`. **Acceptance:** C++ 46/46 (`build`, gcc 15); all golden archives load and `golden_archive/rewrite_identical` passes (every object archive byte-identical, XML and binary); `make golden-archives` changes no file; `--without-boost-serialization` 30/30; ASan (`build-asan`): no memory errors, only the accepted truncated-archive leak (finding B). |
| **S4 exit** | **done** | 2026-11-06, committed. Suggested tag: `phase4-std-shared-ptr`. |
| S6.1 design spike | done | 2026-11-06. All 11 items, as real tests (§10): (1) forked caster `swig_rules/nanobind/gc_shared_ptr.h` (nanobind 3.1.0; `dynamic_pointer_cast` for `enable_shared_from_this`, most derived *registered* type with a `dynamic_cast` to its subobject, `None` → empty pointer, registry fallback in the converting pass); `GenericObject : std::enable_shared_from_this<GenericObject>` (virtual root, compiles with the fork); no virtual base is ever a nanobind base. (2) `type_registry.h`: per bound class a `GenericObject*` → `B*` cast and a python → `shared_ptr<GenericObject>` function; trampolines registered as aliases; shared across modules through a capsule in `builtins`. (3) aliasing `this_obj` (S4) tested from python too (constructor-registered and python subclass observers notified). (4) `weak_ptr` matrix `python/tests/unit/nanobind_test.py`: plain classes use `nb::new_` + `make_shared` (always valid); **director classes** need `nb::init` (nanobind's `nb::new_` wraps a python subclass around a base instance, so overrides wouldn't work), so like python subclasses a `weak_ptr` is valid only while a C++ `shared_ptr` owns the object (decision 6 extended; `test_director_example_weak_ptr` changed to record it). (5) pickling: `__reduce__` helpers; python subclass objects in archives through `SWIG_RULES_NB_DIRECTOR_SERIALIZATION` (python part pickled as class + `__dict__` in SWIG's form, element and class names as SWIG wrote them): **shelves written by SWIG with a python subclass load** (`unit_test_data/golden/director_python*.xml`, written by the S5 SWIG build, class `python/tests/fixtures/director_golden.py`); identity within one archive kept (`u.value() is u.value_vec()[1]`). The S0 strict `xfail` `test_director_example_object_tracking` still fails, now only at the pickle of C++ objects inside a python `__dict__` (separate archives; re-entrant pickling set aside, decision 3). (6) exceptions: `register_exceptions<Exception, TypeError>`: `Skeleton::TypeError` (new, a library type) → `TypeError`, `nb::cast_error` → `TypeError`, every other `std::exception` → `RuntimeError` as with SWIG (nanobind's own mapping would give `IndexError` etc.), non-`std::exception` → `RuntimeError("unknown C++ exception")`; a `Skeleton::Exception` raised into python carries `cpp_exception` (copy, `.stacktrace()`) and `cpp_source`. (7) numpy ↔ Blitz through `mdspan` (conda-forge `mdspan` `0.6.1.dev20260223`, `lib/skeleton_mdspan.h`, `lib/blitz_mdspan.h` + `blitz_mdspan_test.cc`: contiguous, strided, column-major, owner lifetime, negative stride rejected; python side `swig_rules/nanobind/blitz_array.h`): exact dtype zero copy any strides, otherwise `numpy.array(copy=True)` in the converting pass only; returns are views (read-only except `T&`). Overload dispatch matches SWIG (float32 → array overload). New: a non-const `blitz::Array&` argument works (was a strict `xfail` under SWIG). (8) iostream: `iostream.h` streambuf (unbuffered input, file position as before). (9) `nb::bind_vector` for each `%template(Vector_*)` (`NB_MAKE_OPAQUE` in `lib/skeleton_nb_opaque.h`), STL caster elsewhere. (10) `lib/downstream_nb.cc` → `skeleton_swig._skeleton_downstream`: creates/takes main-module types, a class derived from a main-module class, `GenericObject` returns via the shared registry. (11) `ThreadExample.run` with `nb::call_guard<nb::gil_scoped_release>`: python override on C++ worker threads works and its exception arrives with type and message (segfaulted under SWIG). |
| S6.2–6.5 rules, bindings, build, stubs | done | 2026-11-06. **Converter** `swig_rules/nanobind/i_to_nanobind.py` run once on the 20 `.i` files → `lib/*_nb.cc`; **hand changes: 92 of 2327 lines, in 7 files** (bytes typemaps in `serialize_function.i`, `GenericObjectMap`'s `%pythoncode`, `Exception`'s `%extend`, `GenericObject`'s output typemaps (now the caster's job), `throw_type_error`, the GIL release, a `%extend` comment). Converter features found necessary while testing: `.none()` on every `shared_ptr` argument (SWIG accepted `None`), setter types of virtual properties from the header, a non-virtual call of the C++ implementation when a python subclass calls `super()` (else the override ran twice), unique names for several `OUTPUT` arguments. **Docstrings** `doxy2nanobind.py` (doxygen XML → `swig_rules_nb::docstring_table()`, template members found without their arguments; nanobind crashes on a null docstring, so missing ones are `""`); pybind11_mkdoc not evaluated (needs libclang python bindings; the doxygen route reuses what exists). **Build**: `config/m4/ac_nanobind.m4`, `swig_rules/nanobind/nanobind.am` (nanobind's `nb_combined.cpp` once, hidden visibility, in `libnanobind_core.la`; binding files **default** visibility, see Notes for geocal), `--without-python-bindings`, extension skipped automatically with `--without-boost-serialization`; `make check` runs the python tests against the build tree (`nb_pkg/`, `LD_PRELOAD` of `.libs/libskeleton.so` for the ST.11 RPATH reason); `.pyi` stubs by `nanobind.stubgen` installed with `py.typed`; `mypy` on `python/skeleton` clean. **CMake reference** `CMakeLists.txt` (`nanobind_add_module`, library, docstrings, `ctest` runs C++ and python tests from the build tree): builds and passes. Also: `Exception::stacktrace()` now runs addr2line once per module with module-relative addresses (Boost's addr2line backend looked every frame up in the main executable: wrong file:line for shared libraries, 11 s per trace from python; now 0.4 s, correct). |
| S6.6 remove SWIG | done | 2026-11-06. Removed: `lib/*.i`, `bindings/` (committed wrappers), `swig_rules/include/`, `create_python_init.py`, `swig_wrap.tmpl`, `doxy2swig.py`, `doxygen_single.cfg`, `interface_file_with_doc.sh`, `README.doxygen`, the SWIG CMake files, `swig_type_mapper*`, `python_exception.h`, `python_ref_ptr_cleanup.h`, `python_lib_init.h`, `swig_to_python.h`, the `.i.sample` files; `swig` from `env/linux/pixi.toml` (+ lock; `mdspan`, `cmake`, `ninja` added). `swig_type_mapper_base`'s director hook replaced by `swig_rules/lib/director_hook.h` (python-free). Rewritten: `DirectorNotes.md`, `swig_rules/README.md`, new `swig_rules/nanobind/README.md`, `README.md` (naming note), `CLAUDE.md`. `config/m4/ax_pkg_swig.m4` and the other shared m4 files are left (shared with geocal, unused here). |
| **S6 exit** | **done** | 2026-11-06, frenchpeas. **gcc 15** (`build`, nanobind 3.1.0, Boost 1.90, Python 3.13): 0 warnings; `make check`: C++ 51/51 test cases, python **168 passed, 1 xfailed** (against the build tree); `make install` + `make installcheck`: the same. **gcc 14** (`build-gcc14`, conda compiler activation incl. `-fvisibility-inlines-hidden`): 0 warnings, the same results. **`--without-boost-serialization`** (`build-noserial`): plain `make` and `make check` now work (the extension isn't built; `golden_archive_generator` needed a guard), C++ 30 test cases. **ASan** (`build-asan`): C++ no memory errors, the only leak is finding B (now 224 bytes: the same 4 allocations, each 16 bytes bigger with `enable_shared_from_this`); python 168 passed, 1 xfailed, no ASan errors (`detect_leaks=0`). **CMake reference** (`build-cmake`): `ctest` C++ and python pass. `make format-check`, `make lint` (55 files, now including the binding code) clean; `make golden-archives` changes no file; all golden archives (and the SWIG-written python subclass shelves) load. The one strict `xfail` is `test_director_example_object_tracking` (decision 3). Committed; suggested tag `nanobind-1`. |
| S7.1 C++23 | done | 2026-11-06. `-std=c++23`: `skeleton_common.m4` (`AX_CXX_COMPILE_STDCXX([23], ...)`, the shipped macro supports it), `nanobind.am` (`libnanobind_core`), `CMAKE_CXX_STANDARD 23` (CMake not rebuilt, set aside). `.clang-format` stays `Standard: c++20` (clang-format 22 has no `c++23`; shared with geocal). `skeleton_mdspan.h`: dropped `MDSPAN_USE_PAREN_OPERATOR`, Kokkos uses `m[i, j]` in C++23 mode; `blitz_mdspan_test.cc` changed to match (with parentheses inside `BOOST_CHECK_EQUAL`, the comma splits macro arguments). Checked: gcc 15.3 has no `<mdspan>`, so `std::mdspan` needs gcc 16 (corrects what was said in chat). Language features used so far: multidimensional subscript; `views::cartesian_product` (gcc 13). Not done: deducing `this` for `Printable` (changes every class's base in geocal; planned for later, skeleton first, §11), `std::expected`. |
| S7.2 `mdspan` sample | done | 2026-11-06. `lib/raster_sample.{h,cc}`, `raster_sample_nb.cc` (hand written), tests `raster_sample_test.cc` (6 cases) and `python/tests/unit/raster_sample_test.py` (8). Based on geocal: `RasterImage::read_ptr(..., int* Res)` (about 14 overrides, not in any `.i`) and `CalcRaster` (director, python fills `self.data` in `calc()`). Classes: `RasterSample` (abstract; virtual `read_into(Lstart, Sstart, RasterView)` and `write_from`, `RasterView` = `layout_stride` mdspan of `int`; non-virtual `read_ptr`/`write_ptr` wrappers, `read` → blitz/numpy, `read_with_pad` reading straight into a `submdspan` of the result, `write`), `MemoryRasterSample` (blitz assignment through `to_blitz` of the view), `BufferRasterSample` (a stand-in C API needing contiguous row-major memory: passes the pointer through when it can, else a temporary, counted by `number_copy`), `SubRasterSample` (forwards the view with an offset), `CalcRasterSample` (director; writable numpy `data`; protected `data_ptr` bound through a publicist), `copy_raster` (tiles through one `std::vector` buffer). Python: `read_into(l, s, out)` fills a writable `int32` numpy array in place, any strides; wrong dtype, read-only, a list or negative strides give `TypeError`. Found: a trampoline override of `desc()` needs `desc` bound on the class, else nanobind fails with "trampoline_enter ... lookup failed" (dropped the override). Not covered: serialization of these classes (the director serialization is already tested by `DirectorExample`). |
| S7.3 bootstrap warning | done | 2026-11-06. `configure.ac:11: AC_PROG_AWK was expanded before it was required`: `AM_INIT_AUTOMAKE` inside `SKELETON_COMMON` expands it, then `LT_INIT` `AC_REQUIRE`s it. Fixed with `AC_REQUIRE([AC_PROG_AWK])` at the top of `SKELETON_COMMON` (hoisted); `./bootstrap` now prints no warnings. |
| **S7 exit** | **done** | 2026-11-06, frenchpeas. **gcc 15** (`build`): 0 warnings; `make check` C++ 57 test cases, python **176 passed, 1 xfailed**; `make install` + `make installcheck` the same. **gcc 14** (`build-gcc14`): 0 warnings, the same. **`--without-boost-serialization`**: `make check` passes, 41 test cases. **ASan**: C++ no memory errors, only the accepted 224-byte leak (finding B); python 176 passed, 1 xfailed, no ASan errors. `make format-check`, `make lint` (58 files) clean. CMake reference not rebuilt (set aside, decision 22). Staged, not committed. |
| Rename (decision 23) | done | 2026-11-06. `git mv` of `swig_rules/` and the files named after it (table in §0 "Naming"), then over the tracked files except `claude_notes/`, `unit_test_data/`, the shared SWIG m4 macros and generated files, in this order: `swig_rules_nb`, `SWIG_RULES_NB`, `SWIG_MAPPER_NAMESPACE`, `SWIG_HAVE_BOOST_SERIALIZATION`, `SwigRulesSample`, `swigrules`, `swig_rules`, `swig-rules-skeleton` (not after `/`, so local paths stay), `swig-rules-repo`, `swig-rules`, `skeleton_swig`, `_swig_wrap`, `pkgpythonswig`, then the remaining `SWIG_RULES` macros. By hand: comments that called the new names "historical", references to the removed `swig_rules/include` put back, subtree notes in `CLAUDE.md`/`README.git` (the subtree history is under the old prefix). Removed unused SWIG leftovers: `SWIG_PYI` (`common.am`), `pythonswigsrc`, `swigsrc`, `skeletonswigincdir`, `swigincdir` (`skeleton_source_directory.m4`). No golden archive or shelf contains a renamed name (checked), so the test data is unchanged. Caught on the way: `perl -pi` had replaced the `README` → `README.md` symlink with a regular file (restored; geocal: check `git status` for `T` after a scripted rename); `make format` needed after the rename (the longer names moved some line breaks). **Bug found:** mypy now sees the stubs of `skeleton_cxx` and found that `write_shelve(Path(...), v)` for `.xml`/`.bin` passed the `Path` to the C++ `serialize_write`, which takes a `std::string` (`TypeError`); fixed with `str(f)` in `swig_shelve.py`, new test `test_path_argument` (geocal's `swig_shelve.py` has the same code). **Results:** gcc 15: 0 warnings, `make check` C++ 57 test cases, python **179 passed, 1 xfailed** (3 new); `make install` (old `skeleton_swig/` removed from the env first) + `make installcheck` 176 passed, 1 xfailed (before the new test); gcc 14: 0 warnings, 176 passed, 1 xfailed; no-serialization 41 test cases; ASan: no memory errors, only the accepted 224-byte leak, python 176 passed; `make format-check`, `make lint` (58 files), `ruff`, `mypy --disallow-untyped-defs` clean (ruff `SIM115` in `director_example_test.py` was there before). CMake not rebuilt (set aside). Staged, not committed. GitHub repositories renamed the same day. **Subtree pushed** (maintainer request, 2026-11-06) to `cxx-python-rules` as branch `modernize` (`d23035d`): `git subtree split --prefix swig_rules` of HEAD (`549b6f5`, keeps the subtree history) plus one commit with the staged `cxx_python_rules/` tree (`git write-tree --prefix=cxx_python_rules/`, `git commit-tree`), since the rename isn't committed here yet. Remote `cxx-python-rules-repo` added in this checkout. Upstream `master` (`7309cc6`) is not an ancestor: it has one commit after the merge base `246446e` that never came into this repository ("Add license"; merge it when `master` is updated). |
| Directory rename, frenchpeas | done | 2026-11-06. The maintainer renamed the checkout to `/data2/smythdata/cxx-python-skeleton` and the scratch area to `cxx-python-skeleton-build` and recreated `build` (and its pixi env). The other build directories still had the old paths in `config.status`/`Makefile`/`CMakeCache.txt`, and the `build-gcc14` and `build-rpath` pixi envs in their scripts (shebangs, activation). **Recreated** (§0 "After moving or renaming"): both envs with `pixi install --frozen` (same packages); each build directory emptied (logs kept) and configured again with its old arguments, paths changed. **Results:** `build-gcc14`, `build-rpath`: `make check` and `make install` + `make installcheck`, python 179 passed, 1 xfailed; `build`: `make check` the same; `build-noserial`: `make check` passes; `build-coverage`: `make skeleton-check` 57 test cases, no errors (now with `cxx_python_rules/`, fixing the gcovr filter in §4a); `build-asan`: 57 test cases, no memory errors, only the accepted 224-byte leak (finding B), so `skeleton-check` exits nonzero as before; `build-cmake`: `ctest` 2/2. Gotcha: a `pixi shell` started before the rename keeps the old paths in its environment. |

---

**Naming.** If the bindings move to nanobind, the names `swig_rules/` and the `*_swig` Python module names no longer
describe the contents. We keep them for now, for historical reasons and so that downstream paths keep working.
(*Superseded 2026-11-06 by decision 23: renamed, see §0 "Naming".*)
(2026-11-06: the repositories were renamed, `swig-rules-skeleton` → `cxx-python-skeleton` and `swig-rules` →
`cxx-python-rules`, remote `cxx-python-rules-repo`; build areas renamed to match.) Add a short note to `README.md` and `swig_rules/README.md` when
the nanobind step lands ("the name is historical; the directory now holds the nanobind rules").

**Branching.** This is a sandbox with no dependents. The work happens on the **`modernize`** branch, which the
maintainer created from a point before the unfinished re-entrant pickling work (it does not contain
`swig_serialization_context.*`, `SerializeSessionLock` or the pickler classes). Changes are **staged, not committed**;
the maintainer commits and can tag each phase boundary (for example `phase0-baseline`, `swig-final`, `nanobind-1`).

**cxx-python-rules repository** (formerly swig-rules). Leave `cxx-python-rules-repo` (the upstream of the `swig_rules/` subtree) alone for now. All
changes are made only in this repository's copy of `swig_rules/`, and it is understood that it will drift from
`cxx-python-rules-repo`. No `git subtree push`. **geocal copies `swig_rules/` (and changed `config/m4/` files) directly from
this repository** (§13 item 9), so keep them self-contained and copyable.

**S0 phase-end checklist (repeat at the end of each phase):** (1) gcc 15 env (`build-pixi`): `make check`, `make install`,
`make installcheck`; (2) gcc 14 env (`build-pixi-gcc14`): the same; (3) `--without-boost-serialization
--without-documentation` build with `make check` (before S6: `--without-swig` and `make skeleton-check`); (4) ASan build
(below); (5) golden archives load (part of 1-2); (6) since S6, `make format-check`, `make lint`, and the CMake reference
build. Expected at the end of S0: C++ 1/1, Python 21 passed + 6 xfailed; at the end of S6: see the S6 exit row.

**Sanitizer builds (S0.8).** No configure option was needed; this works:

    configure --enable-maintainer-mode --enable-debug --prefix=/tmp/asan_inst \
        CXXFLAGS="-g -O1 -fsanitize=address -fno-omit-frame-pointer" LDFLAGS="-fsanitize=address"
    make -j && make install && make skeleton-check          # C++ tests under ASan: clean
    source script/setup_skeleton.sh
    LD_PRELOAD=$(g++ -print-file-name=libasan.so) ASAN_OPTIONS=detect_leaks=0 pytest python/tests

Python tests under ASan: 21 passed, 6 xfailed, no errors. `detect_leaks=0` is needed for Python (it reports the
interpreter's own allocations at exit). The C++ run can keep leak detection on.

---

## 1. Where the skeleton stood before S0 (measured 2026-10-23)

*(Snapshot taken before Phase S0. Several items below were fixed in S0; the progress log in §0 has the current state.)*

| Item | State |
|---|---|
| Code | `lib/`: 27 files, ~2.3k lines; 9 `.i` files; `swig_rules/` ~9.8k lines |
| Build env (`../cxx-python-skeleton-build/build-pixi`) | gcc **15.3**, Boost **1.90**, SWIG **4.5.0**, Blitz 1.0.2, Python 3.13, numpy 2.5, doxygen 1.18. That is already newer than geocal's env (gcc 15.1, Boost 1.84, SWIG 4.3.1). |
| C++ standard | `AX_CXX_COMPILE_STDCXX([17], [ext], [mandatory])` in `config/m4_local/skeleton_common.m4` (and in the shared `config/m4/afids_common.m4`) |
| C++20 readiness | All `lib/*.cc` and `swig_rules/lib/*.cc` pass `-fsyntax-only -std=gnu++20` (with the real build's `-DHAVE_CONFIG_H -DSWIG_HAVE_BOOST_SERIALIZATION`). |
| `--without-boost-serialization` | **Broken**: 4 files fail to compile with `SKELETON_HAVE_BOOST_SERIALIZATION` undefined (`director_example.cc`, `generic_object_map.cc`, `generic_object_sample.cc`, `serialize_function.cc`; errors are `SKELETON_NVP_` / `SKELETON_GENERIC_BASE` / `make_nvp` used outside the guard). This is the same problem as geocal (12 files there). |
| Tests | C++: 7 Boost.Test cases (`generic_object_map_test.cc`, `printable_test.cc`, `blitz_array_serialize_support_test.cc`). Python: 17 tests in `python/tests/unit/`. |
| Known-broken tests on `master` | `test_director_example_weak_ptr` is `@pytest.mark.skip` (since the SWIG 4.2.1 change, see `swig_rules_review.md`). `test_director_example_object_tracking` has its key assertions commented out and refers to `PickleWithSwigSupport`, **which is not defined or imported**, so it likely fails with `NameError` (to confirm in Phase 0). |
| In-progress work | `master` carries unfinished re-entrant pickling work (`swig_serialization_context.*`, `SerializeSessionLock`, `PicklerWithSerializationSupport`; commits `46d6216`–`c03e221`). This is **not in `cxx-python-rules-repo/master`** and not in geocal's copy of `swig_rules`. **Set aside:** the `modernize` branch, where this plan is carried out, doesn't contain it. |
| Generated wrappers | Committed under `bindings/python/swig/`, same deliberate choice as geocal. |
| Downstream heritage | Same `GenericObject` / `Printable<T>` / `%python_attribute` / `my_shared_ptr.i` / serialization pattern as geocal and framework. |

What the skeleton does **not** have yet, but geocal or framework rely on. These need new samples, because otherwise
the skeleton cannot prove the steps that matter:

| Missing pattern | Where it matters |
|---|---|
| `Observer`/`Observable` with the `this_obj = to_ptr(*this)` self-pointer and a `weak_ptr` observer list | geocal `observer.h` (Camera, Orbit, IGCs); framework `Observer<T>` (150 uses, central to `StateVector`) |
| A class with **two bound bases** (for example `Camera : Printable<Camera>, Observable<Camera>, WithParameter`) | geocal: 8 SWIG classes incl. `Camera`, `Orbit`, `IgcArray`; framework: 52 |
| A **virtual-inheritance diamond** (`IgcRollingShutter : virtual ImageGroundConnection, virtual WithParameterNested`, with `WithParameter : virtual GenericObject`) | geocal (IGC collections, RSM classes); framework (169 virtual bases, e.g. `virtual public StateVectorObserver`) |
| A **director that is a template instance** (`Observer<T>`) | framework (6 `Observer<...>` directors) |
| `boost::array` / `std::array` members with serialization and SWIG typemaps | geocal (701 uses) |
| Exception with backtrace (`boost::backtrace` base) | geocal and framework `Exception` |
| Column-major (Fortran-order) Blitz arrays through the Python boundary | framework (831 `ColumnMajor`/`fortranArray` uses) |
| A second extension module that uses the first module's types (a "downstream" module) | ECOSTRESS/OCO-3 on top of geocal; instrument repos on top of framework |
| `multiprocessing.Pool` pickling | geocal `parallel_process.py` (production use) |

---

## 2. Findings that change the earlier plans

### 2.1 nanobind probe results (2026-10-23)

I built throwaway modules in `/tmp/nbtest` (a disposable pixi env: nanobind 3.1.0, gcc 15.3, Python 3.13) that mimic the
skeleton/geocal class patterns. Nothing in this repository was touched. Results:

| Question | Result |
|---|---|
| Build without CMake? | **Yes.** `g++ -std=c++20 -fPIC -fvisibility=hidden -shared -I<nanobind include> -I<robin_map> -I<python include> module.cpp <nanobind>/src/nb_combined.cpp` builds a working module in ~5 s. `nanobind.include_dir()`/`source_dir()` give the paths. The module does **not** link `libpython`. |
| Downcast `shared_ptr<Base>` → most-derived Python type (single bound base) | Works. |
| Downcast through a **non-primary non-virtual** base that is the bound base (`shared_ptr<WithParameter>` → `QuaternionCamera`) | Works, including method calls. |
| Return `shared_ptr<GenericObject>` where `GenericObject` is a **virtual** base | Gives the right Python type, then **segfaults** on the first method call. nanobind passes the base address as if it were the derived address; the virtual-base offset is not applied. |
| Declare a virtual base as the nanobind base (`nb::class_<Camera, GenericObject>`) | Compiles, **segfaults** when passing a derived object to a `shared_ptr<GenericObject>` parameter (static offset used for a virtual base). **Rule: never bind a virtual base as a nanobind base.** |
| Directors (trampolines): C++ `shared_ptr` keeps the Python subclass alive after Python drops it | Works. |
| Same C++ object returned twice → same Python object | Works (nanobind keeps an instance map; SWIG gives a new wrapper each time). |
| Python exception raised in an override propagates to the caller with the original type and message | Works. |
| `weak_ptr` to an object passed in from Python | **Expires immediately in every case**, even for objects created in C++ and even while C++ holds a strong `shared_ptr`. Without `enable_shared_from_this`, nanobind creates a **new control block for every Python→C++ conversion**. This is the same "second shared_ptr" problem `DirectorNotes.md` describes for SWIG directors, but it now affects every class. |
| Same, with `std::enable_shared_from_this` on the class | Correct when the object was created in C++, or when some C++ `shared_ptr` already owns it. A `weak_ptr` to an object owned **only** by Python still expires (no C++ owner exists; the SWIG `SHARED_PTR_NO_OWN` situation). |
| `enable_shared_from_this` on a **virtual** `GenericObject` root | **Does not compile** with nanobind's caster (it uses `static_pointer_cast` from the `enable_shared_from_this` base, which is ill-formed across a virtual base). |
| **Forked caster**: copy of `nanobind/stl/shared_ptr.h` (138 lines) with two changes: `dynamic_pointer_cast` instead of `static_pointer_cast` in the `enable_shared_from_this` path, and `dynamic_cast<void*>` to hand nanobind the most-derived address and type in `from_cpp` (what pybind11's polymorphic type hook does) | With `GenericObject : enable_shared_from_this<GenericObject>` kept **virtual**: `shared_ptr<GenericObject>` and `shared_ptr<WithParameter>` (second virtual branch) both downcast and work; `weak_ptr` behaves correctly for C++-created objects and for Python-created objects with a C++ owner. |
| Pass a derived object to a `shared_ptr<GenericObject>` parameter, when `GenericObject` is not in the bound chain | Rejected by nanobind (type error). Needs a small conversion helper (a registry from Python type to an upcast function), the reverse of the type mapper. |

What this means:
* **Keep virtual inheritance and the `GenericObject`/`Printable` design.** Removing virtual inheritance is not
  practical: geocal has real diamonds and framework has 169 virtual bases. The earlier idea (modernization_ideas.md
  10.2 item 4) of reworking `Printable`/`GenericObject` into non-virtual mixins is dropped. A side benefit: the
  serialization base-object records don't change, so XML archives are unaffected.
* **`GenericObject` gets two new jobs** in the nanobind world:
  1. **`std::enable_shared_from_this<GenericObject>`**, so `weak_ptr` and `shared_ptr` identity work across the
     boundary.
  2. **Root of a small type-mapper registry**, used by the forked caster and by the "pass to a `GenericObject`
     parameter" helper. This is the same role `swig_type_mapper` has today, in a much smaller form.
* **The rules library owns a fork of nanobind's `shared_ptr` caster**, just as it owns `my_shared_ptr.i` today.
  Document the diff and re-check it on each nanobind upgrade (S6.2).
* **`enable_shared_from_this` conflicts with `to_ptr`/`this_obj`, and the fix is small (tested 2026-10-23).**
  * *The problem:* geocal's and framework's `Observer` build a non-owning `shared_ptr` (null deleter) to themselves in
    their constructor (`this_obj = to_ptr(*this)`). With `enable_shared_from_this`, that non-owning pointer becomes the
    object's `weak_this`. Later `shared_from_this()`, and the forked caster, would then hand out a **non-owning** pointer
    (dangling-pointer risk). Confirmed in a probe: after `make_shared<Igc>()`, `shared_from_this()` did *not* share the
    real owner's control block.
  * *Rejected fix: `weak_from_this()`.* This would require every observer to be owned by a `shared_ptr` when it
    registers, and that breaks geocal and framework (§2.4):
    * registration happens mostly in **constructors** and in serialization **`load()`**, where no `shared_ptr` owns the
      object yet;
    * Python-constructed objects (`nb::init`) have no C++ control block at all.
  * *Adopted fix:* keep `this_obj` (the design is deliberately ownership-agnostic) but build it with the **aliasing
    constructor**, `std::shared_ptr<Observer>(std::shared_ptr<void>(static_cast<void*>(this), null_deleter()), this)`.
    A `shared_ptr<void>` doesn't touch `weak_this`. Make the same one-line change in `to_ptr`/`const_pointer`
    (`null_deleter.h`), so no null-deleter pointer anywhere can claim `weak_this`.
  * Probe results with this change (ASan clean in C++; through nanobind with the forked caster):
    * `shared_from_this()` returns the real owner;
    * constructor registration works, for C++- and Python-constructed observers;
    * stack, member, `shared_ptr`-owned and Python-subclass observers are all notified, and dropped when they die;
    * `add_observer_and_keep_reference` keeps a Python observer alive.
* **Multiple inheritance:** nanobind allows one bound base per class (by design). Classes with two bases bind the
  primary one; the secondary base's methods are bound directly on the class (lambdas or member pointers both work,
  tested), and functions taking the secondary base as a parameter need an overload or the conversion helper.
* **Memory errors can pass silently.** In one probe a call through a wrongly adjusted pointer returned the right
  answer by luck. **Run the binding tests under AddressSanitizer** (or valgrind) at each nanobind step.

### 2.2 Decisions carried over from the geocal plan

| Topic | Decision (geocal plan) | Applies to the skeleton as |
|---|---|---|
| Branches | geocal: `develop_2_0`, with `maintain_1.28` for the gcc 11.2 system | Skeleton: work on `modernize`, tag phase boundaries |
| C++ standard | **C++20 now**, minimum **gcc 14**; C++23 later (after SWIG is gone). **C++23 done in S7, minimum still gcc 14** (decision 22) | Same; switch in Phase 1 |
| macOS | Best-effort | Same |
| Boost | Require current conda-forge Boost (≥ 1.84) | Same; the env already has 1.90 |
| Boost → std | filesystem, regex, format, lexical_cast, foreach, noncopyable, scoped_ptr, multi_array, optional, array, shared_ptr | Same, plus the skeleton-only `boost::range`/`boost::assign` in `generic_object_map.h` |
| Keep from Boost | Serialization, Test, math::quaternion, iostreams (gzip), asio/process | Same (the skeleton uses serialization, test, iostreams) |
| Backtrace | Boost.Stacktrace held as a member, plus `std::source_location` | Skeleton `Exception` has no backtrace yet. Add it here first (both consumers have one). |
| Exceptions | Format-string constructor via `std::format`; generic `std::formatter` for `Printable` | Same; prove here |
| Serialization | Stay on Boost.Serialization; keep the repo's own `shared_ptr` save/load (`px`, version 1, `track_never`) | Same |
| `--without-boost-serialization` | Keep while cheap; fix the broken files; add a regular build | Same (4 files here) |
| Generated wrappers | Committed on purpose until nanobind removes them | Same |
| Blitz | Keep; `mdspan` (conda-forge Kokkos reference implementation, pinned snapshot) as the boundary/interop type | Same; add a column-major case for framework |
| GSL, libsdbx, FFTW, HDF5 | geocal-only library swaps | **Not exercised here.** framework uses GSL much more heavily (§2.3). |
| Bindings | SWIG now; nanobind pilot after `std::shared_ptr` | Skeleton **is** the pilot (Phase 6) |
| `GenericObject`/`Printable` | Keep both | Keep both, plus the new jobs in §2.1 |
| Build | geocal autotools; framework CMake | Skeleton: autotools primary plus a small CMake reference build for the nanobind part (for framework) |
| Python-free C++ library | Keep `libskeleton` free of Python; all Python code in the extension module | Same |

### 2.3 framework: where it differs (survey 2026-10-23)

Counts are grep counts over non-generated `.h`/`.cc` (878 files).

| Item | framework | Consequence |
|---|---|---|
| Build | CMake 3.12+, `enable_language(Fortran)`, `CMAKE_CXX_STANDARD 14` | nanobind's own CMake path (`nanobind_add_module`). The skeleton keeps a small CMake reference. The C++14 → C++20 jump is bigger but needed anyway (nanobind needs C++17). |
| Environment | `environment.yml`: `python<3.9`, `swig<4.0`, `defaults` channel | Must move to a current pixi/conda-forge env before any of this. nanobind 3.x needs a current Python. |
| GSL | **563 uses / 39 files**: `gsl_multifit_nlinear` (248, the NLLS solvers), `gsl_multimin`, `gsl_sm` sparse, `gsl_fft` | GSL is part of framework's core algorithms. Its removal is a separate, much larger project; it stays in the shared environment even after geocal drops it. Nothing in the shared rules library may assume GSL is gone. |
| Eigen | Already found by CMake (`FindEigen.cmake`), 9 uses | Consistent with geocal's choice of Eigen to replace GSL. |
| Blitz / autodiff | `ArrayAd<T,D>` 1,038 uses / 218 files, `AutoDerivative` 969, `ColumnMajor`/`fortranArray` 831 | Keep Blitz. The Python array caster and `mdspan` must support **Fortran order** (`layout_left`, tested OK with the reference implementation). |
| Multiple inheritance | 52 SWIG classes with 2+ bases; 98 header classes; **169 virtual bases** (`virtual public StateVectorObserver` ×23, `NLLSProblem` ×13, …; `public virtual GenericObject` ×11) | The "primary bound base + forked caster + conversion helper" design (§2.1) has to scale to this. That is why the virtual-diamond sample and its tests are required here. |
| Directors | 47 `%feature("director")`, incl. 6 `Observer<...>` template instances | Trampolines are mechanical; generate them from the `.i` (Phase 6.3). Add a template-director sample here. |
| Observer | `Observer<` 150 uses; `to_ptr` 5 uses; `observer_ptr` | The `enable_shared_from_this` / `this_obj` interaction (§2.1) matters even more here. |
| Boost | `lexical_cast` 78, `BOOST_FOREACH` 111, `boost::bind`/`function` 39, `date_time` 29, `filesystem` 29, `regex` 20, `optional` 17, `array` 9, iostreams 7, spirit/phoenix/fusion 4, geometry 1; no `math::quaternion`, no asio/process | Same std replacements, plus `bind`/`function` → lambdas/`std::function`. |
| Lua / luabind | 442 uses / 112 files | Leave behind (already decided). Don't convert. |
| `backtrace.hpp` | 1 file (same `Exception` heritage) | Same Boost.Stacktrace change. |
| Generated wrappers | Not committed (generated at build time) | The opposite trade-off to geocal; moot after nanobind. |
| `--without-boost-serialization` | No equivalent CMake option | Autotools-only concern (skeleton, geocal). |

**Does framework change any geocal decision?** No decision needs reversing:
* C++20 with gcc ≥ 14, Boost ≥ 1.84, Boost.Serialization, Blitz, Boost.Stacktrace, nanobind, and `mdspan` all apply as
  they are.
* Things that need care for framework's sake:
  * The shared rules library must ship both an `.am` fragment and a CMake file (as `swig_rules` does today).
  * The array caster and `mdspan` helpers must handle column-major storage.
  * GSL removal stays a geocal-only change.
  * The nanobind design must handle multiple and virtual inheritance at framework's scale. The skeleton tests in
    Phase 6 are sized for that.

Rechecked 2026-11-05: framework `master` has no commits since `0521a2dd` (2026-10-22), before this survey; the only
newer branch is a dependabot `setuptools` bump. Nothing here changes.

### 2.4 Observer ownership in geocal and framework (survey 2026-10-23)

The question: is it OK to require that Observers (or Observables) are owned by a `shared_ptr`? Short answer: **not
needed, and it would break things**. Keep the ownership-agnostic `this_obj` design with the aliasing-constructor change
from §2.1.

How the pattern works today (identical in both repos; framework adds `CacheInvalidatedObserver` with its own
`this_obj_` and a `colist`):
* `Observer<T>` holds `this_obj`, a null-deleter `shared_ptr` to itself, created in its constructor.
* `Observable<T>::olist` stores `weak_ptr`s made from `this_obj`, so an entry expires exactly when the observer object is
  destroyed, however it was owned (stack, member, `shared_ptr`, Python).
* `add_observer` takes `Observer<T>&` (a reference, not a `shared_ptr`).
* `add_observer_and_keep_reference(shared_ptr&)` is the explicit "the Observable owns it" option.
* Observables need no particular ownership either: they only hold `weak_ptr`s.

Registration sites (`add_observer(*this)` / `add_cache_invalidated_observer(*this)`):

| | geocal | framework |
|---|---|---|
| `add_observer(...)` calls, all | 20 in 14 files | 129 in 73 files |
| `add_observer(*this)` (self-registration) | 11 in 8 files | 29 in 14 files (+5 `add_cache_invalidated_observer(*this)`) |
| … of which in a **constructor/init** | `GlasGfmCamera`, `IpiImageGroundConnection` (×2), `OrbitDataImageGroundConnection` | 19 |
| … of which in serialization **`load()`/`serialize()`** | `AirMspiOrbit::load`, `IpiImageGroundConnection::serialize` | 5 |
| … elsewhere (setters such as `orbit(...)`, `tie_point(...)`) | 5 | 10 |
| `add_observer_and_keep_reference` | 2 | 2 |
| `to_ptr` / `const_pointer` / `null_deleter` uses | 2 / 8 / 14 | 5 / 0 / 4 |
| `shared_from_this`, `weak_from_this`, `enable_shared_from_this` | 0 | 0 |

Consequences:
* Requiring `shared_ptr` ownership at registration time (`weak_from_this()`) would silently break every constructor
  and `load()` registration (about 4 + 2 in geocal, 24 in framework), and all Python-constructed observers. Making
  it work would need two-phase construction (factory plus `init()`) for those classes, which is a large API change for
  no benefit.
* With the aliasing `this_obj`, **no ownership requirement is needed** for Observers or Observables, in C++ or from
  Python. Ownership rules stay as they are today: an observer must be kept alive by someone (C++, Python, or
  `add_observer_and_keep_reference`), and is silently dropped when it dies.
* Serialization: framework serializes `olist`/`colist` (`FP_OBSERVER_SERIALIZE`) with the two-pass `weak_ptr` rule.
  The `weak_ptr`s are still made from `this_obj`, so the change to how `this_obj` is built doesn't change what is
  written. Verify with the golden archives (the S5 Observer sample includes this).
* Expected nanobind note: the leak checker reports Python objects kept alive through C++
  (`add_observer_and_keep_reference` with a Python observer). These cycles through C++ are invisible to Python's GC,
  under SWIG today as well. Tests should drop such references explicitly before exit.

### 2.5 geocal changes since the last sync (reviewed 2026-10-30)

geocal (`develop_2_0`, commits `60e4c54c7`..`2193f1953`, 2026-10-23 to 2026-10-27) changed its copy of `swig_rules/`
and its plan. Decision 9 (§13) says geocal copies `swig_rules/` *from* the skeleton, so changes made first in geocal
have to come back here, or the next copy to geocal would undo them. File-by-file diff of geocal's `swig_rules/`
against this repository's:

| File | geocal change | Action here |
|---|---|---|
| `include/swig_array.i` | SWIG ≥ 4.4 uses multi-phase init, so `import_array()` (returns `NULL` on failure, which looks like success) became `import_array1(-1)` under `#if SWIG_VERSION >= 0x040400` | **Take it** (ST.1). The skeleton builds with SWIG 4.5.0, so it has the same latent bug: a numpy import failure would not be reported. |
| `include/swig_vector_shared_ptr.i` | New `%shared_ptr_value_traits(TYPE)`: lets a Python list convert to `std::vector<T>` when `T` is wrapped with `%shared_ptr` but stored by value (e.g. `std::vector<Time>`), and makes iteration return real objects instead of leaked untyped pointers | **Take it**, and add a sample + test (ST.3), since the skeleton has no by-value vector of a shared_ptr-wrapped class. Note for S6: the nanobind equivalent is `nb::bind_vector` / the STL caster; add the sample to the S6.1 item 9 acceptance tests. |
| `swig_rules.am`, new `py.typed`, `__init__.py` comment | Generates `<pkg>_swig/__init__.pyi` (star-imports every module) and installs it with `py.typed` and optional hand-written `SWIG_PYI` stubs, so mypy sees the SWIG classes | **Take it** (ST.1). Check `make mypy` in `python/` still works. For S6.5 this becomes `nanobind.stubgen`; record that the hand-written-stub mechanism (`SWIG_PYI`) is replaced there. |
| `__init__.py` | `if False:` → `if True:` (geocal glob-imports every module into the top package) | **Don't take**: this is a geocal-specific setting. Keep the skeleton's value; note it in `swig_rules/README.md` as the one line each project sets. |
| `doxygen_single.cfg` | `WARNINGS = NO` (single-file runs give spurious warnings); `MACRO_EXPANSION = YES`, `EXPAND_ONLY_PREDEF = YES`, `PREDEFINED = "UNUSED(x)=x"`; regenerated for doxygen 1.13 | **Take it.** `UNUSED` is defined in geocal's `generic_object.h`, not in the skeleton's; harmless here. |
| `doxy2swig.py` | geocal has the newer upstream m7thon version (2025-10-17, `2246a41ef`, "basic type information in the docstrings"); the skeleton still has the 2019 one | **Take it**, and regenerate the committed wrappers (separate commit). Matters for S6.3: the nanobind docstring generator should start from this version. |
| `lib/serialize_shared_ptr.h` | geocal lacks the S0.6 `#ifdef SWIG_HAVE_BOOST_SERIALIZATION` guard | Nothing here; geocal gets it at its next copy (geocal Phase 0 step 4a). |
| `config/m4/ac_msp.m4` | `-I` → `-isystem` for MSP headers | Take it (geocal-only macro, but keeps `config/m4/` identical for copying). |

Other geocal changes that affect this plan:
* **SWIG version pin.** geocal now pins `swig = "4.4.*"` so regenerated wrappers match the committed ones. The
  skeleton's env has unpinned `swig` (4.5.0 today). Pinning here too keeps wrapper diffs meaningful; open question
  (§13).
* **`TypeError` for type check failures** (geocal decision 2026-10-25; geocal plan Phase 6 cleanup item). SWIG maps
  every `std::exception` to `RuntimeError`. In S6.1 item 6 the skeleton must prove the pattern: a library exception
  type (`Skeleton::TypeError : Exception`, *not* `nb::type_error`, so `libskeleton` stays Python-free), and a
  translator registered in the module that maps it to Python `TypeError`. Added to S6.1 item 6 and §12.
* **Warnings cleanup** (geocal 2026-10-25/27): gcc 15 `-Wall` clean apart from Blitz false positives, plus
  `--strict-markers` and a curated `filterwarnings` list in `pytest.ini`. Both belong in S1 here (S1.2) and ST
  (`--strict-markers`). geocal's `dir_change.h` fix (destructor doesn't throw, constructor closes the handle) is
  already done in the skeleton's `serialize_function.cc` `DirChange`, but that class is still not tested (§4a).
* **`RELEASE_NOTES_2.0.md`**: geocal records every user-visible change. The skeleton has no users, but the S6.6
  "Python API changes to record" list should be written in the same form, so geocal can copy entries.
* **mypy** now passes in geocal with `--disallow-untyped-defs`, and `make` generates the `__init__.pyi`. S6.5 must
  keep that working with nanobind stubs.

### 2.6 geocal and framework changes since 2026-10-30 (reviewed 2026-11-05)

geocal (`develop_2_0`, `2193f1953`..`f3d94e93`, all 2026-11-03) did the Resampler speed-up
(`claude_documents/resampler_threading_plan.md`): a threaded `Resampler`, guards in `ThreadPool`, a new
`parallel_row_blocks` helper, and one new open item in the geocal plan (§6.1 there). It did **not** change
`swig_rules/`, `config/m4/` or `env/`; the remaining `swig_rules/` differences are the ST fixes made here and geocal's
`__init__.py` setting (§2.5), so nothing comes back this time. framework: no change (§2.3).

| geocal finding | Action here |
|---|---|
| **`make check` tests the installed library** (geocal plan §6.1, open item 5; **done in ST.11, see progress log**). Conda's compiler activation puts `-Wl,-rpath,$CONDA_PREFIX/lib -Wl,--disable-new-dtags` in `LDFLAGS`. With `--prefix=$CONDA_PREFIX`, the env's old `libgeocal.so` comes before `.libs` in the test program's RPATH (RPATH, so `LD_LIBRARY_PATH` doesn't help). New symbols give `undefined symbol`; changed code silently tests the old version. geocal asks for the fix to be tried in the skeleton first. | **ST step 11.** The skeleton has the same setup (`--prefix=$CONDA_PREFIX`) but doesn't show it in the frenchpeas scratch env: that env's `compilers` 2.0.0 brings no `gxx_linux-64` activation, so `LDFLAGS` is empty and `skeleton_test_all` has `.libs` first in its RPATH (checked 2026-11-05). geocal's env does have the activation. To reproduce, add `gxx_linux-64` (and `gcc_linux-64`) to a scratch env, then pick one of geocal's four options and test it here. Check both the C++ test program and the Python extension (`_swig_wrap.so` loads `libskeleton` the same way). |
| **GIL.** The SWIG wrappers are built without `-threads`, so the GIL is held for every C++ call. geocal's `ThreadPool` guard against two threads using the global pool at once relies on this for Python callers. | S6.1 item 11: keep the GIL held by default under nanobind (no `nb::gil_scoped_release` unless a call is shown to be safe and worth it), and test a director called from a C++ worker thread. |
| **Exceptions from worker threads.** A task that threw used to call `std::terminate`. Now the first `std::exception_ptr` is saved and rethrown from `wait_tasks_finish`, so the original exception reaches the caller (and Python, as `RuntimeError`). | Covered by the same S6.1 item 11 test: the exception, or a Python exception from an override run on a worker thread, arrives in Python with its type and message. |
| **Overload dispatch on the first argument**: `resample_field(blitz::Array<double,2>)`, `(shared_ptr<RasterImage>)` and `(std::string)`. A numpy `float32` array must be accepted for the `double` overload, and a `MemoryRasterImageFloat` must not be taken by the Blitz typecheck. | S6.1 item 7 acceptance test. nanobind resolves overloads in two passes (exact, then with implicit conversion); check that the result is the same as SWIG's. The ST `ArrayTypemapSample` already shows a different dtype is converted (a copy) for a single signature. |
| **Static data members** go through `cvar` in SWIG, so geocal used static `max_step_ratio()` / `set_max_step_ratio()` functions instead. | S6.3: the converter needs `static` functions (`.def_static`) only; no static data members. |
| **doxy2swig and anonymous namespaces**: an anonymous namespace in a `.cc` makes doxy2swig emit a `%feature` line SWIG can't parse; geocal moved the helpers into the class. | S6.3: the docstring generator must skip anonymous-namespace symbols. The skeleton's only anonymous namespaces are in `*_test.cc` files, which doxygen doesn't read, so nothing to fix now. |
| **Serialization version bump**: `Resampler` went to class version 1 with new fields; old pickles load (checked by a small version 0 pickle in the test data). A renamed member kept its serialized name (`GEOCAL_NVP_`). | Nothing new to do: the golden archives are the same check. S5 samples that change version should follow the same pattern. |
| **A `*_test.cc` file never listed in the `.am`** (`resampler_test.cc`, plus two others, so their tests never ran). | Checked here: every `*_test.cc` is listed. |
| **`ThreadPool` uses `boost::asio::thread_pool`**; the geocal plan keeps Asio (a `std`-only replacement is optional). | No change to §2.2 (Asio kept). The skeleton has no thread pool; whether to add a small one as a test sample is an open question (§13). |

---

## 3. Phase overview

The phases match the geocal plan's numbering where they correspond. Each phase ends with a full build, all tests
passing (or explicitly `xfail`ed with a reason), the golden-archive test green, and a tag.

| Skeleton phase | Content | geocal plan phase |
|---|---|---|
| **S0** | Baseline, housekeeping, safety nets | 0 |
| **ST** | Test hardening (added 2026-10-30): coverage measurement, tests for every `swig_rules` feature consumers use, bug fixes found by them, geocal sync (§2.5). **Done before S1.** | 0 (geocal should repeat the coverage measurement) |
| **S1** | C++20 switch, tooling, hygiene, Exception/Printable modernization | 1 |
| **S2** | Boost → std (not in the API) | 2 |
| — | GSL / libsdbx / FFTW (geocal only) | 3 |
| **S3** | `std::optional`, `std::array` (touch SWIG typemaps and serialization) | 4 |
| **S4** | `std::shared_ptr` in C++ and serialization; Python bindings broken until S6 (the state just before S4 is `swig-final`) | 5 |
| **S5** | New samples for consumer patterns (Observer, multiple/virtual inheritance, template director, arrays). **Do all of S5 before S4**: S4 breaks the Python bindings until S6, so SWIG reference results for the new samples' Python tests can only be recorded before S4. Order of work: S0 → ST → S1 → S2 → S3 → S5 → S4 → S6 | (prerequisite for 4 and 6) |
| **S6** | nanobind: rules library, bindings (numpy ↔ Blitz through `mdspan` views), build, stubs/docstrings, SWIG removal | 6 |
| **S7** | C++23 (minimum still gcc 14), `mdspan` for raw pointer-plus-size interfaces (done 2026-11-06); CMake reference expansion later | 7 |

---

## 4. Phase S0: baseline and safety net

1. **Record the baseline.** In `../cxx-python-skeleton-build/build-pixi`: `configure --enable-maintainer-mode
   --enable-debug`, `make -j check`, `make install`, Python tests. Record per-test pass/fail/skip/xfail in this file.
   Confirm whether `test_director_example_object_tracking` fails with `NameError` (`PickleWithSwigSupport`).
2. **Unfinished re-entrant pickling work: set aside (decided).** The `modernize` branch doesn't contain it.
   * Its purpose (R7 in `serialization_options.md`: a C++ object holds a Python object that holds C++ objects, with
     identity preserved across the boundary) is redesigned in S6 for nanobind.
   * Turn the tests that describe the intended behavior into acceptance tests for S6:
     * uncomment the assertions in `test_director_example_object_tracking`;
     * mark it `@pytest.mark.xfail(strict=True, reason="nested C++/Python identity: redesigned in S6")`;
     * replace the `@skip` on `test_director_example_weak_ptr` with an explained `xfail`.

     `strict=True` makes the suite tell you when they start passing.
   * The pickling work stays on `master` for reference.
3. **Golden archives** (`unit_test_data/golden/`), written by the *current* code and committed:
   * XML **and** binary archives for: `GenericObjectA`, `GenericObjectB` (shared `A`), `GenericObjectB2` (by-value `A`),
     `GenericObjectC` (`weak_ptr`), `GenericObjectMap` with shared and weak entries, the C++-only `DirectorExample*`
     objects, and Blitz arrays (ranks 1–5, the 5 element types).
   * A small generator (`lib/golden_archive_generator.cc` built as a `check_PROGRAMS`, or a Python script) so the
     files can be regenerated deliberately, with a README saying when they were made.
   * A C++ test that loads every file and checks values, plus a Python test doing the same through `read_shelve`.
4. **Tooling in `env/linux/pixi.toml`** (and the build-pixi env): `clang-tools` (not `bear`: it doesn't run on this host, see the progress log) (pin builds at least a week
   old), `nanobind` (for later), `eigen` not needed here. `mdspan` (pinned `0.6.1.dev20260223` snapshot) is added at
   the start of S6 (S6.1 item 7); it was not added in S0.
5. **Minimum-compiler build env**: a second scratch pixi env pinned to `gxx_linux-64 14.*` (the 2.0 minimum), used at
   each phase end.
6. **Fix `--without-boost-serialization`** (the 4 files listed in §1) and add a build with it to the phase-end checklist
   (C++ tests only, Python bindings off). This is the reference for the same fix in geocal (12 files).
7. **Housekeeping**: `.gitignore` for `*~`, `python/build/`, `python/*.egg-info/`, `python/pytest_tmp/`,
   `python/test*.xml`; remove committed backup files (`configure~`, `README.git~` if tracked).
8. **Sanitizer target**: a way to run the C++ and Python tests under AddressSanitizer (`--enable-asan` configure option
   or a documented `CXXFLAGS=-fsanitize=address` build plus `LD_PRELOAD=libasan` for Python). Valgrind is already
   supported in `test_all.sh` (`valgrind=1`).

**Exit criteria:** baseline recorded; known-broken tests are explicit `xfail`s; golden archives committed and loading;
`--without-boost-serialization` builds; tag `phase0-baseline`.

---

## 4a. Phase ST: test hardening (added 2026-10-30, before S1)

**Question asked:** once modernizing starts, will the tests tell us if something broke? **Answer: not yet.** The
tests are good at what they were written for (director lifetime, pickling of `GenericObject`s with shared and weak
pointers, numpy lifetime, the golden archives). But most of what `swig_rules` provides, and what geocal and framework
rely on, is never run by any test. Each later phase rewrites exactly those pieces: S1 `Printable`/`Exception`, S2
`serialize_function.cc`, S3 `boost::optional`/`boost::array` typemaps, S6 *all* typemaps. A regression there would
pass the suite today.

### Coverage measured (2026-10-30)

Build: `build-coverage` (recipe below), `make skeleton-check` (C++ 1/1) then pytest (21 passed, 6 xfailed). Line
coverage from `gcovr` (C++) and `pytest-cov` (Python).

| Area | Lines covered | Not covered (main gaps) |
|---|---|---|
| C++ `lib/` + `swig_rules/lib/`, total | **52%** (1050 of 1986) | |
| `lib/skeleton_exception.h` | **0%** | `Exception` is never constructed or thrown in a C++ test; `range_check*` never called. From Python, the only exception test is a Python exception raised in a director. |
| `lib/printable.h` | 30% | Only `printable_test.cc` with one class. `desc()` override path tested only indirectly. |
| `lib/blitz_array_serialize_support.cc` | 20% | Only `double`/`int`/`bool` and `std::string` arrays; `float`, `short`, `unsigned`, `unsigned short`, `char`, `unsigned char` never serialized (and four of them can't be, S0.3 finding b). |
| `swig_rules/lib/serialize_function.cc` | 85% | `serialize_write_binary(file)` never called; `DirChange` error paths; read of a missing file; `skip_weak_ptr_handling`. |
| `swig_rules/lib/serialize_shared_ptr.h` | 91% | The director branch of `our_shared_ptr::reset` (loading an archive containing a Python director object) is hit only from Python. |
| `swig_rules/lib/swig_type_mapper.h` | 49% | Only the classes the Python tests happen to return. |
| Python `skeleton` package | **42%** | `swig_shelve.py` 38%: `.bin` write, `.json`, sqlite `key` path, `shelve_time_after`, `extra_python_init`, `SQLiteShelf` methods. |

`swig_rules/include/*.i` typemaps (no line coverage; checked by reading the tests and the generated `_wrap.cc`
coverage). geocal/framework counts are the number of `.i` files using the feature:

| `swig_rules` feature | Tested here? | geocal `.i` files | framework |
|---|---|---|---|
| `blitz::Array<double,2>` in/out, lifetime | yes (`numpy_sample_test.py`) | many | many |
| Other ranks (1, 3–8) and element types (`int`, `bool`, …) | **no** | `int` 12, `bool` 14 | many |
| `OUTPUT`/`OUTPUT1`/`OUTPUT2` argout typemaps | **no** | 47 | 15 |
| `FORCE_COPY` | **no** | 2 | 6 |
| non-contiguous / Fortran-ordered numpy input, wrong dtype/rank error | **no** | | ColumnMajor 831 C++ uses |
| `std::vector<blitz::Array>` in/out, director in/out of Blitz arrays | **no** | | |
| `boost::array` (`swig_boost_array.i`, `%python_attribute_boost_array`) | **no** (no sample; S3 adds one) | 26 / 3 | |
| `boost::optional<int>`, `<double>` | **no** (only `optional<blitz::Range>` out) | 1 / 1 | |
| `boost::rational` (`swig_rational.i`) | **no** | 4 | 3 |
| `%python_attribute` / `_with_set` | yes | | |
| `%python_attribute2*`, `_nonconst`, `_with_set_virtual` | **no** | 23 / 1 / 7 | |
| `%pickle_serialization` | yes | 161 | |
| `%pickle_init`, `%pickle_vector`, `%pickle_serialization_dir` | **no** | 14 / 1 / 7 | 1 / – / – |
| `Vector_*` (`std::vector<T>`) pickling, `swig_std.i` `vector_double` etc. | **no** | | |
| `%shared_ptr_value_traits` (geocal 2026-10-27, §2.5) | **no** (not here yet) | 2 | |
| iostream in/out | yes | 5 | |
| `__str__` through `print_to_string` | **no assertion** anywhere | all | all |
| C++ `Exception` → Python `RuntimeError` with message | **no** | all | all |
| Unknown non-`std::exception` C++ throw → `RuntimeError` (`catch(...)`) | **no** | | |
| Return `shared_ptr<GenericObject>` → most-derived Python type | partly (via `GenericObjectMap.__getitem__`) | | |
| `SHARED_PTR_NO_OWN` | yes (`test_director_example_weak_ptr`) | 0 | |

### Bugs found by this review (none is caught by a test today)

1. **`write_shelve("x.bin", obj)` always fails** with `TypeError: serialize_write_binary() takes exactly one argument
   (2 given)`. Same root cause as the S0.3 `.bin` read `xfail`: the skeleton's `lib/serialize_function.i` lacks the
   file-based `serialize_write_binary(Fname, Obj)` and `serialize_read_binary_generic(Fname)` that
   `swig_rules/lib/serialize_function.i.sample` has. Fix: bring `serialize_function.i` in line with the sample, then
   drop the 5 `xfail`s.
2. **`write_shelve`/`read_shelve` with `.json` fails** (`RuntimeError: input stream error`). jsonpickle encodes
   `__reduce__`'s `bytearray` argument as an empty `{"py/object": "builtins.bytearray"}`, so the binary blob is lost.
   **Skeleton-only:** geocal, framework and `serialize_function.i.sample` return `bytes`, which jsonpickle round-trips,
   and framework tests `.json` of a C++ object. Same fix as bug 1 (decision 13).
3. **`Printable::print_to_string` returns text prefixed with `"blah"`-overwritten garbage.** `std::ostringstream
   os(buf)` with `buf = "blah"` starts writing at position 0 *over* `"blah"`, so output shorter than 4 characters
   keeps the tail: `str(GenericObjectA(3))` is `'Alah'`, `str(GenericObjectB(a))` is `'Blah'`. Same in
   `Exception` (`what()` of `Exception("x")` is `"xlah"`). **This bug is also in geocal** (`printable.h`,
   `geocal_exception.h`, `msp.h`, `swig_quaternion.i`) **and framework** (`printable.h`, `fp_exception.h`) and
   `swig_rules/lib/printable.h.sample`. It only shows for text shorter than 4 characters, which is why it went
   unnoticed. S1.4/S1.5 already removes this code; ST adds the tests that catch it first.
4. **`GenericObjectB2 : Printable<GenericObjectB>`** (`lib/generic_object_sample.h`), a CRTP typo: should be
   `Printable<GenericObjectB2>`. `print_to_string` casts `this` to the wrong class (undefined behavior); it is not
   exposed to SWIG, so `str()` gives the default repr. Fixing it doesn't change the archive (the serialize function
   registers only `GenericObject` as base); verify with the golden archives.
5. Known from S0.3, still open: four `blitz::Array` element types instantiated but not serializable. Since all 9
   types are kept (decision 16), add the missing `BOOST_SERIALIZATION_SPLIT_FREE` lines in ST and extend the golden
   corpus to them (new files only; existing ones unchanged).

### ST steps

Steps follow the decisions of 2026-10-30 (§13 items 11–17). Each step ends with `make check` green; new tests that expose a bug are first added as `xfail(strict=True)`, then the
fix removes the `xfail` in the same staged change (so the reviewer sees both).

1. **geocal sync** (§2.5): `swig_array.i`, `swig_vector_shared_ptr.i`, `swig_rules.am` + `py.typed` (`.pyi`),
   `doxygen_single.cfg`, `doxy2swig.py`, `ac_msp.m4`. Regenerate wrappers in a separate change.
2. **Coverage tooling, made repeatable**: add `gcovr` and `pytest-cov` to `env/linux/pixi.toml` (already in the
   scratch env); add a `make coverage` target (or a `script/coverage.sh`) that runs the recipe below and writes
   `coverage/` HTML. Not part of `make check`.
3. **New samples and tests for the untested `swig_rules` features** (one small `lib/` sample class with C++ and
   Python tests, mirroring how geocal uses them). These must survive S6 unchanged as the nanobind acceptance tests,
   so test *behavior*, not SWIG details (no `.this`, no proxy type names, except in tests explicitly marked SWIG-only):
   * `ArrayTypemapSample`: Blitz arrays of all 9 element types (decision 16) at ranks 1, 2, 3; `OUTPUT`/`OUTPUT1`/`OUTPUT2`; `FORCE_COPY`; `std::vector<blitz::Array>`; a director
     method taking and returning a Blitz array; numpy input that is non-contiguous, Fortran-ordered, a wrong dtype
     (expect conversion or a clear error, record which) and a wrong rank (error).
   * `OptionalSample`: `boost::optional<int>`, `<double>`, `<blitz::Range>` in and out, engaged and disengaged.
     (S3 converts these to `std::optional`; the tests stay.)
   * `boost::rational<int>` round trip.
   * `%python_attribute2`, `_nonconst`, `_with_set_virtual` (overridden in a Python subclass).
   * `%pickle_init`, `%pickle_vector` (`Vector_*` of a `GenericObject` and of `int`), `%pickle_serialization_dir`.
   * `%shared_ptr_value_traits`: a by-value `std::vector<ValueType>` from a Python list, iteration returns typed
     objects.
   * Downcast: a C++ function returning `shared_ptr<GenericObject>` for each sample class gives the most-derived
     Python type and a method call works.
4. **Printing and exceptions** (gaps for S1):
   * C++: `operator<<` and `print_to_string` agree for each sample class, including outputs shorter than 4
     characters (bug 3); `Exception` streaming, copy, `what()`; `range_check` throws with the expected text.
   * Python: `str()` of every sample class asserted against an exact string (this is the "`str()` unchanged" baseline
     S1.7 compares against); a C++ `Exception` thrown into Python is `RuntimeError` with the exact message; an unknown
     C++ throw becomes `RuntimeError("unknown C++ exception")`; `desc()` override from Python is used by `str()`.
5. **Serialization file entry points** (moved forward from S2, which rewrites this code): C++ tests for
   `serialize_write`/`serialize_read_generic`, binary file write/read, `.gz` on both, reading a missing file raises,
   relative paths read from the archive's directory and the working directory is restored also when the read throws.
   Fix bugs 1 and 2 by bringing `lib/serialize_function.i` in line with `swig_rules/lib/serialize_function.i.sample`
   (file-based binary read/write, `bytes` instead of `bytearray`, accepting both on input), and remove the 5 golden
   `.bin` `xfail`s.
6. **`swig_shelve.py`**: tests for every format (`.xml`, `.xml.gz`, `.bin`, `.bin.gz`, `file.db:key`, `.json`),
   `shelve_time_after`, `extra_python_init.py` / `_extra_python_init`, `SQLiteShelf` (keys, `in`, delete, update
   time). `.json` stays supported (decision 13); its test is the check for bug 2.
7. **Small fixes**:
   * bug 3, the `"blah"` printing bug (decision 12): in `lib/printable.h`, `lib/skeleton_exception.h` and
     `swig_rules/lib/printable.h.sample`, use a plain `std::ostringstream` (no seed string). The tests from step 4
     come first. Note for geocal/framework in §0;
   * bug 4 (CRTP typo);
   * `--strict-markers` in `python/tests/pytest.ini` (as geocal did);
   * pin SWIG `4.5.*` in `env/linux/pixi.toml` (decision 14);
   * `python/python.am` `python-install`: run `$(PYTHON) -m pip uninstall -y <pkg>` (with the same prefix on the
     path; ignore "not installed") before the install, so a switch between normal and editable mode can't leave a
     shadowing copy (decision 17). Verify both directions as in geocal's `python_editable_install_plan.md`.
8. **`multiprocessing.Pool` round trip** of a plain C++ object and a director object (moved forward from §12: it
   passes or fails under SWIG today, and that result is the S6 target).
9. Re-measure coverage and record it in the progress log. Target: no `swig_rules` feature used by geocal or
   framework is untested; C++ line coverage of `lib/` + `swig_rules/lib/` ≥ 80% excluding Blitz explicit
   instantiations that aren't used.
10. **Record the SWIG reference results** (counts and the list of `xfail`s) in the progress log. These, plus the S5
    results, are what S6 has to reproduce.
11. **`make check` tests the library that was just built** (added 2026-11-05, §2.6; geocal plan §6.1). First reproduce
    it: a scratch env with conda's compiler activation (`gxx_linux-64`), `--prefix=$CONDA_PREFIX`, `make install`, then
    a change to a library function that a test checks. `make check` should fail on the change, but passes against the
    old installed `libskeleton`. Then try geocal's options, smallest first: (4) `-Wl,--enable-new-dtags`, (3) filter
    `-Wl,-rpath,$prefix/lib` out of `LDFLAGS` in configure (relying on libtool's install relink), (2) the test driver
    runs through `libtool --mode=execute` / `LD_PRELOAD`, (1) install to a prefix that isn't the env. Check with
    `readelf -d` on the test program and on `_swig_wrap.so`, run the C++ and Python tests before and after
    `make install`, and check that the *installed* programs still find the installed library. The fix goes in the
    shared `config/m4` (or `swig_rules.am`) so geocal can copy it; add a row to "Notes for geocal" in §0.

**Exit:** ST steps done; S0 phase-end checklist green (gcc 15, gcc 14, `--without-boost-serialization`, ASan);
suggested tag `phaseT-tests`.

### Coverage builds

The system `lcov` (1.14) can't read gcc 15 coverage data; use `gcovr` from the pixi env.

    cd /home/smyth/Local/cxx-python-skeleton-build/build-coverage      # uses the build-pixi env
    pixi run --manifest-path ../build-pixi/pixi.toml bash
    ../../cxx-python-skeleton/configure --prefix=$PWD/inst --without-documentation \
        CXXFLAGS="-O0 -g --coverage" LDFLAGS="--coverage"
    make -j 16 && make install PYTHON_EDITABLE_MODE=t
    find . -name '*.gcda' -delete
    make skeleton-check
    source script/setup_skeleton.sh
    (cd ../../cxx-python-skeleton/python && pytest --cov=skeleton --cov-report=term-missing tests)
    gcovr -r ../../cxx-python-skeleton . --filter '.*/cxx-python-skeleton/(lib|cxx_python_rules/lib)/' \
        --exclude '.*_test\.cc' --exclude '.*golden_archive_generator.*' --exclude '.*(global_fixture|test_all).*'

Notes:
* The editable install (`PYTHON_EDITABLE_MODE=t`) is needed so `pytest-cov` measures the files in the source tree.
  **Gotcha:** if a normal `make install` was done first, pip leaves the old copied `site-packages/skeleton/`
  directory behind and it shadows the editable install; delete it (or `pip uninstall skeleton` with the same prefix)
  before switching. The same applies to geocal's pixi env. Fixed in ST step 7 (`python-install` uninstalls first,
  §13 decision 17); geocal should do the same (§0 "Notes for geocal").
* The C++ numbers include the code run from Python (the `.so` is instrumented too), so they are combined C++ +
  Python coverage of the C++ code.

---

## 5. Phase S1: C++20, tooling, hygiene

1. **C++20**: `AX_CXX_COMPILE_STDCXX([20], [noext], [mandatory])` in `config/m4_local/skeleton_common.m4`. Fall back to
   `ext` only if something needs GNU extensions. `config/m4/afids_common.m4` is the shared `autoconf-repo` subtree;
   change it there only in coordination with geocal (or keep the setting project-local, as the skeleton already does).
2. **Warnings**: third-party include paths as `-isystem` (Boost already is via `BOOST_CPPFLAGS`; do the same for Blitz).
   Add `-Wall -Wextra -Wdeprecated` to the debug build. geocal reached a gcc 15 `-Wall`-clean build on 2026-10-27
   (only Blitz false positives left, §2.5); aim for the same here, and add a pytest `filterwarnings` list like
   geocal's (specific entries only).
3. **clang-tidy** with `compile_commands.json` (not from `bear`: it doesn't run on this host; use `script/make_compile_commands.py` and
   `script/run_clang_tidy.sh`, see the S0.4 note). Apply, one check per commit:
   `modernize-use-override`, `modernize-use-nullptr`, `modernize-use-noexcept` (`throw()` in `skeleton_exception.h`),
   `modernize-use-equals-default`, `modernize-make-shared` (configured for `boost::make_shared` until S4),
   `modernize-loop-convert`, `modernize-deprecated-headers`. This is the dry run of the geocal §8 cleanup: record which
   checks needed manual fixes and which SWIG could not parse. Regenerate wrappers in separate commits.
4. **`Printable<T>`** (`lib/printable.h`, plus `swig_rules/lib/printable.h.sample`):
   * `static_cast<const T*>(this)` instead of the C-style casts;
   * remove the "Mac 10.4.11 double free" `buf.reserve(1000)` workaround (also copy-pasted into `skeleton_exception.h`).
     It is not just dead code: it is the source of the `"blah"` output bug (§4a bug 3). If ST already fixed the bug,
     this step only removes what's left;
   * add a generic `std::formatter` for anything derived from `Printable<T>` (calls `print_wrapper`).

   Keep `desc()`, the virtual `GenericObject` base, and the class name.
5. **`Exception`** (`lib/skeleton_exception.h`):
   * store a `std::string` message instead of an `ostringstream`;
   * keep `operator<<` streaming;
   * add a format-string constructor (`Exception("x = {}", x)`), capturing `std::source_location`;
   * add a Boost.Stacktrace member and include it in `what()` or a separate accessor, configurable like geocal's
     backtrace.

   These are the patterns geocal and framework will copy, so get the details right here: the constructor-overload
   trick for `source_location` with variadic arguments, the cost of capturing a stacktrace on every throw, and the
   SWIG-visible interface.
6. **Formatting** (done 2026-11-05, see the progress log): the clang-format trial was **not adopted** (decision 21).
   Instead an AI cleanup made the formatting consistent by hand. geocal and framework do the same; how is in §5a.
   *Originally:* trial `claude_notes/clang_format_trial.clang-format` (tuned to geocal's style; impact on geocal in the
   geocal plan §8.5).
7. **Mechanical tests** (C++): formatter output equals `operator<<` output; the format-string constructor produces the
   expected message and file/line; copying an exception keeps the message; stacktrace is non-empty in a debug build.
   Python: `str()` of each class unchanged (compare with S0 output), exception message text unchanged.

**Exit:** builds with gcc 15 and gcc 14 as C++20; all tests pass; tag `phase1-cxx20`.

---

## 5a. Formatting by AI instead of clang-format (decision 21)

**Superseded (2026-11-05, later the same day):** the skeleton and geocal now use clang-format (decision 21, §5b,
progress log "S1.6 revisit"). The notes below still apply to the one-time hand cleanup done
before that, and to any repository that doesn't adopt clang-format.

**Original decision (maintainer, 2026-11-05):** no clang-format in the skeleton, geocal or framework. The trial (S1.6) gave a
large diff, misparsed C++20 and the serialization chains, and changed layouts nobody minds. Instead, when a repository
is modernized, an AI session makes the formatting consistent: one cleanup, then keeping to the style while editing.
This is what was done here; the notes below are for doing the same in geocal and framework.

**When.** One cleanup per repository, as its own commit (add it to `.git-blame-ignore-revs`), after the clang-tidy passes
and before the big mechanical edits (`std::shared_ptr`), so those diffs aren't mixed with whitespace. Regenerated
wrappers in a second commit.

**How.**
* *Measure first.* Count each form and its variants over the files (as in S1.6: tabs, trailing whitespace, `if (` vs
  `if(`, `Type & x` vs `Type& x`, `template <`, `catch (`, space before `)`, function `{` placement), and look at where
  the minority forms are before deciding anything.
* *Majority rule.* Keep the form the repository already uses most. Make no new style decisions; only remove
  inconsistencies. Where a repository's majority differs from the skeleton's, decide explicitly (known case:
  `Archive & ar`, geocal 583 : 33, skeleton now `Archive& ar`).
* *Write the changes as a script*, not free-hand edits, so the rules are visible, reviewable and can be rerun:
  `claude_notes/style_cleanup.py` is the skeleton's (file list, exclusions, regex rules, a few hand edits with
  `assert`ed match counts). Adapt it per repository.
* *Tabs:* expand with 8-column stops (the code was written in Emacs), which keeps the visual alignment. Check first that
  no string literal contains a tab.
* *Review with `git diff -b`* (hides tab and trailing-space changes but shows `if (` → `if(` and the like) and
  `git diff -w` (only non-whitespace changes); summarize with `sort | uniq -c` over the changed lines to spot anything
  unintended.

**Don't touch.**
* Upstream forks and copies, so they stay diffable: here `my_shared_ptr.i` (SWIG), `serialize_shared_ptr.h` (Boost). In
  geocal also `thirdparty/` and vendored code (the old `boost/backtrace.*` goes anyway); in framework `thirdparty/`.
* Code written in its upstream's style on purpose: whitespace cleanup only (here `python_lib_init.h`, Python C-API docs
  style; `swig_vector_shared_ptr.i`, SWIG traits style).
* Generated code: regenerate it, don't edit it (SWIG wrappers; geocal commits them, framework generates them at build
  time).
* Forms that are already consistent or evenly split, and SWIG idioms: `> >` (skeleton: used everywhere), C-style cast
  spacing (both common), `< TYPE >` spacing inside SWIG macros.
* Layout that a tool would "fix" but nobody minds: function signature line breaks, `{ body }` on the line after a short
  signature, the `ar & X & Y` serialization chains (clang-format turned these into `ar& X`).
* Python: `ruff format` already covers it.

**Check.** Full build and tests in every configuration (S0 phase-end checklist), 0 warnings (count `make` *and* the test
programs), regenerated wrappers differ only in copied `.i` code, generated `.py` docstrings unchanged.

**geocal and framework differences.**
* Size: geocal `lib/` is 652 files / 106k lines (8.5k lines with tabs, 5.9k with trailing whitespace, geocal plan §8.5);
  framework 878 C++ files. Do it per directory, building after each, so a review is manageable.
* geocal commits its wrappers, so its `.i` cleanup gives a large wrapper diff (second commit). framework doesn't.
* framework has Fortran and Lua (`luabind`, being dropped); only the C++ and `.i` files are in scope.

**Afterwards.** New and edited code follows the same rules (an AI session reads them from the plan or CLAUDE.md). Rerun
the measurement counts now and then; a non-zero count means something drifted.

### 5b. How the clang-format configuration was found (2026-11-05)

A record, in case it ever has to be redone (for example after a clang-format major version changes the output, or
for another repository). The inputs and output are kept in `claude_notes/clang_format_infer/`.

1. **Hand-tuned starting point.** `claude_notes/clang_format_trial.clang-format` (2026-10-23): LLVM base, tuned by hand to
   geocal's Emacs/GNU-ish style (2-space indent, 80 columns, `if(`, `X& y`, `template<class T>`, function braces on their own
   line, `public:` at column 0, no namespace indentation, includes and comments left alone). Measured on geocal in the
   geocal plan §8.5.
2. **Trial fixes** (S1.6 trial, on the skeleton): `Standard: c++03` → `c++20` (C++03 mode misparsed `requires`/`consteval`),
   plus `EmptyLineBeforeAccessModifier: Leave`, `FixNamespaceComments: false`, `ConstructorInitializerIndentWidth: 2` to
   match the existing code. Result: `clang_format_infer/start.clang-format`.
3. **Search** with [`clang-format-auto-infer`](https://github.com/ammen99/clang-format-auto-infer) (commit `dd4a6ee`,
   2025-06-20): a genetic search over clang-format options for the configuration that changes the existing code least
   (fitness: insertions + deletions from `git diff --shortstat` after `clang-format -i`). Option list: the tool's
   `data/clang-format-values.json` (clang-format 20 options; fine with 22).
   * Setup: clone it into an empty scratch directory, read what it runs first. Python deps `PyYAML` and `nevergrad` in a
     scratch venv. `clang-format` 22.1.8 from the pixi env on `PATH`. Set `TMPDIR` to scratch space (it copies the repo
     into `tempfile` directories, one per job).
   * **Never point it at a working tree:** each evaluation runs `clang-format -i` on every tracked C/C++ file and then
     `git restore .`. Give it a throwaway git repository holding a copy of only the files to be formatted:
     ```sh
     mkdir repo && cd <src> && git ls-files 'lib/*.h' 'lib/*.cc' 'swig_rules/lib/*.h' 'swig_rules/lib/*.cc' \
         | grep -v <excluded files> | xargs tar -cf - | tar -xf - -C <repo>
     cd <repo> && git init -q && git add -A && git commit -qm base
     ```
     Excluded: generated wrappers, vendored and upstream-copied code (as in `.clang-format-ignore`).
   * Forced options, for correctness, not looks (`clang_format_infer/forced.yml`): `Standard: c++20`,
     `SortIncludes: Never` (reordering can change meaning), `QualifierAlignment: Leave`, `UseTab: Never`,
     `DisableFormat: false`, `Language: Cpp`.
   * Command (run inside `pixi run` so clang-format 22 is found):
     ```sh
     TMPDIR=<scratch>/tmp python main.py <repo> --start-config-file start.clang-format \
         --option-values-json data/clang-format-values.json --forced-options-yaml forced.yml \
         --iterations 40 --population-size 32 --islands 4 --jobs 12 --output optimizer_output.clang-format
     ```
     About 4,000 evaluations; on the skeleton each takes well under a second.
4. **Result.** Skeleton: 1909 → 1727 changed lines; all 4 islands converged on the same configuration, which changed 5
   options (`clang_format_infer/optimizer_output.clang-format`). geocal (652 `lib/` files): the same search, started from
   the skeleton's optimum (`--iterations 20`), found nothing better (73,220), so **one configuration for both**.
   `.clang-format` is exactly the optimizer output plus a comment header; nothing was changed by hand afterwards.
5. **Measuring a configuration** without the tool: apply it to a copy and look at the diff.
   ```sh
   clang-format -style=file:<config> -i $(git ls-files ...)
   git diff --shortstat; git diff -w --shortstat    # all changes / ignoring whitespace
   git diff -b -U0                                  # changes other than tab and trailing-space cleanup
   ```
   Classifying the removed lines (`git diff -b -U0 | grep '^-'`, then by shape) showed which changes no option can
   avoid: the `//---` doxygen blocks inside a class get indented (`CommentPragmas` doesn't help), `/****//**` banners
   become `/****/ /**`, `{ body }` after a signature is expanded, `ar & X` becomes `ar& X`, and geocal's aligned
   parameter lists are repacked.
6. **Checks before adopting.** Full build and tests; and **force SWIG to regenerate every wrapper** (touch the `.i` files;
   header changes alone don't trigger it) to compare the docstrings doxygen extracts. Skeleton: byte-identical; geocal:
   one docstring changed, for the better.
7. **Redoing it.** Repeat steps 3–6 with `optimizer_output.clang-format` as the start, on whatever the code is then.
   Expect a small diff (the code is already formatted); check `Standard` still parses the newest C++ in the code.

---

## 6. Phase S2: Boost → std, not in the API

| Change | Where | Notes |
|---|---|---|
| `boost::range` adaptors, `boost::assign` → loop / `std::views::keys` | `generic_object_map.h` | `boost::assign` appears unused. |
| `boost::regex` → `std::regex` | `swig_rules/lib/serialize_function.cc` | Shared code: also fixes geocal/framework when pulled. |
| `boost::filesystem` → `std::filesystem` | `serialize_function.cc` (`DirChange`-style logic) | |
| `BOOST_FOREACH`, `noncopyable`, `scoped_ptr` | where present | |
| Keep `boost::iostreams` gzip | `serialize_function.cc` | Kept by decision. |

Tests: the `serialize_function` file entry point tests (XML, binary, `.gz`, working-directory restore on a throw)
were moved forward to ST step 5, so they exist and pass before this phase. Run them before and after the
regex/filesystem change.

**Exit:** compiled Boost libraries needed by `libskeleton` reduced to serialization, iostreams (and test);
golden archives load; tag `phase2-boost-std`.

---

## 7. Phase S3: `std::optional`, `std::array`

1. **`boost::optional` → `std::optional`**: `NumpySample::maybe_range`; `swig_rules/include/swig_boost_optional.i` gets a
   `std::optional` version. Serialization: write an `std::optional` save/load with the **same archive layout** as
   Boost's `boost::optional` serializer (only if Boost 1.90 doesn't already provide a compatible one; check first).
2. **`boost::array` → `std::array`**: the skeleton has no sample. **Add one** (`ArraySample` with a
   `std::array<double,3>` member and a `std::array<std::array<double,3>,2>`, serialized, exposed with a property), so
   `swig_boost_array.i` → `std::array` typemaps are proven before geocal (701 uses).
3. Golden-archive check: an archive written by the old `boost::array` code (add it to the S0 corpus *before* converting)
   must load into `std::array`.

Tests: Python round trip of the array property (list/tuple/numpy in, value out); pickle; XML compare.

**Exit:** tag `phase3-std-types`.

---

## 8. Phase S4: `std::shared_ptr` (C++ and serialization; Python bindings broken until S6)

**Decision (2026-10-23): the Python API/bindings may be broken from S4 until S6.** S4 does **not** port the SWIG layer
(`my_shared_ptr.i`, `shared_ptr_type_mapper.i`, `swig_type_mapper*.h`, director handling) to `std::shared_ptr`, because
all of it is deleted in S6. During S4–S5(rest)–S6:
* build with `--without-swig` (and don't build or run the Python bindings and pytest suite); `make skeleton-check` runs
  the C++ tests;
* acceptance is the C++ tests, the `--without-boost-serialization` build, and the golden archives;
* the last fully working SWIG state (end of S3 plus the S5 Observer sample) is the comparison point for S6. The
  maintainer can tag it `swig-final` after committing. Record its Python test results in the progress log (§0) before
  starting S4, because those results are the S6 acceptance target.

The SWIG items in step 2 below are therefore **skipped**; only the C++/serialization files change.

1. **Migration script** (`script/boost_to_std_ptr.sh` or a Python script): rewrites `boost::shared_ptr`, `weak_ptr`,
   `make_shared`, `dynamic_pointer_cast`, `static_pointer_cast`, `enable_shared_from_this`, and the includes. Develop it
   here; reuse it unchanged for geocal (2,917 uses) and framework (~3,700).
2. **`swig_rules` changes** (C++ side only):
   * `serialize_shared_ptr.h`: keep the repo's own save/load (`px`, version 1, `track_never`); only the pointer type
     changes, built on `shared_ptr_helper<std::shared_ptr>`. Its director hook (`is_python_director`, via
     `swig_type_mapper_base`) is part of the SWIG layer: keep it compiling (stub it if needed) and redo it in S6.
   * `weak_ptr_serialize_support.h/.cc`.
   * **Skipped (deleted in S6):** `my_shared_ptr.i`, `shared_ptr_type_mapper.i`, `swig_type_mapper.h`, `DirectorNotes.md`.
     `swig_type_mapper_base.h/.cc` is compiled into `libskeleton`, so convert it mechanically to keep the C++ build green.
3. **No `boost::shared_ptr` support is kept (decided).** The rules in this repository switch to `std::shared_ptr`
   outright; no parameterization on the pointer type.
4. **Observer self pointer**: change `to_ptr`/`const_pointer` and `Observer::this_obj` to the aliasing-constructor form
   (§2.1, §2.4) as part of this phase. It is harmless without `enable_shared_from_this` and makes S6 a no-op for it.
   **Prerequisite:** the S5 `ObserverExample` sample and its tests exist and pass first, so this change is tested.
5. **Archive neutrality is the key acceptance test.** All golden archives load. New archives written by the
   `std::shared_ptr` build are **byte-identical** (XML text) to the S0 ones for the same objects. Any difference must be
   understood and recorded.

Tests: the C++ suite (including the weak_ptr serialization tests and the S5 Observer sample) and the golden archives.
The Python suite (director lifetime with exact `__del__` counts, pickle/shelve) is **not run** in S4–S6; it comes
back in S6 under nanobind, with the recorded `swig-final` results as the target.

**Exit:** C++ green, archives byte-identical; suggested tag `phase4-std-shared-ptr`.

---

## 9. Phase S5: new samples for consumer patterns

**Done before S4** (S4 breaks the Python bindings until S6). Add these to `lib/` with C++ tests and SWIG Python tests,
so behavior is pinned down before the pointer and binding changes. The Python tests must pass under SWIG now and
under nanobind in S6. Record their SWIG results in the progress log (§0). Each sample mirrors a real geocal/framework
class.

| Sample | Mirrors | What it must cover |
|---|---|---|
| `ObserverExample`: `Observable<T>` / `Observer<T>` copied from geocal's `observer.h` (including `this_obj`, `add_observer`, `add_observer_and_keep_reference`, notification), framework's `CacheInvalidatedObserver`, an observer that **registers in its constructor** and one that registers in serialization **`load()`**, plus framework-style serialization of the observer lists | geocal `Camera`/`Orbit` → IGC notifications; framework `StateVector` | Notification reaches stack, member, `shared_ptr`-owned, Python-constructed and Python-subclass observers; observers that died are dropped; constructor and `load()` registration work; `shared_from_this()` returns the real owner; serialization keeps observers only if they are also in the archive (two-pass `weak_ptr` rule) |
| `MultiBaseExample`: `class CameraLike : public Printable<CameraLike>, public Observable<CameraLike>, public WithParameterLike` with a derived class | geocal `Camera` (8 such SWIG classes), framework (52) | Python sees methods of all bases; functions taking either base accept the derived object; `isinstance` against the primary base |
| `VirtualDiamondExample`: `WithParameterLike : virtual GenericObject`, `NestedLike : virtual WithParameterLike`, `RootLike : virtual GenericObject`, `Diamond : virtual RootLike, virtual NestedLike` | geocal `IgcRollingShutter`, `IgcArray`; framework `virtual public StateVectorObserver` | Returns as `shared_ptr` of each base downcast correctly; calls through each base work; serialization round trip through `shared_ptr<GenericObject>` |
| `TemplateDirectorExample`: a director on `Observer<Something>` | framework (6 Observer directors) | Python subclass of a template instance overrides `notify_update`; lifetime as for `DirectorExample` |
| `ArraySample` (from S3) plus a **column-major** Blitz array member and a strided view | framework Fortran interop | numpy in/out for C- and Fortran-ordered and non-contiguous arrays; zero-copy where promised |
| `ExceptionSample` (from S1) | geocal/framework `Exception` | Message, format constructor, stacktrace available from Python |

Add golden archives for the new serializable samples (S0 step 3 style).

**Exit:** samples merged, tests green under SWIG; tag `phase5-samples`.

---

## 10. Phase S6: nanobind

Work in small steps on `modernize`. The SWIG bindings are already broken (since S4); the reference is the
`swig-final` state and its recorded Python results (progress log). SWIG files are removed in 6.6.

### 6.1 Design spike (turn the `/tmp/nbtest` probes into real tests)

Decide and test each of these, in this order:
1. **Forked `shared_ptr` caster** (§2.1) in the rules library, with `GenericObject : std::enable_shared_from_this<GenericObject>`.
   Verify there is **no** build where a virtual base is declared as a nanobind base.
2. **Type-mapper registry** (`typeid` → cast to the registered most-derived type) and its reverse (Python type → upcast
   to `GenericObject` or to a secondary base), used for `shared_ptr<GenericObject>` returns/parameters and for
   secondary-base parameters.
3. **Observer self pointer** (already changed in S4): test that `shared_from_this()` never returns a non-owning pointer,
   and that constructor-registered observers created from Python are notified (§2.4). No ownership requirement on
   Observers or Observables.
4. **`weak_ptr` semantics policy**: a `weak_ptr` to an object owned only by Python expires (no C++ owner).
   * *Mechanism:* passing a Python-constructed object to a `shared_ptr<T>` parameter gives a temporary `shared_ptr`
     whose deleter only releases a Python reference. If C++ keeps just a `weak_ptr`, that temporary is the only strong
     owner, so the `weak_ptr` expires when the call returns, while the Python object is still alive.
   * *Not a crash:* `lock()` returns null; the failure is a silently wrong answer.
   * *Where it occurs:* only plain `weak_ptr` members set from parameters. geocal has none; framework has only its copy
     of `DirectorExample`; the skeleton has `GenericObjectC` and `DirectorExampleWeakPtr` (test samples). Observer
     lists are not affected: they use `this_obj` (§2.4).
   * *SWIG comparison:* SWIG keeps a real `shared_ptr` in the wrapper even for Python-constructed plain classes, so
     plain classes work there today. Python subclasses (directors) have this same issue under SWIG; that is what
     `SHARED_PTR_NO_OWN` and the skipped `test_director_example_weak_ptr` are about.
   * *Plan:*
     * For plain classes, bind constructors with `nb::new_` returning `std::make_shared<T>(...)`, generated by the
       converter for `GenericObject` classes, so Python-constructed objects carry a real control block (to verify; also
       check whether `nb::new_` works with trampolines).
     * For Python subclasses, document "a `weak_ptr` set from Python is only valid while some C++ `shared_ptr` also owns
       the object" (no worse than SWIG today).
     * Low priority: no production code depends on it.
   Write the expected-result matrix as a parametrized test (creator: C++ / Python-constructed / Python subclass ×
   owner: C++ strong ref exists / Python only).
5. **Pickling**:
   * ordinary classes use `__reduce__` returning `(skeleton._new_from_serialization, (serialize_write_binary(self),))`,
     the same design as today's `%pickle_serialization`;
   * Python subclasses of C++ classes (directors) need the C++ part and the Python `__dict__`/class. Redesign R7/R8 from
     `serialization_options.md` for nanobind (the S0 `xfail` tests are the acceptance tests);
   * check pickle memo identity (same C++ object → same Python object → pickled once), which nanobind's instance map
     should give for free.
6. **Exceptions**: a translator for the C++ `Exception` hierarchy (`nb::register_exception_translator`) and for Python
   exceptions raised in overrides (works already, keep the tests).
   * Add `Skeleton::TypeError : Exception` (a library type, not `nb::type_error`, so `libskeleton` stays
     Python-free) and map it to Python `TypeError`. This is the pattern for geocal's deferred Phase 6 item
     (`IgcMspImp::ground_coordinate_dem`, §2.5). Check what nanobind's default translations do with `std::`
     exceptions in the pinned version (geocal's plan believes none maps to `TypeError`; `std::invalid_argument` →
     `ValueError`).
   * Keep the catch-all for non-`std::exception` throws (today's `catch(...)` in `swig_exception.i`); the ST test
     for it is the acceptance test.
7. **numpy ↔ Blitz through `mdspan`** (matches the geocal plan §3.3.1, which adds `mdspan` together with the nanobind
   pilot):
   * Add the conda-forge `mdspan` package, **pinned to the `0.6.1.dev20260223` snapshot** (not `0.6.0`, which
     hard-codes `std::experimental`), to `env/linux/pixi.toml` and both scratch envs.
   * Add the namespace-alias header (S6.2), so a later switch to `std::mdspan` is one line.
   * Build the bridge in two small layers:
     * `nb::ndarray` (pointer, shape, strides) ↔ `mdspan` view (`layout_right`, `layout_left` for Fortran order,
       `layout_stride` for non-contiguous);
     * `mdspan` view ↔ `blitz::Array<T,N>` sharing the same memory (Blitz's `extent()`/`stride()`/`data()`; for
       numpy → Blitz, a Blitz array over external memory that keeps the Python owner alive).
   * Ownership: returning a Blitz array to Python gives a view that keeps the C++ owner alive, or a copy; decide
     per case.
   * Acceptance tests: the existing three lifetime tests in `numpy_sample_test.py`; the S5 `ArraySample` column-major
     and strided cases; read-only returns; and a clear error for a wrong dtype or rank. Run them under ASan.
   * C++ tests for the `mdspan` ↔ Blitz layer on its own (no Python): contiguous, strided slice, column-major.
     These are the cases already shown to work in a probe on 2026-10-22.
   * Overload dispatch (geocal `Resampler::resample_field`, §2.6): one function overloaded on a Blitz
     `double` array, a `shared_ptr` to a `GenericObject` class, and a `std::string`. A numpy `float64` array, a
     `float32` array, a sample object and a `str` each pick the right overload, and a `GenericObject` class with a
     float array inside isn't taken by the array overload. Record the SWIG result first (ST or S5) and match it.
8. **iostream**: Python file object ↔ `std::istream`/`std::ostream` via a small `streambuf` (no nanobind equivalent).
   The existing `iostream_swig_test.py` is the acceptance test.
9. **STL containers**: SWIG's `%template(Vector_DirectorExample)` gives a reference-semantics proxy type that the
   tests use. nanobind's `stl/vector.h` converts to a Python `list` (copy). Use `nb::bind_vector` where the proxy type
   and its lifetime behavior matter. Decide per type and record it as a Python API change if a type disappears.
10. **Cross-module types**: build a second tiny extension module (`_skeleton_downstream`) in this repo that uses and
    returns types from the main module, to prove the geocal → ECOSTRESS and framework → instrument-repo pattern
    (nanobind version pinned across modules).
11. **GIL and C++ worker threads** (added 2026-11-05, §2.6). SWIG holds the GIL for every call (no `-threads`), and
    geocal's `ThreadPool` relies on that. Default: nanobind bindings hold the GIL too; releasing it
    (`nb::call_guard<nb::gil_scoped_release>`) is a per-function decision for later. Test with a small sample that
    runs a callback on a `std::thread` (or a few) and waits for it, the way `ThreadPool` does:
    * a director (Python override) called from the worker thread works: the trampoline must take the GIL, and the
      waiting thread must not hold it while it waits, or it deadlocks. Record what SWIG does today (it likely
      crashes or deadlocks, since it has no thread support built in), so the result is known, not assumed;
    * a C++ exception, and a Python exception raised in the override, thrown on the worker and rethrown by the
      waiting thread (through `std::exception_ptr`, as geocal's `ThreadPool` now does) arrives in Python with its type
      and message.
    Whether this sample lives in the skeleton is open (§13); without it, these tests move to geocal's nanobind pilot.

### 6.2 Rules library layout

Keep the directory name `swig_rules/` (historical, documented). Add `swig_rules/nanobind/` with:
* `gc_shared_ptr.h` (forked caster, with a header comment giving the nanobind version it was forked from and the diff);
* `type_mapper.h/.cc`, `python_exception.h` (translator), `ndarray_blitz.h` (the `nb::ndarray` ↔ `mdspan` ↔ Blitz
  bridge), `iostream.h`;
* the `mdspan` namespace-alias header (for example `skeleton_mdspan.h` in `lib/`, mirroring the planned
  `geocal_mdspan.h`). This one is **not** Python-specific, so it belongs to the C++ library side, not the extension
  module;
* `pickle.h` (`__reduce__` helpers), `printable.h` (`__str__` from `print_to_string`, `std::formatter` reuse);
* `nanobind.am` (automake rule compiling `nb_combined.cpp` once into the extension) and `CMakeLists.txt`
  (for framework).

Everything Python-specific lives in the extension module; `libskeleton` stays Python-free (as today).

### 6.3 Bindings for the skeleton classes

* Write a **`.i` → nanobind converter** (`swig_rules/nanobind/i_to_nanobind.py`). It reads each `.i` file and emits a
  `*_nb.cc` registration function:
  * class and bases → `nb::class_` (primary base only, with a comment listing the others);
  * constructors → `nb::init`;
  * methods → `.def`;
  * `%python_attribute` → `.def_prop_ro`/`_rw`;
  * `%pickle_serialization` → `__reduce__`;
  * `%feature("director")` → trampoline class;
  * `%template(Vector_X)` → `bind_vector`;
  * anything in `%extend` / `%pythoncode` → flagged for hand conversion;
  * static functions → `.def_static`. Static data members aren't needed: geocal avoids them because SWIG exposes them
    through `cvar` (§2.6).

  The `.i` files are the specification of the Python API, so this keeps it stable. Run it on the skeleton's 9 `.i`
  files and hand-fix the output. The goal is that geocal's 230 and framework's 324 `.i` files convert with only
  modest hand work, so measure how much hand work the skeleton's output needed.
* Module layout: one extension module (`skeleton._skeleton` or keep the `skeleton_swig` name for compatibility; decide)
  with one `init_<file>(nb::module_&)` per header group.
* Docstrings: generate a `docstrings.h` from the existing doxygen XML (a `doxy2nanobind.py` adapted from
  `doxy2swig.py`, or `pybind11_mkdoc`, which emits plain strings usable by nanobind; evaluate both on the skeleton).
  It must skip symbols in anonymous namespaces: doxy2swig emits an unparsable `%feature` line for them today, and
  geocal works around it by moving helpers into the class (§2.6).

### 6.4 Build

* Autotools (primary): `swig_rules/nanobind/nanobind.am` plus a configure macro that finds `nanobind.include_dir()` /
  `source_dir()` via the configured Python (verified to work, §2.1). Flags: `-std=c++20 -fPIC -fvisibility=hidden`, no
  `-lpython`.
* CMake reference: a small top-level `CMakeLists.txt` that builds `libskeleton` and the module with
  `nanobind_add_module`, enough for framework to copy. Not a replacement for autotools here.
* `make check` runs C++ tests, then Python tests against the build tree (no install needed if possible: put the
  module and `python/skeleton` on `PYTHONPATH`, as `test.sh` does).

### 6.5 Stubs, typing, docs

* `python -m nanobind.stubgen` as a `make` rule; commit or generate `.pyi`.
* Run `mypy` on `python/skeleton` with the stubs; `ruff` as now.
* Test: `help()` on a class shows the doxygen text.

### 6.6 Remove SWIG

Once all tests pass under nanobind (including the S0 `xfail`s that were meant to be fixed, the S5 samples, ASan clean):
* remove the `.i` files, `bindings/python/swig/`, the SWIG/doxygen rules, `create_python_init.py`,
  `swig_wrap.tmpl`, `my_shared_ptr.i` and friends;
* update `CLAUDE.md`, `README.md`, `swig_rules/README.md`, `DirectorNotes.md` (rewrite for nanobind), and record the
  naming note.

Tag `nanobind-1`.

**Python API changes (done in S6; for geocal's `RELEASE_NOTES_2.0.md`)**:
* Module layout: one extension module `skeleton_swig._swig_wrap` (name kept, decision 2), everything at the top of
  `skeleton_swig` (and re-exported by `skeleton`); the per-file modules `skeleton_swig.<file>` (e.g.
  `skeleton_swig.serialize_function`) are gone. Installing over a SWIG install leaves the old `skeleton_swig/*.py` and
  `__init__.pyi` behind (harmless at run time, but the stale `.pyi` misleads mypy): remove the directory first.
* No `.this` attribute, no `_v_<name>` methods except for the `%python_attribute_with_set_virtual` properties (python
  overrides them, as before).
* Identity: the same C++ object returned twice is the same python object, also through another base.
* `isinstance(obj, SecondaryBase)` is `False` (one bound base per class; the secondary base's methods are bound on the
  class, and the object can be passed for it). `isinstance(x, GenericObject)` is `False` too.
* `weak_ptr`: an object of a *director* class (or a python subclass) owned only by python has no C++ owner, so a
  `weak_ptr` to it expires at once (SWIG's `SHARED_PTR_NO_OWN` kept it). Plain classes are unaffected.
* `Vector_*` types: kept (`nb::bind_vector`); vectors without a `%template` are python lists (as before).
* `print_to_string` kept as a method, and `__str__`.
* Exceptions: still `RuntimeError` for C++ exceptions, now `TypeError` for `Skeleton::TypeError` and for a python
  override returning the wrong type; a python exception raised in an override comes back unchanged (as before); the
  C++ side's `what()` of such an exception is the python traceback text (SWIG's started "Python error occured:").
  New attributes `cpp_exception`, `cpp_source` for library exceptions.
* `boost::array` classes (`Array_double_3`, ... from `swig_boost_array.i`) are gone; `std::array` comes back as a numpy
  array. `rational_int` and `Range` can now be constructed from python.
* Returned Blitz arrays are read-only views unless the C++ returns a non-const reference (as before); a non-const
  `blitz::Array&` argument now works.
* Pickles made with the SWIG bindings can't be loaded (they name `skeleton_swig.<file>._new_from_serialization`);
  boost archives and shelves (`.xml`, `.bin`, `.db`, `.json`) still load, including python subclass objects.
* Not needed any more: `SerializeSessionLock`/pickler classes (were never on this branch).

---

## 11. Phase S7: C++23, raw pointer plus size → `mdspan`

Started 2026-11-06 at the maintainer's request (decision 22): C++23 with gcc 14 as the minimum, a worked `mdspan`
example based on what geocal needs, and the bootstrap warning if simple. CMake is set aside until framework.

* **C++23** (S7.1): `-std=c++23` (configure, nanobind's own compile in `nanobind.am`, `CMAKE_CXX_STANDARD`). Only
  language features and library features that gcc 14 has. Not available with gcc 14: range formatting (gcc 15),
  `std::mdspan` (gcc 16; gcc 15.3 has no `<mdspan>`, checked). Kokkos mdspan turns on `m[i, j]` in C++23 mode and
  turns `m(i, j)` off, so the code uses `m[i, j]` (as `std::mdspan` will). Not done (would change every class in
  geocal): deducing `this` for `Printable<T>`; `std::expected`.
* **`mdspan` for raw pointer plus size** (S7.2): `lib/raster_sample.{h,cc}`, modeled on geocal's
  `RasterImage::read_ptr(int Lstart, int Sstart, int Number_line, int Number_sample, int* Res)` (overridden in about
  14 geocal classes; not wrapped by SWIG, so a C++-only change there) and its `CalcRaster` (a director: python
  subclasses fill `self.data` in `calc()`). See the progress row for what it covers.
* **Later: deducing `this` in `Printable`** (maintainer, 2026-11-06). When geocal is ready for it, change `Printable<T>`
  to use deducing `this` (C++23, gcc 14 has it) instead of the CRTP parameter. Do it **in this repository first**
  (`lib/printable.h` and `swig_rules/lib/printable.h.sample`, the `PrintableObject` concept and the `std::formatter`,
  the bindings' `printable()` helper, all the sample classes) and test it here before geocal changes every class's
  base. Not part of S7.
* Still later: expand the CMake reference if framework wants more from it; `std::mdspan` when gcc 16 is the
  minimum (check `std::submdspan`, C++26, which `read_with_pad` uses).

---

## 12. Test additions: summary checklist

**Phase ST (§4a; before S1)**
* [x] Coverage tooling (`gcovr`, `pytest-cov`) in `env/linux/pixi.toml` and a `make coverage` target
* [x] Every `swig_rules` typemap/macro used by geocal or framework has a Python test (table in §4a)
* [x] Exact `str()` for every sample class; C++ `Exception` → `RuntimeError` with exact message; unknown throw
* [x] `operator<<` == `print_to_string`, short outputs (the `"blah"` bug); `Exception`/`range_check` C++ tests
* [x] `serialize_function` file entry points: XML, binary, `.gz`; missing file; working-directory restore on exception
* [x] `swig_shelve` every format, `shelve_time_after`, `extra_python_init`, `SQLiteShelf`
* [x] `multiprocessing.Pool` round trip (plain and director object)
* [x] `%shared_ptr_value_traits` vector sample (from geocal)
* [x] `make check` fails on a just-changed library after `make install` into the env is fixed (ST step 11, from geocal; done 2026-11-05)

**C++ (Boost.Test)**
* [x] Golden-archive load test: every class, XML and binary (S0); byte-identical re-write after `std::shared_ptr` (S4)
* [x] `serialize_function` file entry points: XML, binary, `.gz`; working-directory restore on exception (ST; rerun in S2)
* [x] `--without-boost-serialization` build compiles and C++ tests pass (S0, every phase)
* [x] `std::formatter` == `operator<<`; `Exception` format constructor, `source_location`, stacktrace (S1)
* [x] `std::optional` and `std::array` serialization round trip, old-archive load (S3)
* [x] Observer/Observable: notification, dead-observer removal, `add_observer_and_keep_reference`, serialization of
      observer lists with the two-pass `weak_ptr` rule (S5)
* [x] Multiple-base and virtual-diamond samples: `dynamic_pointer_cast` across every base, serialization through
      `shared_ptr<GenericObject>` (S5)
* [x] `shared_from_this()` never yields a non-owning pointer after the aliasing `this_obj`/`to_ptr` change (S4)
* [x] Observer notification for stack, member and `shared_ptr`-owned observers; constructor and `load()` registration (S5)
* [x] `mdspan` ↔ Blitz views: contiguous, strided slice, column-major; writes visible both ways (S6)
* [x] All of the above under AddressSanitizer at the end of S4 and S6

**Python (pytest)**
* [x] S0: explained `xfail(strict=True)` instead of `skip`/commented asserts (done; there was no `NameError`, and
      `test_director_example_weak_ptr` passes, so it is a normal test now)
* [x] Golden archives load via `read_shelve` (S0)
* [x] `str()` and exception text unchanged vs. baseline (S1)
* [x] Array property round trip (S3)
* [x] Downcast matrix: functions returning `shared_ptr` to the primary base, a secondary base, a virtual base, and
      `GenericObject` → most-derived type **and a method call** (the crash case) (S5/S6)
* [x] Parameter matrix: pass a derived object to parameters typed as each of those bases (S5/S6)
* [x] Identity: same C++ object returned twice; record SWIG (different wrapper) vs nanobind (same object) (S6)
* [x] `weak_ptr` matrix (creator × owner, S6.1 item 4), including directors; `test_director_example_weak_ptr` (passes
      under SWIG since S0) must keep passing, or record why not (S6)
* [x] Director lifetime: existing exact `__del__` count tests, plus "C++ keeps Python subclass alive after Python drops
      it" (S6)
* [x] Exception propagation from Python overrides (existing) and C++ exceptions mapped to the right Python type,
      including `Skeleton::TypeError` → `TypeError` (S6)
* [x] Observer notification into Python-subclass observers; template director (S5/S6)
* [x] Pickle: every class; shared identity within one pickle (`GenericObjectMap`); `weak_ptr` inside a map; director
      pickle (done); the nested C++→Python→C++ identity test from S0 still a strict `xfail` (pickle part; decision 3)
* [x] **`multiprocessing.Pool` round trip** of C++ objects and director objects (geocal production use) (ST under
      SWIG; must keep passing in S6)
* [x] shelve (sqlite) round trip (existing, keep)
* [x] numpy (through the `mdspan` bridge in S6): existing three lifetime tests, plus Fortran-order input,
      non-contiguous input, read-only returns, wrong dtype/rank gives a clear error (S5/S6)
* [x] iostream tests (existing, keep)
* [x] Overload dispatch: Blitz `double` array vs. `GenericObject` class vs. `str`, with `float32` input (S6.1 item 7)
* [x] Director called from a C++ worker thread; exceptions from a worker thread reach Python (S6.1 item 11)
* [x] Cross-module: the downstream test module creates, receives and returns main-module types (S6)
* [x] `help()` shows docstrings; `mypy` passes against generated stubs (S6)
* [x] Python tests under AddressSanitizer or valgrind (S6)

**Phase S7**
* [x] Raw pointer plus size → `mdspan`: old pointer wrappers, strided output (part of a bigger array), contiguous
      fast path and temporary for a C-style API, padded read through `submdspan`, forwarding sub-image, tiled copy,
      python subclass filling a writable numpy view in `calc()` and used from C++, `read_into` a numpy array (C++ and
      python)

---

## 13. Decisions and open questions

**Decided (maintainer, 2026-10-23):**
1. **Python API changes are acceptable**, consistent with 2.0 breaking backward compatibility. The changes listed in
   6.6 are fine; record them, but don't contort the bindings to avoid them.
2. **Module name:** keep the current name (`skeleton_swig`) for now; revisit later.
3. **Re-entrant pickling work:** set aside; the `modernize` branch excludes it (S0 step 2).
4. **No `boost::shared_ptr` support** in the converted rules; `cxx-python-rules-repo` is left alone and allowed to drift.
5. **Observer ownership:** a `shared_ptr` ownership requirement was considered acceptable, but the survey (§2.4) shows it
   isn't needed and would break constructor/`load()` registration. Instead, keep the ownership-agnostic design with the
   aliasing-constructor `this_obj`. No requirement on consumers.

6. **`weak_ptr` to an object owned only by Python** (S6.1 item 4): adopt the recommendation. Plain classes get
   `nb::new_` + `std::make_shared` constructors (generated by the converter) so they carry a real control block; for
   Python subclasses, document that a `weak_ptr` set from Python is only valid while a C++ `shared_ptr` also owns the
   object. Revisit only if a real (non-test) case appears.

7. **S4 does not port the SWIG layer**: the Python bindings may be broken from S4 until S6 (§8). The SWIG
   `std::shared_ptr` re-fork is skipped. geocal does the same in its Phase 5 (item 10).
8. **Commit policy:** stage with `git add`, never commit (the maintainer commits and tags).

9. **geocal (and later framework) copies this repository's `swig_rules/`** (and the changed `config/m4/` files, such as
   the S0.6 `ac_boost.m4` fix) **directly from the skeleton.** Don't worry about the upstream repositories
   (`cxx-python-rules-repo`, `autoconf-repo`) for now. Merging back can be looked at later; with nanobind,
   `cxx-python-rules-repo` may end up not existing at all. So keep `swig_rules/` and `config/m4/` here self-contained and
   in a state that can be copied as a whole.
10. **geocal may also break its Python bindings in its Phase 5** (`std::shared_ptr`) until Phase 6 (nanobind), the
    same as S4 here. So geocal won't need a SWIG `std::shared_ptr` port either.

**Decided (maintainer, 2026-10-30):**
11. **Phase ST is approved**, before S1, with the §4a step 9 coverage target.
12. **Fix the `"blah"` printing bug in ST** (§4a bug 3), with its tests. The S1 "`str()` unchanged" baseline is
    recorded after the fix.
13. **`.json` shelves: keep them.** The maintainer would have removed them if unused, but the check (2026-10-30)
    found uses: framework's `python/test/swig_shelve_test.py::test_all_formats` writes and reads a C++ object
    (`PressureSigma`) as `.json`; geocal's `python/bin/sba` documents `.json` output; ecostress-level1
    `end_to_end_testing/iss_orbit_determine.py` uses a `.json` shelve for a plain dict. The failure here is
    skeleton-only: its `lib/serialize_function.i` returns a `bytearray`, which jsonpickle encodes as an empty object.
    geocal, framework and `swig_rules/lib/serialize_function.i.sample` return `bytes`, which round-trips (checked).
    Fixing bug 1 by bringing `serialize_function.i` in line with the sample fixes bug 2 too. For S6, the pickle blob
    must stay `bytes`.
14. **Pin SWIG** in `env/linux/pixi.toml` to the version of the committed wrappers (`4.5.*`; geocal pins `4.4.*`).
    Short-term only: it goes away with SWIG in S6.
15. **`swig_rules` changes may be made in either repository for now.** Both geocal and the skeleton are changing it
    while they are being updated. The long-term rule is "skeleton first, then copy" (decision 9), but not enforced in
    the near term. Consequence: before each copy in either direction, diff the two `swig_rules/` (and `config/m4/`)
    trees and merge by file, as done in §2.5, instead of copying blindly.
16. **Keep all 9 Blitz element types**, and test them (ST step 3).
17. **`python-install` uninstalls before installing** (fixes the stale-copy shadowing gotcha, §4a). Done in ST;
    geocal should make the same change (§0 "Notes for geocal").

**Decided (maintainer, 2026-11-05):**
18. **`make check` must test the library just built: accept the ST.11 fix.** Never-installed programs linked to the
    library use `UNINSTALLED_PROGRAM_LINK_FLAGS = -no-install -Wl,-rpath,$(abs_top_builddir)/.libs` (`lib/lib.am`), so
    `.libs` comes before conda's env rpath. Keep watching it (for example, check `readelf -d` on the test program when
    the env or libtool changes). geocal should make the same change (§0 "Notes for geocal"); this closes geocal plan
    open item 5 once geocal has it.

19. **`--without-boost-serialization` covers only the C++ library and its tests**, not the Python extension (ST finding
    A). Documented in `CLAUDE.md`.
20. **Accept the Boost.Serialization leak on a truncated archive** (ST finding B): a Boost error-path leak with no safe
    fix on our side. Note it; keep watching.

21. **clang-format after all, for the skeleton and geocal** (revised 2026-11-05, approved by the maintainer for both;
    framework will follow when it is modernized).
    First decided "no clang-format" (the S1.6 trial wasn't worth the diff); then revisited with
    `clang-format-auto-infer`, which found a configuration close to the existing style (progress log, "S1.6 revisit";
    how it was done: §5b).
    The maintainer's reasoning: like `ruff format`, there are things one would do differently, but a standard tool plus
    all changes conforming to it counts for a lot. `.clang-format` and `.clang-format-ignore` at the repository root,
    the **same `.clang-format` in both repositories** (geocal approved after reviewing its reformat and the test
    results); `make format`, `make format-check` and `make lint` (clang-tidy, `.clang-tidy`) in both. §5a (formatting
    by AI) is no longer the plan. No `.git-blame-ignore-revs` (maintainer, 2026-11-05: this branch changes a lot anyway).

**Decided (maintainer, 2026-11-06):**
22. **C++23, with gcc 14 still the minimum.** Range formatting (gcc 15) isn't worth raising the minimum for: printing
    containers is rare, and it would print `Printable` objects in a vector quoted (S1.4). `std::mdspan` (gcc 16) is
    replaced by the Kokkos alias until then. Add a raw pointer plus size → `mdspan` sample based on geocal's needs.
    CMake issues are set aside until framework.

23. **Rename away from SWIG** (maintainer, 2026-11-06): repository `cxx-python-skeleton`, `cxx_python_rules/`
    (upstream `cxx-python-rules`), python package `skeleton_cxx._bindings`; full table in §0 "Naming". Breaking
    pickles and shelves is fine; only the test data in use has to keep working (it does, unchanged).

**Open:**
* *Answered 2026-11-06 (decision 23):* final module name (item 2) and repository name.
* *Answered 2026-11-06 (added in S5, `ThreadExample`):* **Threading sample in the skeleton?** (2026-11-05, §2.6). A small `std::thread` callback sample (no `ThreadPool`
  copy) would let S6.1 item 11 test directors and exceptions on worker threads here. Without it, these are tested
  first in geocal's nanobind pilot. Recommendation: add it in S5, since it is small and the deadlock case is easier to
  debug in the skeleton.
