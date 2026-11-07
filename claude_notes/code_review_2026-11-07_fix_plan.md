# Plan: fix what the 2026-11-07 code review found

**Date:** 2026-11-07
**Status:** Done (2026-11-07), except F2.5 (CI, deferred). See the progress log.
**Input:** `claude_notes/code_review_2026-11-07.md` ("the review"; its section numbers are given as §4.1 and so on).
**Goal:** when every step is done, the problems in the review are fixed or have a written reason for being left. The
python API, the archive formats and the test results stay the same, apart from the changes listed in "Intended
behavior changes".

Rules (the same as `implementation_plan.md` §0 and the previous fix plan): work on `modernize`; stage with `git add`,
never commit; run `clang-format -i` on C++ you change and `ruff format` on python you change; mirror changes into the
matching `.sample` files under `cxx_python_rules/`; a step is done when its checks pass and the full test suite passes.
After each phase, add a line to the progress log at the end of this file, and a row to the "Notes for geocal" table in
`implementation_plan.md` §0 for anything geocal should also do.

Phases are named F0 to F10 (F for this fix plan; the previous plan used R0 to R9).

## Intended behavior changes

Everything else must stay the same, which the F0 snapshot checks.

| Change | Step | Who notices |
|---|---|---|
| A `weak_ptr` written after a nested pickle (a python subclass object with a C++ attribute, earlier in the same archive) is kept; it used to be written as null | F1.1 | Nobody, except that the connection now survives. Archives written before still load. |
| Python `serialize_write_string` returns `bytes`; `serialize_read_generic_string` accepts `bytes` or `str` (D6) | F1.3 | Python callers of these two. Neither the skeleton's tests nor geocal's python code calls them. |
| The type registry key includes the namespace (version 2) | F1.4 | Nobody: all modules of a project are built together. A module built before the change doesn't share the registry with one built after. |
| The in-memory serialization functions are renamed in C++ and python, and the old names are gone (D8) | F4 | Callers of the old names. In geocal: `mars_igc_test.py` and the pickling helpers, switched when geocal copies `cxx_python_rules/`. Existing pickles and shelves still load. |
| Python file name arguments of the serialization functions also accept `pathlib.Path` | F4 | Nobody; `str` still works. |
| `configure` requires Boost 1.84 | F3.1 | Builds with an older Boost fail at configure, with a clear message, instead of during the compile. |
| Debug builds define `_GLIBCXX_ASSERTIONS` | F2.2 | An out-of-range access to a standard container stops the program with a message in a debug build (it was undefined behavior). |
| `str()` of a `NumpySample` gives its description instead of nanobind's default | F3.3 | Python callers printing one. |
| `RasterSample.read` with a negative size raises the region error before allocating | F3.5 | Python callers passing negative sizes (they got a different exception before). |
| Loading a `CacheExample` with no observed object throws an `Exception` instead of crashing | F3.4 | Only damaged archives. |
| Copying a `GenericObject` through a `GenericObject&` (slicing) no longer compiles | F5.2 | C++ code that does that; geocal's trial build (F5.2) finds any. |
| An `Observable` written to an archive never lists observers that have already been destroyed | F7.2 | Nobody: such entries loaded as empty and were dropped. Only if F7.2 is done. |
| CMake installs the library with a package configuration | F8 | CMake users (framework). |

## Decisions

- **D6. Python type of the XML archive strings (§4.3). Decided: `bytes`.** `serialize_write_string` returns `bytes`;
  `serialize_read_generic_string` accepts `bytes`, or `str` (encoded as UTF-8 with `surrogateescape`). For the
  record: SWIG returned `str` decoded with `surrogateescape` (SWIG 4.5's `pystrings.swg`) but encoded input strictly,
  so a SWIG round trip of such an archive failed too.
- **D7. Pickles in new XML archives (§4.3). Decided: no change.** They stay raw; no new archive format.
- **D8. Names of the in-memory functions (§4.5). Decided: rename, and drop the old names now** (no deprecated
  wrappers or aliases). Every caller in this repository switches in the same step; geocal switches its few callers
  when it copies `cxx_python_rules/`.

  | Now | New |
  |---|---|
  | C++ `std::string serialize_write_binary(const std::shared_ptr<GenericObject>& Obj)`; python `serialize_write_binary(Obj)` | `serialize_write_binary_string(Obj)` |
  | C++ `serialize_read_binary(const std::string& Data)`; python `serialize_read_binary(Data)` | `serialize_read_binary_generic_string(Data)` |
  | C++ `serialize_read_binary(Data, Base_dir)`; python `serialize_read_binary(Data, Base_dir)` | `serialize_read_binary_generic_string(Data, Base_dir)` |

  This fits the existing pattern: `_generic` returns a `GenericObject`, `_string` means in memory, `binary` is the
  format (compare `serialize_read_generic_string`, `serialize_read_binary_string<T>`). File functions and the
  templates keep their names. With the non-template `serialize_read_binary` gone, `serialize_read_binary("x.bin")`
  without `<T>` no longer compiles instead of parsing the name as an archive.
- **D9. Free-threaded python (§5.1). Decided: a long-term goal, not required now.** Write down thread safety per class
  (F7.1). Do what helps the future goal when the code is being changed anyway, best effort: the "in passing" items in
  F7.2, each skipped if it turns out not to be simple. F7.3 lists what is left for a later free-threading phase, so
  the list isn't lost.
- **D10. Comment sweep (§5.6). Decided: in this plan (F9),** before geocal next copies `cxx_python_rules/`.
- **D11. CI (§6.4). Decided: yes, on the CI runner at JPL, but deferred** (maintainer, 2026-11-07): it is easier to set
  up at JPL with access to the runner and GitHub. This plan writes `script/check_all.sh` (F2.4), run by hand for now;
  the workflow itself is in "Deferred: CI on the JPL runner".
- **D12. CMake reference build (§6.5). Decided: fix now (F8).**
- **D13. Observer lists as `std::vector` (§5.3). Decided: A, switch to `std::vector` in F7.2.** The explanation the
  decision was based on:
  - *What it is.* `Observable<T>` keeps its observers in two `std::list`s of `weak_ptr` (`olist` and `colist`,
    `observer.h:328-329`). The suggestion is `std::vector`, with `std::erase_if` where the code now calls the list's
    `remove_if`.
  - *The code change.* About 15 lines in `observer.h`: the two member types, `remove_if` → `std::erase_if` in the
    remove functions and `clean_dead_ptr`, and the `merge_loaded` template. `observer_serialize_support.h` doesn't
    change (it uses `decltype(olist)`), and Boost's `vector.hpp` is already included.
  - *What it gains: little.* A vector keeps its elements in one block; a list allocates a node per observer.
    `notify_update_do` copies the list on every notification (`auto ol = olist;`), which for a vector is one
    allocation instead of one per observer. The list's strengths (iterators that stay valid, cheap removal in the
    middle) aren't used: the code never keeps an iterator, and removal scans the whole list either way. With the few
    observers an object usually has, nobody would measure a difference. The main gain is that the code uses the
    container a reader expects today.
  - *Archive risk: none.* Checked 2026-11-07 with a test program against the scratch build: Boost writes a
    `std::list` and a `std::vector` of `shared_ptr` or `weak_ptr` byte for byte the same, in XML and in binary, and
    an archive written from a list loads into a vector. Old archives load, new ones are identical, and the golden
    archives (`observer_ctor`, `observer_map`, `simple_camera_like`) would confirm it.
  - *The cost is coordination.* `observer.h` is a copy of framework's (`lib/Support/observer.h`, which is geocal's
    plus additions), so the skeleton's tests cover the class those projects use. The copy already differs: deleted
    copying (D2), `std::shared_ptr` (S4), `merge_loaded`, each with a row in the notes for geocal. This would be one
    more such row, and since the change is small and archive-compatible, geocal and framework could take it whenever
    they modernize their copy.
  - *It overlaps with D9.* The "in passing" change to `notify_update_do` (F7.2) edits the same functions, so doing
    both at once costs little more than doing one.
  - *Options.* (A) Do it in F7.2 together with the `notify_update_do` change, and add a row to the notes for geocal
    and framework. (B) Skip it; close the review item as not worth the extra row.
  - *Recommendation: A.* (The first version of this plan said B, before the archive check and before D9 put
    F7.2 on the list.) B is fine if you would rather keep `observer.h` as close to framework's as possible until
    framework's own modernization.

