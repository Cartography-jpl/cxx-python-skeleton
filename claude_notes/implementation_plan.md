# swig-rules-skeleton: implementation plan for the modernization

**Date:** 2026-10-23 (reviewed and updated 2026-10-30: geocal sync §2.5, test coverage phase ST §4a)
**Status:** In progress. Phase S0 is done; the next phase is ST (test hardening); see §0.
**Purpose:** Work through every modernization step planned for geocal (and later framework) on this small repository
first, so the problems are found and solved here before touching the large repositories.

Related documents:
* `claude_notes/modernization_ideas.md`, `claude_notes/serialization_options.md`, `claude_notes/build_layout_review.md`,
  `claude_notes/swig_rules_review.md`: the earlier discussion in this repository. Section 11 of
  `modernization_ideas.md` lists what has changed since; where they disagree, this plan wins.
* `/ssdata/smyth/geocal-repo/claude_documents/cpp_modernization_and_library_plan.md`: the geocal plan this mirrors
  (referred to below as "the geocal plan").
* framework (`/home/smyth/Local/framework`): surveyed read-only; it will follow later with the same steps.

## 0. Handoff: where to pick up (read this first)

**Status (2026-10-30):** **Phase S0 is done** (all 8 steps; see the progress log). A review on 2026-10-30 measured the
test coverage (§4a) and found that the tests don't cover enough to catch regressions in S1–S6, plus a few real bugs. So
the next step is the new **Phase ST** (test hardening, §4a), done **before** S1. It also brings in the `swig_rules`
changes made in geocal since 2026-10-23 (§2.5). After ST comes **Phase S1** (C++20, tooling, hygiene), including the
clang-format trial (S1.6, using `claude_notes/clang_format_trial.clang-format`). The maintainer has to say "go" before
ST starts (approved 2026-10-30, §13 item 11; code work still waits for "go"); if unsure, ask.

**Rules for working in this repository** (maintainer decisions):
* Work on the **`modernize`** branch (already checked out). Don't touch `master` (it keeps the set-aside pickling work).
* **Stage changes with `git add`, never commit.** The maintainer reviews and commits. Phase "tags" in this plan are
  suggestions for the maintainer to create after committing.
* Don't push, and don't `git subtree push` to `swig-rules-repo` (it's left alone and allowed to drift).
* Put generated documentation in `claude_notes/`.
* **The Python bindings may be broken from S4 until S6** (decided): S4 converts C++ and serialization to
  `std::shared_ptr` and is verified by the C++ tests and golden archives only; the SWIG side is not ported. See §8.

**Locations:**
| What | Where |
|---|---|
| This repo | `/home/smyth/Local/swig-rules-skeleton` (same as `/ldata/smyth/swig-rules-skeleton`) |
| Scratch build + pixi env (disposable; fine to modify its `pixi.toml`) | `/home/smyth/Local/swig-rules-skeleton-build/build-pixi`; configure with `../../swig-rules-skeleton/configure --prefix=$CONDA_PREFIX --enable-maintainer-mode --enable-debug` |
| Coverage build (scratch, uses the `build-pixi` env) | `/home/smyth/Local/swig-rules-skeleton-build/build-coverage`; recipe in §4a "Coverage builds" |
| nanobind probe sources (reference for S6.1) | `claude_notes/nanobind_probes/` (see its README) |
| geocal plan (the large repo this rehearses) | `/ssdata/smyth/geocal-repo/claude_documents/cpp_modernization_and_library_plan.md` |
| framework (follows later; read-only survey) | `/home/smyth/Local/framework` |
| Earlier discussion (superseded where it disagrees) | `claude_notes/modernization_ideas.md` (§11), `serialization_options.md` (§9), `build_layout_review.md`, `swig_rules_review.md` |

**Notes for geocal (and framework)** — things found here that geocal should also change. A Claude session working
on geocal and pointed at this repository should read this list, check each item against geocal's current code, and
apply it there (or record why not) in geocal's plan:
| Item | Found | geocal/framework action |
|---|---|---|
| `python-install`: uninstall from the prefix first, and `--ignore-installed` instead of `--force-reinstall` | 2026-10-30, ST.7 (commit `5fc4847`), decision 17 | geocal's `python/python.am` has the same rule. Two problems with `--force-reinstall`: (1) switching from a normal to an editable install leaves the old copied package in `site-packages`, which shadows the editable one; (2) it uninstalls the package from *whatever environment pip sees*, not only the prefix, so a `--prefix=/tmp/x` build removes the copy in the conda env. Copy the skeleton's rule (in `python/python.am`): `if ls -d @prefix@/@pythondir@/<pkg>-*.dist-info; then PYTHONPATH=@prefix@/@pythondir@ $(PYTHON) -m pip uninstall -y -q <pkg>; fi`, then `pip install --prefix=@prefix@ --ignore-installed --no-deps [-e] ...`. The guard matters: without it, pip uninstall goes on to remove the package from the env. Verify both switching directions as in geocal's `python_editable_install_plan.md`, and that the env's own install survives. |
| `"blah"` printing bug, and `Exception` copy constructor | 2026-10-30, §4a bug 3, decision 12, ST.7 (commit `76cdf37`) | `print_to_string` / `Exception` seed an `ostringstream` with `"blah"`, so outputs shorter than 4 characters come out wrong (`'Alah'`). Same code in geocal `lib/GeoCalCore/Support/printable.h`, `geocal_exception.h`, `lib/Msp/msp.h`, `lib/swig_quaternion.i`; framework `lib/Support/printable.h`, `fp_exception.h`. Use a plain `std::ostringstream`; add a test with a 1-character `print`. **Also** geocal's `Exception(const Exception&)` sets the copy with `s_.str(cp)`, which leaves the stream at position 0, so a later `<<` overwrites the copied message (framework already uses `s_ << E.s_.str()`, which is right). Tests: the skeleton's `lib/printable_test.cc` and `lib/skeleton_exception_test.cc`. |
| `.bin`/`.json` shelves | ST.5/6 (commit `f1d57f1`) | Skeleton-only bug (its `serialize_function.i` had drifted from the sample). Nothing to do in geocal, but geocal's `sqlite_shelf.py` gets the same jsonpickle `DeprecationWarning`; pass `keys=True` to `jsonpickle.encode`/`decode` as the skeleton now does. |
| `skip_weak_ptr_handling(true)` used stale marks | ST.5 (commit `d264924`) | Fixed in `swig_rules/lib/serialize_function.cc` (`mark_pointer` clears before returning). Nobody calls it in geocal/framework; comes along with the next `swig_rules` copy. |
| `doxygen_single.cfg` `DOT_MULTI_TARGETS` | ST.1 | Obsolete in doxygen 1.18 (warning on every module). Removed here; geocal's copy (doxygen 1.13) will need the same when its doxygen is updated. |
| Test coverage measurement | 2026-10-30, §4a | Recipe in §4a "Coverage builds" (`gcovr`; the system `lcov` 1.14 can't read gcc 15 data). geocal should measure its own coverage before its Phase 5/6, especially for the `swig_rules` typemaps (table in §4a). |
| `swig_rules` changes flow both ways for now | decision 15 | Before copying `swig_rules/` or `config/m4/` in either direction, diff the trees and merge file by file. geocal-only settings (e.g. `swig_rules/__init__.py` `if True:`) must not be overwritten. |
| SWIG pin | decision 14 | geocal already pins `4.4.*`; the skeleton pins `4.5.*`. Keep each pinned to its committed wrappers. |

**Progress log** (update as phases complete; one line per step):
| Phase / step | State | Notes |
|---|---|---|
| Planning | done | 2026-10-23 |
| S0.1 baseline | done | 2026-10-23. C++: 1 test program (`skeleton_test_all`) passes. Python: 16 passed, 1 skipped (`test_director_example_weak_ptr`). `test_director_example_object_tracking` passed (no `NameError`; its identity asserts were commented out). Env: gcc 15.3, Boost 1.90, SWIG 4.5.0. |
| S0.2 set aside pickling | done | Asserts uncommented; `object_tracking` is `xfail(strict=True)` (the `e1_nest_p` identity assert fails). `test_director_example_weak_ptr` **passes when unskipped** (5 of 5 runs), so it is now a normal test, not an `xfail`. |
| S0.3 golden archives | done | `unit_test_data/golden/` (52 files), `make golden-archives`, `lib/golden_archive_test.cc`, `python/tests/unit/golden_archive_test.py`. Findings: (a) `serialize_function.i` lacks `serialize_read_binary_generic` (the sample has it), so `.bin` through `read_shelve` fails: 5 strict `xfail`s, fix when convenient. (b) `blitz::Array<float/short/unsigned/ushort>` are instantiated in the `.cc` but lack `BOOST_SERIALIZATION_SPLIT_FREE` in the header, so can't be serialized; left alone. |
| S0.4 tooling in pixi | done | `clang-tools` and `nanobind` in `env/linux/pixi.toml` (+ lock) and in both scratch envs. `clang-format`/`clang-tidy` 22.1.8 and `nanobind` 3.1.0 work. **`bear` and `intercept-build` do not run on this host, so `bear` was removed from all three pixi setups** (glibc 2.28; the conda `bear-driver` needs 2.39 and the `intercept-build` preload crashes). This is a host problem, not a gcc version one, so a gcc 16 env would not help and the gcc 14 env is no different. Workaround, tested: `script/make_compile_commands.py` (builds `compile_commands.json` from `make -n`, run in the build dir) and `script/run_clang_tidy.sh` (adds the conda sysroot and gcc builtin headers, and a shim `blitz/llvm/bzconfig.h` -> `gnu`, because the conda blitz has no llvm config and clang can't parse blitz otherwise). Use these for S1.3 instead of `bear -- make`. First look: `modernize-use-nullptr` finds 3 hits in `swig_rules/lib` headers, `modernize-use-override` finds the `virtual` functions in `director_example.h`. |
| S0.5 min-compiler env | done | `/home/smyth/Local/swig-rules-skeleton-build/build-pixi-gcc14` (gcc/g++ 14.4.0, SWIG 4.5.0). Configure with `--prefix=$CONDA_PREFIX --enable-maintainer-mode --enable-debug`, `make`, `make check` (1/1), `make install`, `make installcheck` (21 passed, 6 xfailed): same as the gcc 15 results. Run `pixi run` from inside that directory (it warns about `PIXI_PROJECT_MANIFEST` pointing at `build-pixi`; the local manifest is the one used). |
| S0.6 `--without-boost-serialization` | done | Builds and C++ tests pass (`--without-swig --without-documentation`). Fixed: 3 `.cc` files guarded, `swig_rules/lib/serialize_shared_ptr.h` body guarded, `-DSWIG_HAVE_BOOST_SERIALIZATION` only when serialization is on (new `HAVE_BOOST_SERIALIZATION` conditional), map tests skip, a stale `GEOCAL_` macro in the blitz test, `ac_boost.m4` no longer requires `libboost_serialization` in that case. Geocal reference: the same pattern. |
| S0.7 housekeeping | done | No `*~` files were tracked. `.gitignore` extended (egg-info, pytest_tmp, test xml). |
| S0.8 sanitizer | done | See "Sanitizer builds" below. |
| Review 2026-10-30 | done | geocal changes since 2026-10-23 reviewed (§2.5). Coverage measured (§4a): C++ `lib/` + `swig_rules/lib/` 52% of lines, Python `skeleton` package 42% (`swig_shelve.py` 38%); most `swig_rules` typemaps are never exercised. Bugs found that no test catches: §4a "Bugs found". `gcovr` and `pytest-cov` added to the scratch `build-pixi` env (not yet to `env/linux/pixi.toml`). Phase ST added before S1. |
| ST | not started | Approved 2026-10-30 with decisions §13 items 11–17. `.json` shelves kept (they are used downstream). Waiting for "go". |

---

**Naming.** If the bindings move to nanobind, the names `swig-rules-skeleton`, `swig_rules/`, `swig-rules-repo` and the
`*_swig` Python module names no longer describe the contents. We keep them for now, for historical reasons and so that
the subtree remotes and downstream paths keep working. Add a short note to `README.md` and `swig_rules/README.md` when
the nanobind step lands ("the name is historical; the directory now holds the nanobind rules").

**Branching.** This is a sandbox with no dependents. The work happens on the **`modernize`** branch, which the
maintainer created from a point before the unfinished re-entrant pickling work (it does not contain
`swig_serialization_context.*`, `SerializeSessionLock` or the pickler classes). Changes are **staged, not committed**;
the maintainer commits and can tag each phase boundary (for example `phase0-baseline`, `swig-final`, `nanobind-1`).

**swig-rules repository.** Leave `swig-rules-repo` (the upstream of the `swig_rules/` subtree) alone for now. All
changes are made only in this repository's copy of `swig_rules/`, and it is understood that it will drift from
`swig-rules-repo`. No `git subtree push`. **geocal copies `swig_rules/` (and changed `config/m4/` files) directly from
this repository** (§13 item 9), so keep them self-contained and copyable.

**S0 phase-end checklist (repeat at the end of each phase):** (1) gcc 15 env (`build-pixi`): `make check`, `make install`,
`make installcheck`; (2) gcc 14 env (`build-pixi-gcc14`): the same; (3) `--without-boost-serialization --without-swig
--without-documentation` build with `make skeleton-check`; (4) ASan build (below); (5) golden archives load (part of 1-2).
Expected at the end of S0: C++ 1/1, Python 21 passed + 6 xfailed.

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
| Build env (`../swig-rules-skeleton-build/build-pixi`) | gcc **15.3**, Boost **1.90**, SWIG **4.5.0**, Blitz 1.0.2, Python 3.13, numpy 2.5, doxygen 1.18. That is already newer than geocal's env (gcc 15.1, Boost 1.84, SWIG 4.3.1). |
| C++ standard | `AX_CXX_COMPILE_STDCXX([17], [ext], [mandatory])` in `config/m4_local/skeleton_common.m4` (and in the shared `config/m4/afids_common.m4`) |
| C++20 readiness | All `lib/*.cc` and `swig_rules/lib/*.cc` pass `-fsyntax-only -std=gnu++20` (with the real build's `-DHAVE_CONFIG_H -DSWIG_HAVE_BOOST_SERIALIZATION`). |
| `--without-boost-serialization` | **Broken**: 4 files fail to compile with `SKELETON_HAVE_BOOST_SERIALIZATION` undefined (`director_example.cc`, `generic_object_map.cc`, `generic_object_sample.cc`, `serialize_function.cc`; errors are `SKELETON_NVP_` / `SKELETON_GENERIC_BASE` / `make_nvp` used outside the guard). This is the same problem as geocal (12 files there). |
| Tests | C++: 7 Boost.Test cases (`generic_object_map_test.cc`, `printable_test.cc`, `blitz_array_serialize_support_test.cc`). Python: 17 tests in `python/tests/unit/`. |
| Known-broken tests on `master` | `test_director_example_weak_ptr` is `@pytest.mark.skip` (since the SWIG 4.2.1 change, see `swig_rules_review.md`). `test_director_example_object_tracking` has its key assertions commented out and refers to `PickleWithSwigSupport`, **which is not defined or imported**, so it likely fails with `NameError` (to confirm in Phase 0). |
| In-progress work | `master` carries unfinished re-entrant pickling work (`swig_serialization_context.*`, `SerializeSessionLock`, `PicklerWithSerializationSupport`; commits `46d6216`–`c03e221`). This is **not in `swig-rules-repo/master`** and not in geocal's copy of `swig_rules`. **Set aside:** the `modernize` branch, where this plan is carried out, doesn't contain it. |
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
| C++ standard | **C++20 now**, minimum **gcc 14**; C++23 later (after SWIG is gone) | Same; switch in Phase 1 |
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
| **S7** | Later: `mdspan` for raw pointer-plus-size interfaces, C++23, CMake reference expansion | 7 |

---

## 4. Phase S0: baseline and safety net

1. **Record the baseline.** In `../swig-rules-skeleton-build/build-pixi`: `configure --enable-maintainer-mode
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

**Exit:** ST steps done; S0 phase-end checklist green (gcc 15, gcc 14, `--without-boost-serialization`, ASan);
suggested tag `phaseT-tests`.

### Coverage builds

The system `lcov` (1.14) can't read gcc 15 coverage data; use `gcovr` from the pixi env.

    cd /home/smyth/Local/swig-rules-skeleton-build/build-coverage      # uses the build-pixi env
    pixi run --manifest-path ../build-pixi/pixi.toml bash
    ../../swig-rules-skeleton/configure --prefix=$PWD/inst --without-documentation \
        CXXFLAGS="-O0 -g --coverage" LDFLAGS="--coverage"
    make -j 16 && make install PYTHON_EDITABLE_MODE=t
    find . -name '*.gcda' -delete
    make skeleton-check
    source script/setup_skeleton.sh
    (cd ../../swig-rules-skeleton/python && pytest --cov=skeleton --cov-report=term-missing tests)
    gcovr -r ../../swig-rules-skeleton . --filter '.*/swig-rules-skeleton/(lib|swig_rules/lib)/' \
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
6. **clang-format trial** (maintainer is considering it as a routine tool, like `ruff format`): use
   `claude_notes/clang_format_trial.clang-format` (tuned to geocal's style; measured impact on geocal in the geocal plan
   §8.5). Format the skeleton with it after the clang-tidy passes, review the result with the maintainer, and adjust the
   config: function-signature layout, `{ body }` after signatures, `// clang-format off` around serialization `&` chains.
   If adopted, add the file as `.clang-format` at the repo root and record the reformat commit for
   `.git-blame-ignore-revs`.
7. **Mechanical tests** (C++): formatter output equals `operator<<` output; the format-string constructor produces the
   expected message and file/line; copying an exception keeps the message; stacktrace is non-empty in a debug build.
   Python: `str()` of each class unchanged (compare with S0 output), exception message text unchanged.

**Exit:** builds with gcc 15 and gcc 14 as C++20; all tests pass; tag `phase1-cxx20`.

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
8. **iostream**: Python file object ↔ `std::istream`/`std::ostream` via a small `streambuf` (no nanobind equivalent).
   The existing `iostream_swig_test.py` is the acceptance test.
9. **STL containers**: SWIG's `%template(Vector_DirectorExample)` gives a reference-semantics proxy type that the
   tests use. nanobind's `stl/vector.h` converts to a Python `list` (copy). Use `nb::bind_vector` where the proxy type
   and its lifetime behavior matter. Decide per type and record it as a Python API change if a type disappears.
10. **Cross-module types**: build a second tiny extension module (`_skeleton_downstream`) in this repo that uses and
    returns types from the main module, to prove the geocal → ECOSTRESS and framework → instrument-repo pattern
    (nanobind version pinned across modules).

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
  * anything in `%extend` / `%pythoncode` → flagged for hand conversion.

  The `.i` files are the specification of the Python API, so this keeps it stable. Run it on the skeleton's 9 `.i`
  files and hand-fix the output. The goal is that geocal's 230 and framework's 324 `.i` files convert with only
  modest hand work, so measure how much hand work the skeleton's output needed.
* Module layout: one extension module (`skeleton._skeleton` or keep the `skeleton_swig` name for compatibility; decide)
  with one `init_<file>(nb::module_&)` per header group.
* Docstrings: generate a `docstrings.h` from the existing doxygen XML (a `doxy2nanobind.py` adapted from
  `doxy2swig.py`, or `pybind11_mkdoc`, which emits plain strings usable by nanobind; evaluate both on the skeleton).

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

**Python API changes to record**:
* the `.this` attribute no longer exists (tests use `olist[0].this`);
* `Vector_*` proxy types (if not kept with `bind_vector`);
* `print_to_string` (keep as a method or not);
* the module name;
* the `SerializeSessionLock`/pickler classes if they become unnecessary;
* identity semantics: the same C++ object now gives the same Python object.

---

## 11. Phase S7: later

* `mdspan` for C++ interfaces that today pass a raw pointer plus sizes (in geocal: raster tiles, GDAL/VICAR buffers, FFT
  input/output). The skeleton has few such interfaces, so add a small sample only if geocal wants a worked example
  first. The Python boundary already uses `mdspan` from S6.
* C++23 once SWIG is gone (deducing `this` for `Printable`, `std::expected`, `views::enumerate`/`zip`).
* Expand the CMake reference if framework wants more from it.

---

## 12. Test additions: summary checklist

**Phase ST (§4a; before S1)**
* [ ] Coverage tooling (`gcovr`, `pytest-cov`) in `env/linux/pixi.toml` and a `make coverage` target
* [ ] Every `swig_rules` typemap/macro used by geocal or framework has a Python test (table in §4a)
* [ ] Exact `str()` for every sample class; C++ `Exception` → `RuntimeError` with exact message; unknown throw
* [ ] `operator<<` == `print_to_string`, short outputs (the `"blah"` bug); `Exception`/`range_check` C++ tests
* [ ] `serialize_function` file entry points: XML, binary, `.gz`; missing file; working-directory restore on exception
* [ ] `swig_shelve` every format, `shelve_time_after`, `extra_python_init`, `SQLiteShelf`
* [ ] `multiprocessing.Pool` round trip (plain and director object)
* [ ] `%shared_ptr_value_traits` vector sample (from geocal)

**C++ (Boost.Test)**
* [ ] Golden-archive load test: every class, XML and binary (S0); byte-identical re-write after `std::shared_ptr` (S4)
* [ ] `serialize_function` file entry points: XML, binary, `.gz`; working-directory restore on exception (ST; rerun in S2)
* [ ] `--without-boost-serialization` build compiles and C++ tests pass (S0, every phase)
* [ ] `std::formatter` == `operator<<`; `Exception` format constructor, `source_location`, stacktrace (S1)
* [ ] `std::optional` and `std::array` serialization round trip, old-archive load (S3)
* [ ] Observer/Observable: notification, dead-observer removal, `add_observer_and_keep_reference`, serialization of
      observer lists with the two-pass `weak_ptr` rule (S5)
* [ ] Multiple-base and virtual-diamond samples: `dynamic_pointer_cast` across every base, serialization through
      `shared_ptr<GenericObject>` (S5)
* [ ] `shared_from_this()` never yields a non-owning pointer after the aliasing `this_obj`/`to_ptr` change (S4)
* [ ] Observer notification for stack, member and `shared_ptr`-owned observers; constructor and `load()` registration (S5)
* [ ] `mdspan` ↔ Blitz views: contiguous, strided slice, column-major; writes visible both ways (S6)
* [ ] All of the above under AddressSanitizer at the end of S4 and S6

**Python (pytest)**
* [x] S0: explained `xfail(strict=True)` instead of `skip`/commented asserts (done; there was no `NameError`, and
      `test_director_example_weak_ptr` passes, so it is a normal test now)
* [ ] Golden archives load via `read_shelve` (S0)
* [ ] `str()` and exception text unchanged vs. baseline (S1)
* [ ] Array property round trip (S3)
* [ ] Downcast matrix: functions returning `shared_ptr` to the primary base, a secondary base, a virtual base, and
      `GenericObject` → most-derived type **and a method call** (the crash case) (S5/S6)
* [ ] Parameter matrix: pass a derived object to parameters typed as each of those bases (S5/S6)
* [ ] Identity: same C++ object returned twice; record SWIG (different wrapper) vs nanobind (same object) (S6)
* [ ] `weak_ptr` matrix (creator × owner, S6.1 item 4), including directors; `test_director_example_weak_ptr` (passes
      under SWIG since S0) must keep passing, or record why not (S6)
* [ ] Director lifetime: existing exact `__del__` count tests, plus "C++ keeps Python subclass alive after Python drops
      it" (S6)
* [ ] Exception propagation from Python overrides (existing) and C++ exceptions mapped to the right Python type,
      including `Skeleton::TypeError` → `TypeError` (S6)
* [ ] Observer notification into Python-subclass observers; template director (S5/S6)
* [ ] Pickle: every class; shared identity within one pickle (`GenericObjectMap`); `weak_ptr` inside a map; director
      pickle; the nested C++→Python→C++ identity tests from S0 now passing (S6)
* [ ] **`multiprocessing.Pool` round trip** of C++ objects and director objects (geocal production use) (ST under
      SWIG; must keep passing in S6)
* [ ] shelve (sqlite) round trip (existing, keep)
* [ ] numpy (through the `mdspan` bridge in S6): existing three lifetime tests, plus Fortran-order input,
      non-contiguous input, read-only returns, wrong dtype/rank gives a clear error (S5/S6)
* [ ] iostream tests (existing, keep)
* [ ] Cross-module: the downstream test module creates, receives and returns main-module types (S6)
* [ ] `help()` shows docstrings; `mypy` passes against generated stubs (S6)
* [ ] Python tests under AddressSanitizer or valgrind (S6)

---

## 13. Decisions and open questions

**Decided (maintainer, 2026-10-23):**
1. **Python API changes are acceptable**, consistent with 2.0 breaking backward compatibility. The changes listed in
   6.6 are fine; record them, but don't contort the bindings to avoid them.
2. **Module name:** keep the current name (`skeleton_swig`) for now; revisit later.
3. **Re-entrant pickling work:** set aside; the `modernize` branch excludes it (S0 step 2).
4. **No `boost::shared_ptr` support** in the converted rules; `swig-rules-repo` is left alone and allowed to drift.
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
   (`swig-rules-repo`, `autoconf-repo`) for now. Merging back can be looked at later; with nanobind,
   `swig-rules-repo` may end up not existing at all. So keep `swig_rules/` and `config/m4/` here self-contained and
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

**Open:**
* Final module name (item 2).