## Not doing, with reasons

| Review item | Reason |
|---|---|
| A helper template for the "call the C++ implementation when self is a python subclass" lambdas (§5.5) | Can't be done: a call through a member function pointer is always virtual, and the non-virtual call needs the function's name. The review is corrected. |
| `final` on the sample classes in `lib/` (§5.3) | The samples are templates for project classes, which are often bases. F5.8 marks only the small helper classes in `cxx_python_rules/`. |
| `GlobalFixture::test_data_dir()` returning `std::filesystem::path` (§5.4) | geocal's tests use `test_data_dir() + "file"` everywhere; the change would touch them all for little gain. F6.7 does the rest of that item. |
| `FileReferenceSample` storing a `std::filesystem::path` (§5.3) | The member is serialized as a string; changing its type changes the archive code for no benefit. F5.7 fixes the error message. |
| Installed headers under `skeleton/` (§6.6) | geocal uses the same flat layout. |
| Moving `i_to_nanobind.py` out (§8) | It is needed until geocal is converted. |
| Free-threaded python support itself (§5.1) | D9: later; F7.3 lists what is left. |
| CI (§6.4) | D11: deferred, to be set up at JPL; notes in "Deferred: CI on the JPL runner". |
| The items in review §7 | Decided earlier. F9.3 adds the trusted-input note. |

## Order and dependencies

F0 → F1 → F2 → F3 → F4 → F5 → F6 → F7 → F8 → F9 → F10. Notes:
- F1 comes first: the bugs, in files geocal copies, so geocal can take them early.
- F2 (sanitizers, assertions, clang build, check script) comes before the refactors, so they check everything
  after. CI is deferred (D11).
- F5.2 comes before F6.8's `cppcoreguidelines-special-member-functions` check and F6.9's `-Wdeprecated-copy-dtor`.
- F4 comes before F9, so the documentation describes the new names.
- F9 comes near the end, so the comment sweep covers comments written by this plan too.
- **In every step (D9):** add no new process-wide mutable state. Where it can't be avoided, say so in the "Threads"
  section of `cxx_python_rules/README.md` (F7.1).
Only library features gcc 14 has: heterogeneous lookup in unordered containers (gcc 11) and `<spanstream>` (gcc 12)
are fine.

## F0. Baseline

1. Rebuild `build` (gcc 15), `build-gcc14`, `build-noserial`, `build-asan` and `build-cmake` from the current `HEAD`
   (rifle: the matching `build-pixi*` directories) and record the `make check` counts (R9 had 208 passed, 1 xfailed;
   the `read_shelve` follow-up added tests since) and 0 warnings.
2. API snapshot: run `script/api_snapshot.py` and save it as `baseline/api_f0.txt` (next to the R0 files in
   `cxx-python-skeleton-build/baseline/`); check the golden archives with `baseline/check_golden.sh` (104 files).
3. Add the reproductions from the review as tests marked `xfail(strict=True)`, so F1 shows them going from failing to
   passing:
   - `python/tests/unit/serialize_nested_test.py`: a `GenericObjectMap` with `"a"` (a `GenericObjectA`), `"b"` (a
     python subclass of `DirectorExample` holding a `GenericObjectA`) and `"c"` (a `GenericObjectC` with a `weak_ptr`
     to `"a"`); after a round trip, `c.a` is `"a"`. Through binary and XML, string and file. Also the same map without
     `"b"` (passes today, so not `xfail`).
   - In `director_example_test.py`: `serialize_write_string` and `serialize_read_generic_string` round trip a python
     subclass object (§4.3).
   - If it stays small: a C++ test in `serialize_function_test.cc` with a test-only class whose `save()` calls
     `serialize_write_binary` on a member, so the nesting is also covered without python.
4. Save the wide clang-tidy command from review §9 as `baseline/tidy_wide.sh` and its output (154 findings) for F10.

## F1. Bugs in `cxx_python_rules/` (review §4.1 to §4.4)

**F1.1 One `weak_ptr` mark set per write (§4.1).** In `weak_ptr_serialize_support.{h,cc}`:
- Replace the process-wide `std::set` and `recursive_mutex` with a per-thread stack of sets. The stack starts with
  one bottom set, which is used outside any `serialize_write*` call, so code that writes an archive directly behaves
  as it does today (geocal has three tests that do: `image_coordinate_test.cc`, `igc_rolling_shutter_test.cc`,
  `blitz_array_serialize_support_test.cc`).
- `PtrSerializeLock` becomes `PtrSerializeScope`: the constructor pushes a new empty set, the destructor pops it. It
  is used only in `serialize_function.cc`, so no alias is needed.
- `add_ptr_serialized_reference`, `is_ptr_serialized` and `clear_ptr_serialized_reference` work on the top set; keep
  their names (geocal's classes call `add_ptr_serialized_reference`).
- A nested write (pickling a python object during a write) now pushes its own set and leaves the outer one alone.
  Writes on different threads have separate stacks, so no lock is needed (this is also the D9 direction: no shared
  state left here).
- Delete `ptr_serialize_depth`, the commented-out code and the long note in the `.cc`; rewrite the header comment to
  describe the new rule (one mark set per write, per thread).

Checks: the F0.3 tests pass (remove `xfail`); a new C++ test in `serialize_function_test.cc` has two threads each
write a map holding a `weak_ptr` 50 times at once, and every result keeps its `weak_ptr` (the lock used to guarantee
this; there is no such test today, and the per-thread stacks must keep it true); the ASan build passes; golden
archives identical. **Trial in framework**, read-only for framework (a scratch copy and the scratch build in
`/data2/smythdata/framework-build`, thrown away afterwards): port the same change to framework's copy and run its unit
tests, since the earlier "outermost only" version gave null pointers there. Record the result in this plan. If
framework can't be built here, record that and leave the trial for the framework phase.

**F1.2 Include guard for `serialize_shared_ptr.h` (§4.2).** Add `#pragma once`; remove `<iostream>`; move the
`shared_ptr_helper.hpp` include and its workaround inside `#ifdef CXX_PYTHON_RULES_HAVE_BOOST_SERIALIZATION`. Do the
same for the Boost includes and the `weak_ptr` `save`/`load` in `weak_ptr_serialize_support.h`. Test: include
`serialize_shared_ptr.h` a second time at the top of `serialize_function_test.cc`, after
`skeleton_serialize_support.h`, with a comment saying why; `build-noserial` still builds.

**F1.3 XML archive strings in python (§4.3; D6, D7).** In `lib/serialize_function_nb.cc`: `serialize_write_string`
returns `nb::bytes`; `serialize_read_generic_string` takes an `nb::handle` and accepts `bytes`, or `str` encoded as
UTF-8 with `surrogateescape`. Update the docstring; the generated `.pyi` then shows `bytes`. Tests: the F0.3 round
trip passes; a pure C++ object's XML decodes as UTF-8 text; `str` input still works.

**F1.4 Type registry key and lookup (§4.4).** In `cxx_python_rules/nanobind/type_registry.h`:
- Key `"__cxx_python_rules_nb_type_registry_v2_" <namespace> "__"`, with the namespace from
  `BOOST_PP_STRINGIZE(CXX_PYTHON_RULES_NAMESPACE)`.
- `Map` becomes `std::unordered_map<std::string, Entry, NameHash, std::equal_to<>>`, where `NameHash` hashes a
  `std::string_view` and has `is_transparent`; `find` looks up `std::string_view(T.name())`, with no allocation.
- In passing (D9): initialize the registry pointer through a function-local static (`static Map* m = find_or_create();`),
  which C++ initializes exactly once even with several threads, instead of the unguarded `if(!m)`.
- Update the header comment (one registry per project, shared by that project's modules).

Tests: `_is_registered` and `downstream_test.py` pass; a python test that `builtins` has the key with `Skeleton` in
it. Optional: time a million `shared_ptr<GenericObject>` returns to python before and after, and record the numbers.

## F2. Tooling (review §6.2, §6.4; D11)

**F2.1 Sanitizers as a `configure` option.** `--enable-sanitizers[=address,undefined]` (default when given without a
value: both) in `config/m4_local/skeleton_common.m4`: adds `-fsanitize=… -fno-omit-frame-pointer` to the compile and
link flags (checked with `AX_CHECK_COMPILE_FLAG` and a link test). It encodes the S0.8 recipe:
- `script/lsan.supp`, the suppression for the accepted leak (ST finding B, `leak:directory_restored`, with
  `fast_unwind_on_malloc=0`).
- `make check` sets `ASAN_OPTIONS`, `LSAN_OPTIONS=suppressions=…` and `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`,
  and, for the python tests, preloads `$(CXX -print-file-name=libasan.so)` with leak detection off (python itself
  doesn't free everything at exit).
- frenchpeas: configure in an environment with conda's compiler activation (`build-rpath`), as the "ASan on
  frenchpeas" note in `implementation_plan.md` says. Write that in CLAUDE.md. (`pixi run` activates the compilers,
  so the future CI won't have this problem.)

Then reconfigure `build-asan` with `--enable-sanitizers` (address and undefined) and run `make check`. UBSan hasn't
been run before: fix findings in our code; findings in Blitz, Boost or nanobind go into `script/ubsan.supp`, one line
each with the reason.

**F2.2 libstdc++ assertions in debug builds.** `--enable-debug` adds `-D_GLIBCXX_ASSERTIONS` to the project's
`CPPFLAGS` (`skeleton_common.m4`, through `AM_CPPFLAGS` in `common.am`); CMake adds it for the Debug configuration
(F8). It doesn't change the ABI, so it can be mixed with libraries built without it. Run `make check` in a debug build
and fix anything it catches.

**F2.3 A clang build.** New scratch build `build-clang`: clang 22 with conda gcc 15's libstdc++, using the flags
`script/run_clang_tidy.sh` already passes (`--gcc-install-dir`, `--sysroot`). `make check`, 0 warnings. Fix what clang
finds; a flag clang doesn't know is already dropped by the `AX_CHECK_COMPILE_FLAG` loop. `env/linux/pixi.toml` has
only `clang-22` (on purpose, to avoid the newer binutils the `clang` package pulls in), so the compiler driver comes
from the scratch build's own `pixi.toml` (it is disposable; the scratch `build` environment already has `clang`).

**F2.4 `make golden-check` and `script/check_all.sh`.**
- Move the golden archive comparison from the scratch `baseline/check_golden.sh` into the repository as a
  `make golden-check` target: run `golden_archive_generator` into a temporary directory and compare each file it
  writes with `unit_test_data/golden/`.
- `script/check_all.sh <build directory>…` (so it works on rifle, frenchpeas and later the CI runner): build, fail
  on any `warning:` in the build log, `make check`, `make golden-check`, `make format-check`, `make lint`; print one
  summary table; exit non-zero if anything failed. F10 uses it, and so will the deferred CI.

**F2.5 CI: deferred** (maintainer, 2026-11-07). It is easier to set up at JPL, with access to the runner and to
GitHub's settings. The notes for it are in "Deferred: CI on the JPL runner" near the end of this plan. Until then,
`script/check_all.sh` (F2.4) run by hand does the same checks.

## F3. Small fixes (review §4.6, §4.7, §8)

1. **Boost minimum.** `configure.ac:47`: `AC_BOOST(required, 1.84.0)`; `./bootstrap`; check that configure prints
   `checking for boostlib >= 1.84.0`.
2. **Global fixture message.** Rewrite the error text in `global_fixture_default.cc.in:19-23` so it says to set
   `abs_top_srcdir` when running outside `make check` (e.g. in a debugger).
3. **`NumpySample`.** Add `print` ("NumpySample" and the shape of `data()`) and `cxx_python_rules_nb::printable(c)` in
   `numpy_sample_nb.cc`. Test: `str()` in `numpy_sample_test.py`.
4. **`CacheExample::load`.** Throw `Exception("CacheExample: the archive has no observed object")` if `observed_` is
   null. No test (it needs a hand-damaged archive).
5. **`RasterSample::read` and `read_with_pad`.** Call `check_region` before allocating in `read`; in `read_with_pad`,
   throw for negative sizes before allocating. Tests: negative sizes in `raster_sample_test.cc` and
   `raster_sample_test.py`.
6. **Unsupported element types.** In `blitz_mdspan.h`, a trait `blitz_owner_supported<T>` listing the element types
   instantiated in `blitz_mdspan.cc`, and a `static_assert` in `to_blitz` that names the list and the `.cc` file to
   extend; a comment in both places to keep them in step, and a C++ test that calls `to_blitz` with an owner for every
   listed type (a missing instantiation then fails to link the test). In `numpy_dtype_name`, a `static_assert` for
   arithmetic types, so `std::complex` can't map to `"uint64"`.
7. **Deprecated Boost.Test header.** `unit_test_support.h:5`: include `boost/test/tools/floating_point_comparison.hpp`
   (or nothing; check that the tests still compile).
8. **Includes.** Remove `<iostream>` from `weak_ptr_serialize_support.cc` and the second `generic_object.h` include
   from `cxx_python_rules_nb.h`.
9. **File error messages.** In `serialize_function.cc`, set `errno = 0` before opening; on failure append
   `std::generic_category().message(errno)` only if `errno` is set. (`strerror` isn't thread safe, and `std::ofstream`
   doesn't promise to set `errno`.)
10. **Python.** `swig_shelve.py:42` and its `.sample`: `(int, float, bytes, str)`. `_nanobind_support.py`: rename the
   `dir` parameters to `directory` (`_new_from_serialization_dir` is called positionally by pickle, so old pickles
   still load; its only other caller, `swig_shelve.py`, passes the argument positionally).

## F4. Names of the in-memory serialization functions (review §4.5, D8)

1. **C++** (`serialize_function.{h,cc}`): rename the three functions as in D8 and delete the old ones, including the
   stubs in the `--without-boost-serialization` branch. Take the data as `std::string_view` and read it through
   `std::ispanstream` (C++23 `<spanstream>`), which avoids copying the archive into an `istringstream`; a
   `std::string` still converts. The template `serialize_read_binary_string<T>` calls the new function.
2. **Python** (`lib/serialize_function_nb.cc`): bind the new names only. `serialize_write_binary` keeps just its
   `(Fname, Obj)` file form; there is no `serialize_read_binary` any more. While here, take file names as
   `std::filesystem::path` with `nanobind/stl/filesystem.h`, so `pathlib.Path` works as well as `str`. Switch
   `_nanobind_support.py` to the new names. Pickles refer to `_nanobind_support._new_from_serialization`, not to these
   names, so existing pickles and shelves still load.
3. **Callers:** switch every use in `lib/`, `cxx_python_rules/`, `python/`, the tests and the `.sample` files.
4. **Checks:** the API snapshot differs from F0 only by the renames (and the `bytes` of F1.3); a test that the old
   python names are gone and that `serialize_read_binary_generic_string` round trips with and without `Base_dir`; a
   hand-compiled `serialize_read_binary("x.bin")` without `<T>` fails to compile.

## F5. `Exception` and class design (review §5.2, §5.3)

**F5.1 Copying an `Exception` can't throw (§5.2).** Keep the message, source location, trace and stream state in a
`std::shared_ptr<Data>`. `what()` returns the shared message. `operator<<` copies `Data` first if it is shared (copy
on write); it is used before the throw, when the exception has one owner. Declare the copy constructor and copy
assignment `noexcept = default`; declaring them stops the compiler from adding move operations, so a move copies the
pointer and `Data` is never null. Copies can then be used on any thread (D9). Tests:
`static_assert(std::is_nothrow_copy_constructible_v<Exception> && std::is_nothrow_copy_assignable_v<Exception>)`;
appending to a copy leaves the original unchanged; the existing exception tests and `printing_test.py` pass. Note for
geocal: an exception class derived from `Exception` that adds its own `std::string` member loses the guarantee; say so
in the class comment.

**F5.2 `GenericObject` special members (§5.3).** Keep the public default constructor and the virtual destructor;
declare copy and move constructors and assignments `protected` and `= default` (C.67). Build everything: library,
bindings (nanobind no longer sees `GenericObject` as copyable, which is right), tests. Tests: `static_assert` that
`GenericObject` isn't copy constructible from outside and that `ValueSample` is. Mirror into `generic_object.h.sample`.
**Trial build in geocal**, read-only for geocal (a scratch copy): apply the same change and build; record every place
that slices, as R1.2 did for the observers.

**F5.3 Out-parameters (§5.3).** Keep `ArrayTypemapSample::split` and `scaled`: they show how to bind geocal's output
arguments (returned as a tuple, `array_typemap_sample_nb.cc:55-72`). Rewrite the comment at
`array_typemap_sample.h:135` to say that, without SWIG's `OUTPUT` names.

**F5.4 `double` by value.** `CxxPythonRulesSample::get_scale(double)` and `weight(double)`, and the trampoline's
`weight` override. The API snapshot must not change.

**F5.5 `[[nodiscard]]`.** On the `serialize_read*` functions, `RasterSample::read`/`read_with_pad`,
`resolve_archive_path`, `Exception::stacktrace`, `GenericObjectMap::get`/`get_generic`/`contains`. Tests that call
one only to check that it throws (inside `BOOST_CHECK_THROW`) need `std::ignore = …`, or the build warns.

**F5.6 Lookups without building a `std::string`.** `GenericObjectMap::map_type` uses `std::less<>`; `get_generic`,
`get`, `contains` and `erase` take `std::string_view` (`operator[]` keeps `const std::string&`; heterogeneous
`operator[]` is C++26). `docstring_table()` becomes `std::map<std::string, const char*, std::less<>>`: change the
declaration in `cxx_python_rules_nb.h` and the generator (`doxy2nanobind.py:127-129`) together, and let `doc()` look up
a `std::string_view`. Checks: golden archives identical (the map's comparator isn't in the archive); docstrings in the
API snapshot unchanged.

**F5.7 `FileReferenceSample` message.** Report the path it tried to open (after `resolve_archive_path`) as well as
the name.

**F5.8 `final` on helper classes.** `ArchiveBaseDirectory`, `PtrSerializeScope`, `OutputFile`, `InputFile`,
`OwnerMemoryBlock`, `py_streambuf`, `py_ostream`, `py_istream`.

## F6. Code-level items, clang-tidy and warnings (review §5.4, §6.1, §6.3)

1. Nested namespace definitions (`namespace boost::serialization {`) in the 6 places clang-tidy lists.
2. `std::array` for the 8 C arrays (`buf.data()`, `buf.size()` for `fgets` and `read`; `shape.data()` for
   `nb::ndarray`).
3. Remove the redundant `inline` (`skeleton_exception.h:108`, the templates in `serialize_function.h`) and the 7
   redundant `typename`s.
4. Move `generic_object_list()` and `generic_object_index()` to `generic_object_sample.h`. Move `director_hook()` to
   a new `cxx_python_rules/lib/director_hook.cc`, listed in `cxx_python_rules/cxx_python_rules.am` (CMake globs it).
5. `iostream_swig.h`: `<iosfwd>` instead of `<iostream>` (the `.cc` includes what it needs).
6. `golden_archive_generator.cc`: an anonymous namespace instead of 49 `static` functions.
7. `GlobalFixture`: remove the virtual destructor (Boost.Test derives from the fixture but never deletes through it);
   `const char* srcdir = std::getenv(…)`.
8. **clang-tidy.** Add to `.clang-tidy`: `modernize-concat-nested-namespaces`, `modernize-avoid-c-arrays`,
   `readability-redundant-typename`, `readability-redundant-inline-specifier`, `performance-*` (fix the 8 findings),
   `misc-use-anonymous-namespace`, `modernize-use-designated-initializers`, and
   `cppcoreguidelines-special-member-functions` with `AllowSoleDefaultDtor` and
   `AllowMissingMoveFunctionsWhenCopyIsDeleted` set to true (after F5.2). `make lint` clean. **`.clang-tidy` is shared
   with geocal:** geocal's `make lint` fails on a new check until geocal is cleaned for it, so geocal keeps its own
   list until then. Say so in the file's comment and in the notes for geocal.
9. **Warnings.** Try `-Wnull-dereference`, `-Wduplicated-cond`, `-Wduplicated-branches`, `-Wlogical-op`,
   `-Wcast-qual`, `-Wformat=2`, `-Wextra-semi` and `-Wdeprecated-copy-dtor` one at a time in all builds (gcc 15,
   gcc 14, clang). Add each flag that is clean or made clean, in `skeleton_common.m4` and `CMakeLists.txt`; write
   down the reason for any left out. `script/check_all.sh` (and later CI, with `--enable-werror`) keeps them clean.

## F7. Thread safety (review §5.1; D9, D13)

**F7.1 Write it down (required).** One "Thread safety:" line in the class comment of each class in `lib/` and
`cxx_python_rules/lib/`, and a "Threads" section in `cxx_python_rules/README.md`. Content:
- `Observer`, `Observable`, `CacheInvalidatedObserver` and classes derived from them: not thread safe; register,
  notify and destroy observers on one thread; an observer must not be destroyed while it is being notified.
- `MemoryRasterSample` and `SubRasterSample` of one: concurrent reads are safe. `BufferRasterSample`,
  `CalcRasterSample`: not thread safe.
- `serialize_write*` and `serialize_read*`: safe on different threads (separate mark sets after F1.1, per-thread
  archive base directory).
- `Exception`: copies can be used on any thread (F5.1).
- The binding code in `cxx_python_rules/nanobind/`: needs the GIL today.
- The README section also keeps the F7.3 list, so the free-threading work has a starting point.

**F7.2 In passing (D9, best effort).** Changes that move toward free-threaded python without being required. Each is
done only if it stays simple; if one doesn't, skip it, note why in the progress log, and move it to F7.3.
- Already in other steps: per-thread mark sets (F1.1); the registry's one-time initialization (F1.4); nothrow
  `Exception` copies (F5.1).
- `pending_director()` (`cxx_python_rules_nb.cc:8-12`) per thread: a `thread_local` pointer to a map that is never
  freed, as the current map is never freed. Correct because a python subclass object read from an archive is created
  (`director_load`) and claimed (`director_hook_func`) on the same thread during one read.
- `Observable::notify_update_do` stops changing the observer lists: it already iterates a copy, so it skips expired
  entries instead of erasing them, and the lists lose `mutable`. `add_observer_do` and `remove_observer_do` erase
  expired entries instead, and `save` writes only live ones. Golden archives must stay identical.

**F7.2b Observer lists as `std::vector` (D13, required).** In the same edit as the `notify_update_do` change (or on
its own, if that change is skipped): `olist` and `colist` become `std::vector`, the list's `remove_if` becomes
`std::erase_if` (the remove functions, `clean_dead_ptr`), and `merge_loaded` takes vectors.
`observer_serialize_support.h` uses `decltype(olist)` and doesn't change. Checks: the observer C++ and python tests
pass; golden archives identical (`observer_ctor`, `observer_map`, `simple_camera_like` hold observer lists); an
archive written with the old `std::list` code loads (the golden ones are). Add a row for geocal and framework.

**F7.3 Later: the free-threading phase (not in this plan).** Kept here and in the README section:
- Build with nanobind's free-threaded support (CMake `FREE_THREADED`; the matching setting in `nanobind.am`).
- Lock the type registry's map while modules register their types at import.
- A mutex around `Observable`'s lists, and a decision on observers destroyed during a notification on another thread
  (the non-owning `weak_ptr` doesn't keep them alive).
- `CalcRasterSample`'s shared scratch array, `BufferRasterSample::number_copy_`.
- Run the python tests on a free-threaded python (conda-forge `python-freethreading`) in a scratch build, and in CI
  once it exists.

## F8. CMake reference build (review §6.5, D12)

In `CMakeLists.txt`:
1. `CONFIGURE_DEPENDS` on every `file(GLOB …)` (lines 44, 61, 85, 108).
2. `PROJECT_SOURCE_DIR`/`CMAKE_CURRENT_SOURCE_DIR` and `CMAKE_CURRENT_BINARY_DIR` instead of `CMAKE_SOURCE_DIR` and
   `CMAKE_BINARY_DIR`.
3. `target_compile_features(skeleton PUBLIC cxx_std_23)` instead of the global `CMAKE_CXX_STANDARD`.
4. `BOOST_DISABLE_ASSERTS` for configurations other than Debug and `_GLIBCXX_ASSERTIONS` for Debug (generator
   expressions), matching `configure`.
5. Install and export: `install(TARGETS skeleton EXPORT skeletonTargets FILE_SET HEADERS …)`, `install(EXPORT …
   NAMESPACE skeleton::)`, and a `skeletonConfig.cmake` from `configure_package_config_file` with `find_dependency`
   for Boost and Threads and the blitz `pkg-config` lookup, plus `write_basic_package_version_file`. The python module
   isn't installed by CMake here (framework packages its python differently); say so in the file.

Checks: `build-cmake` builds with 0 warnings and `ctest` passes; install into a scratch prefix; a 10-line consumer
project does `find_package(skeleton)`, links `skeleton::skeleton` and runs; a scratch super-project builds the
skeleton with `add_subdirectory`. Keep the consumer project in the repository (e.g. `cmake/consumer_test/`) so it can
be rerun by hand and by the deferred CI.

## F9. Comments and documentation (review §5.6, §5.5, §7; D10)

1. **Comment sweep** in `lib/`, `cxx_python_rules/` (code, `.am`, `.sample`, python), `configure.ac`,
   `config/m4_local/`, `CMakeLists.txt`, `python/skeleton/`, `env/linux/pixi.toml` (it still names
   `lib/skeleton_mdspan.h`, since moved) and the repository's dot files (`.clang-tidy`). Comments say what the code
   does now. Remove phase numbers, review section numbers and `claude_notes/` paths; the history stays in git and in
   `claude_notes/`. Keep the reasons that still apply, phrased for today, e.g. "archives written by the old SWIG
   bindings use this class name". Fix the stale texts listed in review §5.6. Leave `config/m4/` alone (a subtree of
   the shared autoconf repository) and keep `gc_shared_ptr.h`'s fork notes, pointing at
   `cxx_python_rules/nanobind/README.md` instead of `claude_notes/`.
2. **`configure` cleanup:** remove the AFIDS `LD_LIBRARY_PATH` block (`configure.ac:26-42`), say "Skeleton objects" in
   the `--without-boost-serialization` help text, remove `CYTHON_SRC` from `common.am`; `./bootstrap`.
3. **Trusted input.** One paragraph in `cxx_python_rules/README.md` and in the `read_shelve` docstring: archives and
   shelves contain pickles and Boost.Serialization doesn't validate its input, so loading one can run arbitrary code;
   load only files you trust.
4. **nanobind version check.** `static_assert(NB_VERSION_MAJOR == 3 && NB_VERSION_MINOR == 1, …)` in
   `gc_shared_ptr.h`, with a message saying to compare the fork with nanobind's `stl/shared_ptr.h` and
   `CXX_PYTHON_RULES_NB_OVERRIDE_REF` with nanobind's trampoline code, then update the numbers.
5. **Documents.** CLAUDE.md (`--enable-sanitizers`, `build-clang`, `make golden-check`, `script/check_all.sh`, the
   new function names, the thread safety section, and that CI is planned but deferred); `cxx_python_rules/README.md`; `implementation_plan.md` (fix the
   status line at the top, add the notes for geocal below to §0).

## F10. Final check

Run `script/check_all.sh` over `build`, `build-gcc14`, `build-noserial`, `build-asan` (address and undefined),
`build-clang`; then `make installcheck` in `build`; then the CMake checks from F8. Each must
pass with 0 warnings. Then:

| Review item | Check |
|---|---|
| §4.1 | `serialize_nested_test.py` passes without `xfail`; the concurrent-write test passes; `grep -rn 'ptr_serialize_depth\|recursive_mutex' cxx_python_rules/lib/` finds nothing; framework trial recorded. |
| §4.2 | `grep -L 'pragma once' lib/*.h cxx_python_rules/lib/*.h cxx_python_rules/nanobind/*.h` finds nothing; the double include in `serialize_function_test.cc` compiles; `build-noserial` passes. |
| §4.3 | The XML string round trip test passes; `nb_pyi/_bindings.pyi` shows `bytes` for `serialize_write_string`. |
| §4.4 | `grep -rn 'registry_v1' cxx_python_rules/` finds nothing; the key test passes. |
| §4.5 | `grep -rnw 'serialize_read_binary' lib cxx_python_rules python` finds only the template `serialize_read_binary<T>(path)` and its uses; no one-argument `serialize_write_binary` is left; the old-names test passes. |
| §4.6, §4.7 | The F3 tests pass; configure output shows Boost 1.84. |
| §5.1 | Every class in `lib/*.h` and `cxx_python_rules/lib/*.h` has a "Thread safety:" line; the README has the "Threads" section with the F7.3 list; F7.2 items done or moved to F7.3 with a reason. |
| §5.2, §5.3 | The `static_assert`s compile; geocal trial (F5.2) recorded; `grep -n 'std::list' lib/observer.h` finds nothing (D13). |
| §5.4, §6.3 | `make lint` clean with the new checks; the wide clang-tidy run (`baseline/tidy_wide.sh`) leaves only `readability-avoid-const-params-in-decls`, `modernize-use-auto`, `modernize-return-braced-init-list` and whatever this plan writes down as accepted. |
| §5.6 | `grep -rn -E 'claude_notes\|phase S[0-9]\|S[0-9]\.[0-9]\|[Cc]ode review 20' lib cxx_python_rules configure.ac config/m4_local CMakeLists.txt python/skeleton env .clang-tidy` finds nothing. Use `grep -r`: on a directory without `-r`, GNU grep reports an error and finds nothing, which looks like a pass. |
| §6.1, §6.2, §6.4 | The warning flags added in F6.9 are in the build logs; UBSan ran clean (or each suppression has a reason); `script/check_all.sh` fails when a test is broken on purpose. CI: deferred, not checked here. |
| §6.5 | The F8 checks pass. |
| everything | `make golden-check` passes (104 generator files identical); the API snapshot differs from `baseline/api_f0.txt` only by the intended changes; `make format-check` clean; `ruff format --check` clean. |

Last, a fresh read of the changed files, and the result in the progress log.

## Deferred: CI on the JPL runner (was F2.5)

Set aside by the maintainer on 2026-11-07, to be done at JPL with access to the runner and GitHub. Everything it
needs from this plan is in place by then: `script/check_all.sh` and `make golden-check` (F2.4), `--enable-sanitizers`
(F2.1), the clang build (F2.3), the clean warning flags (F6.9) and the CMake consumer project (F8). What is left:

- **Find out first:** the runner's labels (for `runs-on`), its OS, and whether it can download from conda-forge (for
  `pixi install`; if not, a pre-built environment on the runner). `origin` is
  `github.com/Cartography-jpl/cxx-python-skeleton`, so this is a GitHub Actions workflow,
  `.github/workflows/check.yml`, on the JPL self-hosted runner. If the repository is public, pull requests from forks
  must not run on the self-hosted runner without approval (GitHub's setting "Require approval for all outside
  collaborators", or run only on `push`), because the workflow runs the pull request's code on the runner.
- **Environments:** add pixi environments to `env/linux/pixi.toml` with features: `default` (gcc 15, as now),
  `gcc14` (`gcc`/`gxx` `14.*`), `clang` (the clang 22 driver, kept out of `default`). Cache them between runs, keyed on
  `pixi.lock`.
- **Jobs**, each `./bootstrap`, `configure --enable-werror …`, then `script/check_all.sh`: gcc 15 debug (also
  `make install` and `make installcheck`); gcc 14; `--without-boost-serialization`; `--enable-sanitizers`; clang;
  CMake (`cmake --build`, `ctest`, the F8 consumer project).
- **Triggers:** push to `modernize` and `master`, pull requests (subject to the fork rule above), and manual runs
  (`workflow_dispatch`).
- **Check:** a run is green; then break a test on purpose in a scratch branch and see the run fail.
- Later, with free-threading (F7.3): a job on a free-threaded python.
- geocal can use the same workflow shape on the same runner.

## Notes for geocal (to copy into `implementation_plan.md` §0)

| Step | What geocal does |
|---|---|
| F1.1 to F1.4 | Copy the changed `cxx_python_rules/` files (`weak_ptr_serialize_support.{h,cc}`, `serialize_function.cc`, `serialize_shared_ptr.h`, `type_registry.h`, `serialize_function_nb.cc`'s two string functions). Check its three tests that write archives directly (F1.1). |
| F2 | Same `--enable-sanitizers` option and `_GLIBCXX_ASSERTIONS` in geocal's own m4; run UBSan once; a `make golden-check` and `script/check_all.sh` of its own. CI later, with the skeleton's deferred workflow as the model. |
| F3.6 to F3.9 | Same edits in its copies. |
| F4 | When it copies `cxx_python_rules/`: switch its callers of the old names (`mars_igc_test.py`; its pickling helpers become `_nanobind_support`'s when it converts). The old names are gone. |
| F5.1, F5.2 | Same `Exception` and `GenericObject` changes; the F5.2 trial lists what slices. |
| F6.8 | Turn on each new clang-tidy check only after cleaning geocal for it. |
| F7 | The same "Thread safety:" lines for its copies of the shared classes; the F7.2 changes come with the copied files. |
| F9.1 | Copy `cxx_python_rules/` again after the comment sweep. |
| F7.2b (D13) | Observer lists as `std::vector`, in geocal's and framework's `observer.h`, whenever they modernize their copy; archive-compatible. |

## Progress log

| Phase / step | State | Notes |
|---|---|---|
| Plan written | done | 2026-11-07. The review was corrected at the same time: an ASan build exists (§6.2), and the helper template of §5.5 can't work. |
| Decisions D6 to D12 | done | 2026-11-07 (maintainer): D6 `bytes`; D7 no change to the XML format; D8 rename and drop the old names now; D9 free-threading is a long-term goal: document thread safety, do the rest in passing, best effort; D10 sweep comments in this plan; D11 CI on the JPL runner, then deferred (next row); D12 fix CMake now. |
| F2.5 CI | deferred | 2026-11-07 (maintainer): to be set up at JPL, with access to the runner and GitHub. Notes in "Deferred: CI on the JPL runner". |
| D13 | done | 2026-11-07 (maintainer): A, observer lists become `std::vector` (F7.2b). Archive check: `std::list` and `std::vector` archives are identical. |
| F0 baseline | done | 2026-11-07, `ddd17c1`. `build` (gcc 15) and `build-gcc14`: 217 passed, 1 xfailed, 0 warnings; golden 104 files identical; `baseline/api_f0.txt`, `baseline/tidy_wide.sh` (154 findings saved). New strict-xfail tests reproduced §4.1 (3 cases) and §4.3. |
| F1 bugs | done | `a79bc7c` (F1.1 per-write mark sets, `PtrSerializeScope`; F1.2 include guard), `6faec9c` (F1.3 bytes; F1.4 registry key `..._v2_Skeleton__`, heterogeneous lookup). New concurrent-write C++ test. **Framework trial not possible here**: `framework-build` is empty and framework needs the muses conda environment and ABSCO data; left for the framework phase. |
| F2 tooling | done | `2e2149a`. `--enable-sanitizers` adds the flags after `AC_ENABLE_DEBUG` (it used to replace a hand-given `CXXFLAGS`, so the old `build-asan` wasn't compiled with ASan at all). libtool links shared libraries with `-nostdlib`, so python preloads both runtimes, the stub generator too. UBSan: no findings. Found on the way: **pytest's `./pytest_tmp` (pytest.ini) was in the source tree, shared by every build directory's `make check`** (two builds tested at once broke each other); now in the build directory. clang 22 (`build-clang`, `configure_clang.sh`, `-Qunused-arguments` for libtool's `-pthread`) found `-Wundefined-var-template` (`BlockMember`; now one type list, `CXX_PYTHON_RULES_BLITZ_OWNER_TYPES`, which also did F3.6) and a `BOOST_DISABLE_ASSERTS` redefinition in release builds. F2.5 CI deferred. |
| F3 small fixes | done | `56983d9`. |
| F4 names | done | `438393e`. Also python file names accept `os.PathLike`; in-memory reads use `std::ispanstream`. |
| F5 class design | done | `b972530` (F5.1, F5.2: `GenericObject` has protected copy and **no** move operations, since a defaulted move assignment in a virtual base gives `-Wvirtual-move-assign`; geocal trial: all 624 files of its compile database syntax-checked in a scratch copy with the change, nothing slices), `3f49d68` (F5.3–F5.8; `check_all.sh` counts warnings in `make check` too, where the tests are compiled). |
| F6 code, lint, warnings | done | `7b488ba`. clang-tidy `--fix` expanded `CXX_PYTHON_RULES_NAMESPACE` to `Skeleton` when merging namespaces; put back by hand. `-Wextra-semi` left out (macro style). Boost.Test's `__COUNTER__` gives clang 22's `-Wc2y-extensions`, turned off for the tests only. |
| F7 thread safety | done | `87d41c9`. F7.2's `notify_update_do` change was skipped (best effort): archives hold empty entries for observers outside them, so pruning or writing only live entries changes the bytes of rewritten archives (`golden_archive/rewrite_identical`). On the README's list. `pending_director()` per thread. D13 done (`std::vector`). |
| F8 CMake | done | `5edea0a`. `-Wnull-dereference` taken back out: the F6.9 trial used `-fsyntax-only`, which never runs it; optimized builds give false positives in libstdc++'s `shared_ptr`. ctest 3/3 (C++, python, consumer of the installed package); `add_subdirectory` super-project builds. |
| F9 comments, docs | done | `56d00cf`. |
| Docstring table (maintainer question) | done | `4bd9741`. On geocal's headers the generated `nb_docstrings.cc` took 3 min 24 s to compile at `-O2` (a 7278-entry `std::map` initializer list); now constant data, 0.6 s. A stamp file keeps it from being replaced, and so recompiled, unless its contents change. Per-class files not needed (doxygen over all of geocal: 2.3 s). Run time, geocal-size table: filling the map 2–3 ms once at import, all lookups 1–2 ms; `std::unordered_map` would save about 1 ms of each, not worth a change. |
| F10 final check | done | Clean builds through `script/check_all.sh`: `build` (gcc 15), `build-gcc14`, `build-noserial`, `build-asan` (ASan+UBSan), `build-clang`: all pass, 0 warnings, golden identical (229 passed, 1 xfailed); `format-check`, `lint` clean; `build-cmake` 0 warnings, ctest 3/3. `check_all.sh` fails on a deliberately broken test. Wide clang-tidy: what is left is the generated `nb_docstrings.cc`, the Boost `const unsigned int version` idiom, braced-return/auto (taste), and deliberate cases (the sample's non-const `counter()`, `IostreamSwig::read`, test literals). API vs `api_f0.txt`: only the intended changes and edited docstrings. `make installcheck` not rerun after F10's clean builds (was green at R9; `make check` covers the same tests). |
