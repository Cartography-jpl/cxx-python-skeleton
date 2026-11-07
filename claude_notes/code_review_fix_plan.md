# Plan: fix everything found in the 2026-11-06 code review

**Date:** 2026-11-06
**Status:** Not started. Code work waits for the maintainer's "go".
**Input:** `claude_notes/code_review_2026-11-06.md` (referred to below as "the review"; its section numbers are given as
§2.1 and so on).
**Goal:** when every step is done, none of the problems in the review remain. That covers the bugs, the older idioms,
and the generated-looking bindings. The python API, the archive formats and the test results stay the same, apart from
the changes listed in "Intended behavior changes" below.

Rules (the same as `implementation_plan.md` §0): work on `modernize`; stage with `git add`, never commit; run
`clang-format -i` on C++ you change and `ruff format` on python you change; a step is done when its checks pass and
the full test suite passes. After each phase, add a line to the progress log at the end of this file.

## Intended behavior changes

Everything else must stay the same, which the R0 snapshot checks.

| Change | Step | Who notices |
|---|---|---|
| Serializing a column-major or strided blitz array writes the values correctly (it used to scramble them or throw) | R1.1 | Nobody, except that data now comes back right. Archives of row-major arrays are byte for byte the same. |
| Serializing a blitz array with a nonzero base throws, instead of silently dropping the base (decision D1) | R1.1 | Any code that serializes such arrays; geocal has to check its uses. |
| Copying an `Observer`, `CacheInvalidatedObserver` or `Observable` (or a class derived from one) no longer compiles (D2) | R1.2 | C++ code that copies these. The R1.2 trial build finds it in framework and geocal. |
| An out-of-range index from python raises `IndexError`/`RuntimeError` instead of crashing | R1.4 | Python callers. |
| `range_check` messages give the caller's file and line; the old message text is kept | R1.8 | Nobody. The message has the same form. |
| `DirectorExample` no longer prints to stderr | R1.9 | Anyone reading test output. |
| Some python keyword argument names change to match the C++ parameter names (`DirectorExample(V=, Name=)` → `value_to_add=, name=`; `arg0` → the real name) | R6 | Callers that pass these by keyword; none of the tests do. |
| Loading an archive no longer changes the process's current directory; relative file names inside archives are resolved through a per-thread base directory instead | R4.1 | C++ classes that open files in `load()` have to use the new resolver (geocal's `AirMspiIgc`). |
| Errors from `lib/` code are `Skeleton::Exception` instead of `std::runtime_error` | R2.6 | Python sees `RuntimeError` either way; the exception now carries a stack trace. |
| A loaded blitz array is always row-major with base 0, whatever the target array was before | R1.1 | Code that loads into a column-major member; it used to get scrambled values. |
| Passing `None` where C++ needs an object raises `TypeError` (or an `Exception`) instead of crashing | R1.4, R6.3 | Python callers. |
| `ThreadExample.run` with `Number_thread < 1` or `None` raises | R1.6 | Python callers. |
| A read-only numpy array passed to a C++ function that writes into its argument raises `TypeError` instead of being changed | R5.3 | Python callers that pass read-only arrays to such functions. |
| Python 3.12 is the minimum | R5.1 | Builds with older python fail at configure time with a clear message. |

## Decisions needed before starting

Each has a recommendation. Ask the maintainer about any that aren't settled. All five (D1–D5) were settled
on 2026-11-06.

- **D1. Blitz arrays with a nonzero base.** **Decided (maintainer, 2026-11-06): throw on save.** Catching a silent
  error matters more than supporting the case; if a real use turns up, record the base then (new format, version
  flag, old format still readable).
- **D2. Copying observers.** **Decided (maintainer, 2026-11-06): delete copying** (option B) in `Observer<T>`,
  `CacheInvalidatedObserver` and `Observable<T>`, and see how much breaks. Any copy becomes a compile error, so no
  copy can silently lose its registration. The breakage shows where copies are really used, and that informs what the
  semantics should be. The alternative, copies that start unregistered (option A, like Boost.Signals2's `trackable`),
  stays open if B turns out to cost too much; see R1.2.
- **D3. Include guards.** **Decided (maintainer, 2026-11-06): `#pragma once`** in every header. It isn't standard
  C++ (no standard, C++23 included, defines it), but gcc, clang and MSVC all support it, and that is good enough here.
- **D4. Where the generic mdspan files live.** **Decided (maintainer, 2026-11-06): move** `blitz_mdspan.{h,cc}`
  and `skeleton_mdspan.h` into `cxx_python_rules/lib/`.
- **D5. Binding style.** **Decided (maintainer, 2026-11-06): ordinary nanobind plus a small `docs` helper.** The
  helper is made once per class (`doc()`, `doc("member")`). Bindings use member pointers, `nb::overload_cast` and
  `"A"_a` names, and lambdas only in the R6.2 cases. No wrapper around `nb::class_`. A test catches a forgotten
  `register_type`, the one advantage a wrapper would have had (R6.6). The second helper discussed, a
  `method<T>(&Base::f)` adapter for methods of non-nanobind bases, turned out to be unnecessary: nanobind already
  handles them (R6.1 item 2).

## Order and dependencies

Work through the phases in order: R0 → R1 → R2 → R3 → R4 → R5 → R6 → R7 → R8 → R9. Within a phase, the steps are
independent unless noted here:
- R1.8 (`range_check` as functions) comes before R1.4, which uses it for the blitz index checks.
- R1.1 comes before R3.2: fix first, so the fix is small enough for geocal to copy, then make it rank-generic.
- R1.3 (moving files) comes before R2.10 (include guards), so the moved headers are converted once.
- R2 comes before R6: the `explicit` and sink-parameter changes change constructor signatures the bindings use.
- Each R7.2 clang-tidy check is turned on as soon as the R2 step that cleans up for it is done, not all at the end.
- R6.5 (generator) comes after R6.3, so the reworked files are the target the generator is checked against.
Only C++23 library features that gcc 14 has are used: `std::ranges::to` and `fold_left` (gcc 14 and 13),
`std::jthread`, `std::format` and `std::source_location` are all fine.

## R0. Baseline (before any change)

1. Build `build`, `build-gcc14`, `build-noserial` and `build-asan` under `/data2/smythdata/cxx-python-skeleton-build/`
   (rifle: the matching `build-pixi*` directories), and record that `make check` passes in each. In the ASan build,
   note the one known leak (ST finding B) so it isn't mistaken for a new one.
2. **Snapshot of the python API**: add `script/api_snapshot.py`. It imports `skeleton_cxx._bindings` (and
   `_skeleton_downstream`) and writes, sorted: every class and function, its `__nb_signature__` (or
   `inspect.signature`), its `__doc__`, and its properties. Save the output and the generated `nb_pyi/_bindings.pyi` as
   the baseline. After each phase the snapshot is compared with the baseline, and every difference has to be one of
   the intended changes above. geocal can use the same script.
3. **Archive check**: run `golden_archive_generator` into a temporary directory and `cmp` each file it writes with
   the same file in `unit_test_data/golden/`. Compare only the generator's files (104 today, all identical). The
   golden directory also holds `director_python.xml`, `director_python_user.xml` (written by the SWIG build in S5)
   and `README.md`, which the generator doesn't write. The comparison must stay identical through every phase,
   except for files a step adds on purpose.
4. Turn the programs that reproduced the serialization bugs into unit tests in
   `blitz_array_serialize_support_test.cc`, marked as expected failures, so R1.1 can show them going from failing to
   passing: a column-major array saved and loaded (review §2.1), a reversed array (`A.reverse(0)`) saved and loaded,
   and a row-major archive loaded into a column-major target. The Observer copy (§2.2) doesn't get a runtime test:
   after R1.2 the copy doesn't compile, and a `static_assert` checks that.

## R1. Bugs (review §2)

**R1.1 Blitz array serialization: column-major, reversed, strided, and the load side (§2.1).** Three bugs, all
confirmed with test programs:
- **save, column-major:** written in memory order and read back row-major (values scrambled).
- **save, reversed** (a negative stride, e.g. `A.reverse(0)`): `isStorageContiguous()` is true, but `A.data()`
  points at the *last* element in memory, so `make_array(A.data(), A.size())` reads past the end of the block (an
  out-of-bounds heap read; a reversed `[1 2 3]` came back as `[3 0 2.4e-322]`).
- **load into a non-row-major target:** `A.resize(...)` keeps the target's storage order and base, so loading a
  row-major archive into a column-major member (e.g. a class whose default constructor makes a `ColumnMajorArray`)
  scrambles the values (`[0 1 2; 10 11 12]` loaded as `[0 2 11; 1 10 12]`).

Fix, in each rank's `save`: write directly only if the array is row-major, ascending and contiguous
(`isStorageContiguous()`, `isRankStoredAscending(i)` for every `i`, and `ordering(i) == N-1-i`); otherwise copy it
into a row-major temporary (`blitz::Array<T,N> tmp(A.shape()); tmp = A;`) and write that. Remove the "only
contiguous" error, since strided arrays now work too. Per D1, `throw Skeleton::Exception` naming the base if any
`lbound(i) != 0`. In each `load`: read into a fresh row-major, base-0 array and then `A.reference(tmp)`, so the
target's old storage order and base don't matter. A loaded array is therefore always row-major; write that in the
header comment (a class that needs column order copies after loading). Tests: the three R0.4 tests, column-major
ranks 2 and 3, a strided view (`A(Range::all(), Range(0, toEnd, 2))`), and a nonzero base (throws), through both
XML and binary archives. The golden archives must still be identical. Remove the "wouldn't round trip" remark in
`array_sample.h`, and make the `ColumnMajorSample` comment say it is no longer serialized only because the sample
doesn't need it. Mirror the change in `cxx_python_rules/lib/blitz_array_serialize_support.cc.sample`.

**R1.2 Copying an Observer (§2.2).** Per D2, delete the copy constructor and copy assignment of `Observer<T>`,
`CacheInvalidatedObserver` and `Observable<T>`, with a comment explaining why (copying `this_obj` makes the copy
register as the original). Deleting the copy also removes the implicit move, which is intended: a move would have the
same problem. `Observable<T>` declares no constructor today, so it also needs `Observable() = default;`: declaring
the deleted copy removes the implicit default constructor, and without it `ObservableExample`, `CameraLike`, the
golden generator and the tests stop compiling. With that added, every `lib/*.cc` (library, bindings, tests) still
compiles, so in the skeleton D2 costs nothing more (checked by the independent review). Derived classes become
non-copyable too. A derived class that needs copies has to write its own copy constructor, which calls the base's
default constructor and decides explicitly whether the copy registers.
- **In the skeleton:** add compile-time checks in `observer_test.cc`:
  `static_assert(!std::is_copy_constructible_v<X> && !std::is_move_constructible_v<X>)` for `ObserverExample`,
  `CacheExample` and `ObservableExample`. (There is no runtime copy test to replace.)
- **Trial build in framework and geocal** (read-only for those repositories: work in a scratch copy, or as a patch on
  a scratch branch that is thrown away afterwards). Apply the same deletion (and the `Observable() = default;`) to
  their `observer.h` and build. Record every error in a new section of this plan: which class, where the copy happens
  (returned by value, a container of values, `clone()`, ...), and whether the copy was meant to be registered.
- **Then decide:** if the breakage is small, keep B and give the classes that need it explicit copy constructors (a
  note for geocal/framework). If it is large, or a pattern turns up where copies must keep working, go back to D2 with
  the list. Option A, or B with a copy-and-register helper, are the candidates.
Python is unaffected (nanobind holds these classes by `shared_ptr` and doesn't copy them); run
`observer_example_test.py` anyway.

**R1.3 `Skeleton::` in the rules library (§2.3).** Per D4: move `lib/blitz_mdspan.{h,cc}` to
`cxx_python_rules/lib/`, and `lib/skeleton_mdspan.h` to `cxx_python_rules/lib/cxx_python_rules_mdspan.h`, with
`namespace CXX_PYTHON_RULES_NAMESPACE { namespace md = Kokkos; }`. Put everything in `blitz_mdspan.h` in
`CXX_PYTHON_RULES_NAMESPACE`, and rename `SKELETON_BLOCK_MEMBER` in `blitz_mdspan.cc` to
`CXX_PYTHON_RULES_BLOCK_MEMBER`. Change `blitz_array.h:96-100` to `CXX_PYTHON_RULES_NAMESPACE::`. `blitz_mdspan.cc`
is compiled into the library with the other rules `.cc` files (`cxx_python_rules.am`). The tests
(`blitz_mdspan_test.cc`) stay in `lib/`. `lib/` code keeps writing `Skeleton::md`, since the namespace macro is
`Skeleton` here.

**Namespace plumbing for R1.3.** Today `-DCXX_PYTHON_RULES_NAMESPACE=Skeleton` is set only for the library, the
golden generator and the bindings; the C++ tests get it from `unit_test_support.h:4`, which has to be included
first. After the move, the public `raster_sample.h` includes a rules header that needs the macro, so every user of the
installed headers would have to define it, or the code lands in a namespace literally called
`CXX_PYTHON_RULES_NAMESPACE` (link errors on `BlockMember<T>::ptr`). Fix: define it once in a project header that
every rules header reaches (`skeleton_config.h`, through `generic_object.h`: `#ifndef CXX_PYTHON_RULES_NAMESPACE`
/ `#define CXX_PYTHON_RULES_NAMESPACE Skeleton`), drop the `-D` flags and the define in `unit_test_support.h`, and
write in `cxx_python_rules/README.md` that a project defines it in its config header. Add the moved headers to the
installed headers (`skeletoninc_HEADERS` in `cxx_python_rules.am`) and remove them from `lib/lib.am`; they need no
doxygen docstrings (not bound). Update the paths in CLAUDE.md and `cxx_python_rules/README.md`. Check: `grep -rni
skeleton cxx_python_rules --include='*.h' --include='*.cc'` finds nothing (the `.sample` files use `GeoCal` on
purpose), and a small program that includes only installed headers, with no `-D`, compiles and links.

**R1.4 Bad arguments from python crash the interpreter (§2.4, §2.7).** Two kinds:

*Indices.* `DirectorExampleUser::apply_func` uses `value_vec_.at(ind)` and `value_vec2_.at(ind).at(ind2)`;
`generic_object_index` uses `.at(I)`. Blitz element access with indices from python is just as unchecked: blitz only
checks bounds when built with `BZ_DEBUG`. That covers `CxxPythonRulesSample::data_value(I, J)`,
`ArrayTypemapSample::element(A, I, J)` and `ArrayTypemapSample::stored_value(I, J)`; add `range_check(I, 0, rows)`
and `range_check(J, 0, cols)` (after R1.8). Our translator is registered last, so it is tried first and turns
`std::out_of_range` into `RuntimeError` rather than nanobind's `IndexError`; check which one happens, and write it
down in `cxx_python_rules/README.md`.

*`None` for a `shared_ptr` argument* (found by the independent review, confirmed: python exits with a segmentation
fault). All 29 `shared_ptr` parameters in the bindings are declared `.none()`, so python can pass `None`, and the
C++ code then dereferences a null pointer: `MultiBaseExample.number_band(None)`, `MultiBaseExample.parameter_sum(None)`,
`VirtualDiamondExample.root_name(None)`/`nested_name(None)`/`parameter_sum(None)`, `CacheExample(None)`,
`ThreadExample.run(None, ...)`, `DirectorExampleUser().apply_func(1)` with no value set (`value_` null), and any
others the R9 search finds. Fix both sides: in the bindings (R6.3), keep `.none()` only where C++ accepts a null
pointer (setters that clear, `DirectorExampleWeakPtr`, `GenericObjectMap::set_generic`, ...), so nanobind rejects
`None` with a `TypeError` elsewhere; in C++, functions that dereference a pointer argument or member check it and
throw `Exception` (C++ callers can pass null too). The generator (R6.5) has to stop adding `.none()` everywhere.

Python tests for both: each out-of-range index and each `None` raises a python exception and doesn't crash. Run the
crash cases in a subprocess, so a regression fails one test instead of killing the whole pytest run.

**R1.5 Uninitialized members (§2.6).** Give every scalar member a default initializer. The full list, from a
search of the headers: `DirectorExample::value_to_add_`, `SimpleCameraLike::focal_length_`/`number_band_`,
`GenericObjectA::v_`, `ValueSample::v_`, `CxxPythonRulesSample::count_`/`scale_`, `ObservableExample::value_`,
`IostreamSwig::word_to_read`, `LegacyRasterFile::number_line`/`number_sample`, and
`CacheInvalidatedObserver::cache_valid` (R2.3). Check:
`clang-tidy` with `cppcoreguidelines-pro-type-member-init` reports nothing (it is turned on in R7.2).

**R1.6 `ThreadExample::run` (§2.6).** Use `std::vector<std::jthread>` (joined automatically, including when an
exception is thrown while threads are being created), and `std::scoped_lock`. Declare the `jthread` vector *after*
`res`, `m` and `first_error`, so it is destroyed (and the threads joined) before the things the threads use. Throw
`Exception` if `Number_thread < 1` or `Obj` is null. Sum with `std::ranges::fold_left`. Tests: `Number_thread = 0`
and `Obj = None` raise.

**R1.7 `read_ptr` order (§2.6).** Call `check_region` before building the mdspan in `read_ptr`. (`write_ptr`
already checks first.) Test: negative sizes throw `Exception`.

**R1.8 `range_check` reports the caller's location (§2.6, §3.3).** Replace the four macros and their `*_template`
helpers with function templates `range_check(const T& Val, const std::type_identity_t<T>& Min, const
std::type_identity_t<T>& Max, std::source_location Loc = std::source_location::current())` (and
`range_check_inclusive`, `range_min_check`, `range_max_check`). `T` is deduced from `Val` only, so `range_check(n, 0,
10)` with a `size_t n` works without a cast. The old template deduced one `T` from all three arguments, so this is
no less strict. Don't allow three independent types: that would allow mixed signed/unsigned comparisons. Build the
message with stream output, as now (`Exception e("", Loc); e << "Out of range error in file " << Loc.file_name()
...`), not `std::format`: `std::format` would need a `std::formatter` for `T`, and types that only have `operator<<`
(blitz `TinyVector`, user types) would stop compiling. Both `what()` and `source_location()` then point at the
caller. Keep the message text the same ("Out of range error in file F at line L\nValue: ..."); `skeleton_exception_test.cc`
checks it. geocal has a macro with the same name (implementation plan S7 row): when geocal copies this, its macro
goes. Test: `e.source_location().line()` is the line of the call.

**R1.9 Output to stderr in `DirectorExample` (§2.6).** Remove the `std::cerr` lines. If a test relies on the
messages (check `director_example_test.py` and `DirectorNotes.md`), add a static live-object counter
(`DirectorExample::number_live()`) and test that instead. That also tests lifetimes better than reading stderr. The
counter is a `std::atomic<int>` (`ThreadExample` uses `DirectorExample` from worker threads). Count in every
constructor, including copy and move: R2.4 removes the declared destructor elsewhere, but `DirectorExample` keeps
one (it decrements), so its copy and move have to be written out (or deleted) so a copy is counted.

**R1.10 Broken `compile_commands.json` link (§2.6).** Delete the link and run `make lint` in `build` to recreate it.
It is git-ignored, so nothing is staged. There are also ignored editor backups (`configure~`,
`lib/skeleton_config.h.in~`, `claude_notes/implementation_plan.html~`). List them for the maintainer; delete them only
with the maintainer's OK.

## R2. Simple style updates (review §3)

Each item is mechanical. Where a clang-tidy check exists, fix the code until it is clean, then turn the check on in
`.clang-tidy` (R7.2) so the problem can't come back.

**R2.1 `explicit`** (`google-explicit-constructor`). Every constructor that can be called with one argument, except
copy and move constructors. The review lists them; R9 finds the rest. Compile errors that follow show where C++ code
relied on the implicit conversion; fix those call sites. One must stay implicit: `ExceptionFormat`'s `consteval`
constructor (`skeleton_exception.h:30`), or `throw Exception("fmt {}", x)` stops compiling. Mark it
`// NOLINT(google-explicit-constructor)` with the reason.

**R2.2 Sink parameters** (`modernize-pass-by-value`, `performance-unnecessary-value-param`). Constructors that store
a `shared_ptr` or `std::string` take it by value and `std::move` it into the member. Fix `DirectorExampleNested`'s
`const std::shared_ptr<> Obj`. `add_observer_and_keep_reference` takes `const std::shared_ptr<Observer<T>>&`.

**R2.3 Default member initializers** (`modernize-use-default-member-init`, `cppcoreguidelines-prefer-member-initializer`):
`cache_valid`, `scale_`, `matrix_`, `position_` and others.

**R2.4 Redundant destructors.** Remove `~X() override = default;` wherever it is the only reason the destructor is
declared. Keep the virtual destructor in `GenericObject`, and any destructor that does work (`OwnerMemoryBlock`,
`py_streambuf`). There's no clang-tidy check for this; R9 has the grep.

**R2.5 `Printable` and `print`.** Use one convention: `void print(std::ostream&) const override` when a base already
has `print` (CameraLike → SimpleCameraLike, RasterSample → derived, DirectorExample → Nested), and plain
`virtual void print(...) const` in the first class that declares it (`-Wsuggest-override` and
`modernize-use-override` then check the rest). Extend the `PrintableObject` concept so it requires
`V.print(Os)` as well. Rewrite the class comment (drop Ruby; say `print_to_string` is for python `__str__`). Use
`if(!d.empty())`. Keep the empty-string sentinel for `desc()`, since python overrides return `""`; changing it to
`std::optional` would change the python API for no benefit.

**R2.6 One exception type in `lib/`.** Replace the `std::runtime_error` throws in `lib/` (`ArraySample::matrix`,
`SimpleCameraLike::parameter`, `FileReferenceSample::read`, and the others R9 finds) with `Skeleton::Exception`, using
its format constructor where the message has values in it. In `cxx_python_rules/lib/blitz_mdspan.h` (generic after
R1.3), `std::runtime_error` stays and is documented as the rules library's choice. Also record why `Exception` keeps
deriving from `Printable`/`GenericObject` (it is bound to python and needs `register_type`) and why Boost.Stacktrace
stays (`std::stacktrace` still needs `-lstdc++exp` on gcc 14). Add both to the class comment, so the review's notes on
those points are answered in the code.

**R2.7 `mutable` instead of `const_cast`.** `Observable::olist`/`colist` are `mutable` and `clean_dead_ptr()` is
`const`, so `notify_update_do` calls it directly. `CacheInvalidatedObserver::cache_valid` is `mutable`, and
`CacheExample::value()` loses its `const_cast`. The deliberate `const_cast` in `ArrayTypemapSample::set_value*` and
`to_mdspan`/`MemoryRasterSample` stays, each with a comment (`cppcoreguidelines-pro-type-const-cast` stays off).

**R2.8 Small idioms.** `std::ranges::fold_left` for the sum loops; lambdas instead of `PointerEqual`/`CPointerEqual`,
comparing with `w.lock().get()` only when the pointer isn't expired (or with `owner_before`); `std::scoped_lock`;
`std::uintptr_t` instead of `std::size_t` for addresses; `static_cast` instead of `(int)`; `std::string res;` instead
of `= ""`. In `GenericObjectMap`: `keys()` via `std::ranges::to<std::vector>()`, `get_generic` via `auto`/`contains`
(R3.1 rewrites the class anyway).

**R2.9 `using` instead of `typedef`** (`modernize-use-using`), in the code and in the `SKELETON_EXPORT_OBSERVER_KEY`
macro. **Not** the 37 global `blitz_*_array_*d` typedefs in `blitz_array_serialize_support.h`: the review called them
unused, which was wrong. Each is the argument of a `BOOST_SERIALIZATION_SPLIT_FREE(...)` line, because that macro
can't take a type with a comma in it (`blitz::Array<double, 1>`); deleting them breaks every blitz serialization.
They go in R3.2, which replaces the 37 macro lines and the typedefs with one template.

**R2.10 Include guards** per D3: `#pragma once` in every header in `lib/` and `cxx_python_rules/`, except the
upstream copies in `.clang-format-ignore`, which keep their own guards. This also removes the misspelled
`WEAK_PTR_SERIALIZED_SUPPORT_H` and the unprefixed guard names.

**R2.11 Autoconf macros leaking into public headers.** Make the header that installed headers include define only
`SKELETON_*` (and `CXX_PYTHON_RULES_*`) macros. Either generate it from the autoheader output with
`AX_PREFIX_CONFIG_H` (an autoconf-archive macro that isn't in `config/m4`, so add it to `config/m4_local/`), or write
a small `lib/skeleton_config.h.in` processed by `AC_CONFIG_FILES` that copies just the needed values, and rename the
autoheader output (`AC_CONFIG_HEADERS`) to a private `lib/config.h` used only by `.cc` files. Then delete the
`#undef PACKAGE...` block in `generic_object.h`. Also: `skeleton_config.h` isn't installed today (no `*_HEADERS`
entry), although the installed `generic_object.h` includes it, so an installed copy of the headers can't be used.
Install the cleaned header. This is also where R1.3 defines `CXX_PYTHON_RULES_NAMESPACE`. Do the same in the CMake
build (`CMakeLists.txt:31-35` writes the header). Check: after `make install`, a program that includes
`generic_object.h` from the install prefix compiles, and no installed header defines `PACKAGE`/`VERSION`/`HAVE_*`.

**R2.12 An obsolete Boost check** (found while reviewing this plan; not in the review).
`skeleton_serialize_support.h` includes `boost/serialization/boost_array.hpp` only `#ifdef
HAVE_BOOST_SERIALIZATION_BOOST_ARRAY_HPP`. The comment talks about Boost 1.64, but the minimum is now 1.84, which
always has that header. The skeleton also no longer has any `boost::array` (S3; the old archives load through the
`std::array` serializer in `array.hpp`). Remove the conditional and the include from `lib/`, and the
`#define HAVE_BOOST_SERIALIZATION_BOOST_ARRAY_HPP 1` line the CMake build writes (`CMakeLists.txt:33`). The
`#undef` in the autoheader template `skeleton_config.h.in` stays: it is generated from the shared
`config/m4/ac_boost.m4` check, which is left alone, and `./bootstrap` would put it back (after R2.11 it no longer
reaches an installed header). In `serialize_support.h.sample`, include it unconditionally, since geocal still uses
`boost::array`. The `array_sample` golden archives must still load.

## R3. Medium refactors (review §3.4, §4.1, §4.2, §4.5)

**R3.1 `GenericObjectMap` holds its map as a member.** Store `std::map<std::string, std::shared_ptr<GenericObject>>
map_` and expose `keys()`, `get_generic`, `get<T>`, `set_generic`, `contains`, `erase`, `size`, `empty`, and
`begin`/`end`, with `operator[]` too if anything uses it. **The archive must be byte for byte the same:** serialize
`map_` under the same element name (`MapGenericObject`). The golden files `generic_object_map.*`,
`director_example_map.*` and `observer_map.*` check it. If boost writes anything different for a base object than for
a member (it shouldn't for `std::map`, which writes no class information), keep the bytes the same with a private
helper. Update `generic_object_map_nb.cc`, which uses `count`. Mirror in `generic_object_map.{h,cc}.sample`.

**R3.2 One blitz serializer for all ranks.** Replace the five `save`/`load` pairs with one template over `N`. The
element names depend on the rank, so use one name table per rank: `size` (1); `rows, cols` (2); `rows, cols, depth`
(3); `shp0..shp3` (4); `shp0..shp4` (5). Also replace the 37 `BOOST_SERIALIZATION_SPLIT_FREE(blitz_*_array_*d)` lines
and their typedefs with one template, `template<class Archive, class T, int N> void serialize(Archive&,
blitz::Array<T, N>&, const unsigned int)` calling `split_free`, which works for every element type. Keep the explicit
instantiations for the same element types. The golden `blitz_*` archives (all types and ranks) must be identical. The
R1.1 fix then lives in one place. Mirror in the `.sample` files.

**R3.3 `serialize_function.cc` without duplication.** Use one `#ifdef` around the whole implementation, and one
`[[noreturn]] no_serialization()` for the build without serialization. Add `template<class OArchive> void
write(std::ostream&, obj)` and `template<class IArchive> std::shared_ptr<GenericObject> read(std::istream&)`, and
build the eight public functions from them. Paths become `const std::filesystem::path&`. Since `const std::string&`
callers still convert implicitly, the python side and the binding are unchanged; check the stubs. Error messages give
the file name and `std::strerror(errno)`. `skip_weak_ptr_handling_val` becomes `std::atomic<bool>`. Remove the unused
`<iostream>` and the needless `.c_str()` calls. Make `Filtering*`/`*Compress` constructors `explicit`. Check that a
write to an unopenable file throws an `Exception` that names the file (add a test); today the stream error may only
surface later, or not at all. Also fix an existing message bug: the "not built with boost::serialization" errors say
literally `CXX_PYTHON_RULES_NAMESPACE was not built with ...`, because a macro isn't expanded inside a string
literal; use `BOOST_PP_STRINGIZE(CXX_PYTHON_RULES_NAMESPACE)` or just "This library".

## R4. Design change (review §2.5)

**R4.1 A per-thread archive base directory instead of `chdir`.**
- In `serialize_function.h` add `const std::filesystem::path& archive_base_directory()` (thread-local; empty when
  not loading), `std::filesystem::path resolve_archive_path(const std::filesystem::path& P)` (returns P if it is
  absolute or no base is set, otherwise base / P), and an RAII `ArchiveBaseDirectory` that sets the base and restores
  the previous one (nested loads work).
- `serialize_read_generic`/`serialize_read_binary_generic` set the base to the archive's directory instead of using
  `DirChange`. Delete `DirChange`.
- Add `serialize_read_binary(const std::string& Data, const std::filesystem::path& Base_dir)`, bound with the same
  bytes conversion as the existing `serialize_read_binary` binding. Python's `_new_from_serialization_dir` calls it
  instead of `os.chdir`.
- **Python code that relied on `chdir`.** `python/skeleton/swig_shelve.py` (lines 80-117) and its sample
  `cxx_python_rules/python/swig_shelve.py.sample` `chdir` to the shelve's directory for xml, bin, json and sqlite
  loads, so pickles inside a shelve find their relative files. Bind `ArchiveBaseDirectory` as a python context manager
  (`with archive_base_directory(dirn): ...`) and use it there instead of `chdir`.
- **Resolve everywhere, not only in `load()`.** Objects constructed *during* a load (a C++ `load()` that builds
  another object, a python director object's `__setstate__`) used to see the archive's directory as the current
  directory. So `FileReferenceSample::read()` (and its geocal equivalents) always opens `resolve_archive_path(fname_)`;
  outside a load that returns the name unchanged, so the constructor behaves as before.
- **Threads.** The base directory is per thread, so a worker thread started during a load doesn't see it. Document
  that (the old `chdir` was visible to all threads, which was the bug).
- Tests: update `serialize_function_test.cc:111` (the working directory no longer changes; relative names still
  resolve); add a test that loads on two threads from two directories at once; a nested load; keep
  `file_reference_sample_test.py` (pickle loaded from another directory) and `swig_shelve_test.py` passing without
  `chdir`.
- Update the geocal note (already in the table) with the API names.

## R5. Rules library details (review §4.3, §4.4, §4.6)

**R5.1 Python 3.12 exception API.** In `register_exceptions`, use `PyErr_GetRaisedException()` and
`PyErr_SetRaisedException()` instead of `PyErr_Fetch`/`PyErr_NormalizeException`/`PyErr_Restore`. Python 3.12 becomes
the minimum, so declare it in `env/linux/pixi.toml` (`python = ">=3.12"`), in the CMake build (`find_package(Python
3.11 ...)` at `CMakeLists.txt:17` becomes 3.12), and in configure. The shared `config/m4/afids_python.m4` asks only
for `>= 3.4.0` (`AX_PYTHON_DEVEL`), and geocal and other projects use it too, so add the 3.12 check to the local
`config/m4_local/skeleton_common.m4` (a `sys.version_info` test that stops configure with a clear message) rather
than changing the shared macro. Test: `printing_test.py` still sees `cpp_exception` and `cpp_source`.

**R5.2 `blitz_array.h` and `std_array.h`.** Replace `(uint32_t)` with `static_cast`. In `to_numpy` and
`std_array.h`'s `from_cpp`, hold the new object in a `std::unique_ptr` until the capsule owns it, then `release()`.
`std_array.h`'s `from_cpp` is `noexcept` but catches only `python_error`, so a `bad_alloc` from `new T[N]`
terminates; catch `std::exception` too and set a python error.

**R5.3 C++ writes into read-only numpy arrays** (found by the independent review, confirmed). `view_of` in
`blitz_array.h:79-101` takes any array as `ndarray<const T>` and hands C++ a writable blitz view of it. A C++
function with a non-const `blitz::Array&` parameter then writes into an array python marked read-only:
`ColumnMajorSample.scale_in_place(a, 5)` with `a.flags.writeable = False` changes `a`. Fix: for a non-const
reference parameter, only take a view of a writable array (`ndarray<T>` without `const`), and reject a read-only one
(`TypeError`), rather than copying it, because the caller expects to see the write. For by-value and `const&`
parameters, read-only arrays stay fine. The caster has to know which case it is in: nanobind's `type_caster` sees
the `cast_flags`, or use a separate caster for `blitz::Array<T,N>&`. The R6.1 spike checks how. Test in
`array_typemap_sample_test.py`: a read-only array passed to `scale_in_place`/`stored_ref`-style functions raises, and
a writable one still works without a copy.

**R5.4 Free-threaded python.** Not supported; write that down instead of half-supporting it. Add a paragraph to
`cxx_python_rules/README.md`. Optionally `#ifdef Py_GIL_DISABLED #error ...` in `cxx_python_rules_nb.h`. The module
isn't declared `FREE_THREADED`, so nanobind keeps the GIL on anyway; check which and write that down.

## R6. Bindings rework (review §5)

Goal: binding files that read like hand-written nanobind code. Member functions are bound with member pointers, and
lambdas appear only where they do something. Keyword names match the C++ parameter names. There's no generator
boilerplate. The python API is unchanged (R0.2 snapshot), apart from the keyword renames.

**R6.1 Spike.** On two files (`generic_object_sample_nb.cc` and `multi_base_example_nb.cc`, which has secondary and
virtual bases), settle these things with nanobind 3.1:
1. `c.def("f", &T::f, nb::arg("A"), doc)` where `f` is declared in `T` or in `T`'s nanobind base: works, and the
   signature, docstring and keyword names are the same as with the lambda.
2. `&Base::f` where `Base` is a **non-nanobind** base (a virtual or secondary base such as `WithParameterLike`).
   Reading nanobind 3.1 shows this works: `class_<T>::def`, `def_prop_ro` and `def_prop_rw` pass `T` as the target
   type, so `self` is converted as `T*` and then `(self->*f)(...)` is called (`nb_func.h`, `cpp_function_def`), and
   the independent review compiled `nb::class_<Diamond>().def("parameter_sum", &WithParameterLike::parameter_sum)`.
   Confirm it at run time (the right result for a `Diamond`, including through `overload_cast`). No `method<T>`
   adapter is needed; add one only if the run-time check fails.
3. Overloads: `nb::overload_cast<int>(&T::value_to_add)` and `nb::overload_cast<>(&T::value_to_add, nb::const_)`.
4. Getter/setter properties: `def_prop_ro("v", &T::v)`, and `def_prop_rw` with two `overload_cast`s.
5. Docstrings: a `cxx_python_rules_nb::docs` helper made once per class (`const auto doc =
   cxx_python_rules_nb::docs("Skeleton::Foo");`), with `doc()` for the class and `doc("f")` for a member. Inherited
   members still pass the full name of the declaring class.
6. Return value policies: compare `__nb_signature__` and the behavior (a numpy view or a copy, keeping the parent
   alive) of reference returns between the lambda and the member pointer. Keep `nb::rv_policy` the same where they
   differ.
Write the rules that come out of this into `cxx_python_rules/nanobind/README.md` ("Writing bindings").

**R6.2 When a lambda is still right** (also for the README):
- **director dispatch** for virtual functions of trampoline classes (`if(dynamic_cast<const PyX*>(&self)) return
  self.X::f(...); return self.f(...);`). When a python override calls `super().f()`, the bound function would
  otherwise call the virtual `f`, which goes back through the trampoline. nanobind 3.1 notices that re-entry and
  calls the C++ base instead of recursing forever (`trampoline.cpp`, `trampoline_enter`), but by then the python
  override has run a second time (the S6.2 progress-log entry: "else the override ran twice"). The lambda calls the
  base implementation directly, so the override runs once. Put one comment per file explaining this, instead of
  nothing;
- parameters that are a **reference to a secondary or virtual base**, taken as `shared_ptr` so the type registry can
  convert them;
- OUTPUT arguments returned as a tuple, `FORCE_COPY`, stream arguments (`py_ostream`/`py_istream`), and
  `to_std_array` property setters;
- `nb::new_` constructors returning `std::make_shared`. These have to stay (decision 6). Don't take the address of
  `std::make_shared`: the standard doesn't allow taking the address of standard library functions. If there are
  many, use a small `cxx_python_rules_nb::make_shared_init<T, Args...>()` helper that returns the `nb::new_` lambda.

**R6.3 Convert every `lib/*_nb.cc`**, one file per step, running the python tests for that file after each:
- member pointers instead of the `decltype(auto)` lambdas;
- keyword names equal to the C++ parameter names in the header. Fix the mismatches (`DirectorExample` ctor
  `V`/`Name`, the `arg0`s in `director_example_nb.cc` and `array_typemap_sample_nb.cc`, and whatever R9's check
  finds). Python callers using the old keywords break; R0.2 shows exactly which;
- drop the `{ auto c = nb::bind_vector<...>(...); }` blocks (keep the scope only where `c` is used, e.g.
  `pickle_vector(c)`), the "Bases not bound as the nanobind base" comments (write one comment where a base really
  matters), the empty `namespace { }`, and the "Generated by i_to_nanobind.py" headers (replace with one line saying
  what the file binds);
- trampolines: keep `NB_TRAMPOLINE`/`NB_OVERRIDE_NAME`; use parameter names from the header.
Also `downstream_nb.cc`.

**R6.4 Module init without the generated `.inc`.** `skeleton_nb_module.cc` declares the `init_*` functions and calls
them in order in plain code, with a comment on why the order matters. Delete `skeleton_nb_init.inc` and its
`EXTRA_DIST` line in `lib/lib.am`. `skeleton_nb_opaque.h` loses its "Generated" header.

**R6.5 Teach the generator the new style** (`cxx_python_rules/nanobind/i_to_nanobind.py`). geocal will generate its
first bindings with it, so its output should look like the reworked files rather than the old ones: member pointers
by default, lambdas only in the R6.2 cases, `docs` helper, no empty blocks, no `.inc`. Check: run it on the `.i` files
from commit `0802a01^` (the last commit before S6.6 removed them; `git show 0802a01^:lib/<name>.i`) and compare the
output with the reworked files. Hand-made changes are allowed, but the differences should be small and explainable.

**R6.6 Check:** add a python test (`nanobind_test.py`) that goes through every class in `skeleton_cxx._bindings` and
`_skeleton_downstream` and checks that the type registry has it. The test can't select "classes derived from
`GenericObject`" from python: `GenericObject` is always a virtual base, never a nanobind base, so
`issubclass(c, GenericObject)` is false for every class. Instead check every bound class except an explicit
allow-list of classes that aren't `GenericObject`s (`Vector_*` from `bind_vector`, `Range`, `rational_int`, ...).
Implement `_is_registered(type)` in `cxx_python_rules_nb` with `nbreg::find(nb::type_info(t))`
(`type_registry.h`), and bind it so the test can call it. A class whose binding
forgets `register_type` then fails the test, whoever wrote the binding. Check that it fails by leaving one
`register_type` out once. The snapshot and the stub file differ from the baseline only in the keyword renames. Add a python
test that calls a few of the renamed functions by keyword (the `DirectorExample` constructor,
`DirectorExampleWeakPtr`, `ArrayTypemapSample.set_value_copy`), so the names can't drift back. `make check`
and `make installcheck` pass. `lib/*_nb.cc` is about half its present 2,663 lines, and `grep -c 'decltype(auto)'
lib/*_nb.cc` counts only director dispatch lambdas.

## R7. Warnings and lint (review §4.8, §4.9)

**R7.1 Compiler warnings.** Fix the 101 `-Wsign-conversion` and 2 `-Wconversion` warnings in `lib/`,
`cxx_python_rules/lib/` and the C++ tests: index with `std::size_t`/`std::ptrdiff_t`, or `static_cast` at the blitz
or mdspan boundary (`int` extents) with a comment. Then add `-Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast
-Wsuggest-override -Wnon-virtual-dtor -Wconversion -Wsign-conversion` to the project's C++ flags in
`config/m4_local/skeleton_common.m4`, each through `AX_CHECK_COMPILE_FLAG` (an autoconf-archive macro that isn't in
`config/m4` today; add it to `config/m4_local/`, not to the shared `config/m4/`), for
`lib/`, the C++ tests and the bindings. Third-party headers come in with `-isystem` and stay quiet. One known case: `gc_shared_ptr.h`, the upstream copy we don't
reformat, is included with `-I` and gives 6 `-Wold-style-cast` warnings in every binding file (confirmed by the
independent review). Put `#pragma GCC system_header` in it (one line, the only change to the copy, with a comment), or
drop `-Wold-style-cast` for the binding targets. Their macros
(`NB_TYPE_CASTER`, `NB_OVERRIDE_NAME`, `BOOST_CLASS_EXPORT_*`, `BOOST_AUTO_TEST_CASE`) are expanded in our own files,
though, so compile the bindings and tests with the flags before turning them on. If a flag only fires inside such a
macro, turn it off for just those targets and write down why. `nb_combined.cc` (nanobind's own source) gets no extra
flags. Set the flags in the CMake build too. `-Werror` only through an opt-in `--enable-werror`, so a new compiler's
new warnings don't break user builds.

**R7.2 clang-tidy.** Turn on, after each corresponding R2 step: `google-explicit-constructor`,
`cppcoreguidelines-pro-type-member-init`, `cppcoreguidelines-prefer-member-initializer`, `modernize-use-using`,
`modernize-use-default-member-init`, `modernize-pass-by-value`, `performance-unnecessary-value-param`,
`readability-container-contains`, `modernize-use-ranges`, and `bugprone-*` (minus checks that only fire on Boost or
blitz macros; list each one turned off with the reason). All of these exist in clang-tidy 22. Leave
`modernize-use-std-print` off: it would rewrite stream output that is supposed to stay a stream. `make lint` must be
clean. The header filter already covers `cxx_python_rules/lib`, and `make lint` already includes the bindings. But
`ExcludeHeaderFilterRegex` excludes only `gc_shared_ptr.h`, not the other upstream copy `serialize_shared_ptr.h`;
add it, or `bugprone-*` reports on it.

## R8. Samples and documentation

**R8.1 `.sample` files** in `cxx_python_rules/lib/`: bring each up to date with its `lib/` counterpart after R1–R3
(Printable, GenericObjectMap, blitz serialization, `using`, `#pragma once`, `explicit`), so a new project starts from
the cleaned code. They aren't compiled, so check them by hand or by a diff with namespace substitution
(`sed s/GeoCal/Skeleton/`) against `lib/`.

**R8.2 Documents:** CLAUDE.md (moved files, the bindings rules, warnings), `cxx_python_rules/README.md`,
`cxx_python_rules/nanobind/README.md` (R6.1 rules), `DirectorNotes.md` (if R1.9 changes what it shows), the "Notes
for geocal" table (fill in the real API names after R1.3/R4.1 and a row for R6 and R7), and a status line at the top
of `code_review_2026-11-06.md` pointing here.

## R9. Final check: are the problems gone?

Run all of these. Each must give the stated result; any hit is fixed or written down as an exception with a reason.

| Review item | Check |
|---|---|
| §2.1, §2.2 | The three R0.4 serialization tests pass (no longer expected failures); the `static_assert`s on non-copyable observers compile; the R1.2 trial-build results are written down and decided. |
| §2.3 | `grep -rni skeleton cxx_python_rules/ --include='*.h' --include='*.cc'` → nothing (this also catches `namespace Skeleton`, `SKELETON_*` macros and `skeleton_mdspan.h` includes); a program using only the installed headers compiles with no `-D`. |
| §2.4 | `grep -nE '\[[A-Za-z_]+\]|\(I, J\)' lib/*.h lib/*.cc` reviewed: no index that comes from python goes unchecked, including blitz `A(I, J)`. |
| §2.7 | The R1.4 `None` and index tests and the R5.3 read-only tests pass; `grep -c '.none()' lib/*_nb.cc` reviewed: each remaining one is a parameter C++ accepts as null. |
| §2.5 | `grep -rn 'chdir\|DirChange' cxx_python_rules/ lib/ python/skeleton/` → nothing (including `swig_shelve.py` and its `.sample`). |
| §2.6, §3.1 | clang-tidy (R7.2) clean; ThreadExample/read_ptr tests pass. |
| §3.1 destructors | `grep -n '~.*= default;' lib/*.h cxx_python_rules/lib/*.h` reviewed: what's left is the first virtual destructor of a hierarchy (`GenericObject`, `GlobalFixture`) or a destructor that does work. |
| §3.3 | `grep -rn 'std::runtime_error' lib/` → nothing; `grep -rn '__FILE__\|__LINE__' lib/ --exclude='*_test.cc'` → nothing (the tests use them on purpose to check messages). Use `grep -r` throughout: GNU grep on a directory without `-r` reports "Is a directory" and finds nothing, which looks like a pass. |
| §3.4 | `grep -n 'public std::map' -r lib cxx_python_rules` → nothing. |
| §3.5 | `grep -n 'const_cast' lib/*.h lib/*.cc` → only the commented deliberate uses. |
| §3.6 | `grep -rn 'typedef' lib cxx_python_rules --include='*.h' --include='*.cc' --include='*.sample' \| grep -v 'serialize_shared_ptr.h\|gc_shared_ptr.h'` → nothing (the two upstream copies keep their own style); `grep -rLn 'pragma once' lib/*.h cxx_python_rules/lib/*.h cxx_python_rules/nanobind/*.h` → only the two upstream copies (D3); `grep -n '#undef' lib/generic_object.h` → nothing; `grep -rn 'BOOST_ARRAY_HPP' lib` → nothing (R2.12). |
| §4 | `grep -rn 'PyErr_Fetch\|PyErr_Restore\|(uint32_t)' cxx_python_rules/` → nothing; `serialize_function.cc` has one `#ifdef`. |
| §4.8, §4.9 | `make lint` clean; every build is warning-free with the R7.1 flags. |
| §5 | R6.6. |
| everything | R0 builds (gcc 15, gcc 14, no serialization, ASan) pass `make check`; `make installcheck` passes; the golden archives are identical; the API snapshot differs only in the intended changes; the CMake reference build (`build-cmake`) still configures and builds. It globs `lib/*.cc` and `cxx_python_rules/lib/*.cc`, so the R1.3 move needs no edit, but its doxygen input is `lib/*.h` only and its warning flags come from R7.1. |

Last, repeat the review itself: a fresh read of the changed files, plus the strict-warnings compile from review §1.
Record the result in the progress log.

## Review of this plan (2026-11-06)

The plan was checked in two ways: every item in the review has to map to a step that removes it and a check that
shows it's gone, and the code was searched again for every instance of each pattern (the review often named only
examples).

**Coverage of the review.**

| Review | Step(s) | Review | Step(s) |
|---|---|---|---|
| §2.1 column-major | R1.1, R3.2 | §3.4 GenericObjectMap, small idioms | R3.1, R2.8 |
| §2.2 Observer copy | R1.2 | §3.5 `const_cast` | R2.7 |
| §2.3 `Skeleton::` | R1.3 (and namespace plumbing) | §3.6 `typedef`, guards, `#undef` | R2.9, R2.10, R2.11 |
| §2.4 python indices | R1.4 | §4.1, §4.2 `serialize_function.cc` | R3.3, R4.1 |
| §2.5 `chdir` | R4.1 | §4.3 PyErr API | R5.1 |
| §2.6 table | R1.5–R1.10 | §4.4 free threading | R5.4 (documented, not supported) |
| §2.7 `None`, read-only arrays | R1.4, R5.3, R6.3 | | |
| §3.1 constructors, destructors | R2.1–R2.4 | §4.5 blitz per rank | R3.2 |
| §3.2 Printable | R2.5 | §4.6 casts, leak | R5.2 |
| §3.3 exceptions, `range_check` | R2.6, R1.8 | §4.7 upstream copies | none (reviewed as out of scope) |
| §4.8, §4.9 lint, warnings | R7.2, R7.1 | §5 bindings | R6.1–R6.6 |
| §6 keep | none (R2.6 writes the reasons into the code) | §7 order | "Order and dependencies" |

**What the review pass added** (all now in the steps above):
1. Blitz element access with indices from python (`data_value`, `element`, `stored_value`) is as unchecked as the
   `std::vector` indexing in review §2.4 → R1.4.
2. The complete list of uninitialized members (the review named four; there are thirteen) → R1.5.
3. The obsolete `HAVE_BOOST_SERIALIZATION_BOOST_ARRAY_HPP` conditional, and a `boost_array.hpp` include the skeleton no
   longer needs → new R2.12.
4. The python minimum lives in the shared `afids_python.m4` (`>= 3.4.0`), so the 3.12 requirement goes in the local
   macro → R5.1.
5. A first draft suggested `nb::new_(&std::make_shared<...>)`, which isn't allowed (you can't take the address of a
   standard library function) → R6.2 corrected.
6. Third-party macros expanded in our own files can trigger the new warning flags → R7.1 checks first and turns flags
   off per target only where needed.
7. The R9 searches would have matched the two upstream copies, which keep their own style; they are now excluded.
8. The `.sample` files have the same problems as `lib/` (`typedef`, the `std::map` base, blitz serialization). Without
   R8.1 a new project would start from the old code.
9. The generator `i_to_nanobind.py` still writes the old style, and geocal will use it → R6.5, checked against the
   `.i` files from git history.
10. A python test that pins the renamed keyword arguments → R6.6. The CMake reference build added to the R9 check.
11. Ignored editor backup files → R1.10 (listed, deleted only with the maintainer's OK).
12. Step dependencies written down ("Order and dependencies").

**Checked and left out on purpose:**
- `lib/test_all.cc` uses Boost.Test's header-only `included` variant while also defining `BOOST_TEST_DYN_LINK`. It
  works, and it only costs compile time in one file. Possibly worth tidying later; not a review problem.
- `exception_sample.h` throws `std::out_of_range` on purpose (it tests a non-`Exception` error), so the R2.6 check
  searches for `std::runtime_error` only.
- `boost::core::demangle` and `boost::rational` stay: there is no `std` replacement.
- The python package (`python/skeleton`) uses none of the keywords R6 renames, so only the bindings change.

**Conclusion:** with these additions, the plan covers every problem in the review and the extra ones found while
checking it. Each has a step that fixes it and an R9 check that shows it's gone. The five decisions (D1–D5)
were settled on 2026-11-06; what is still open is the maintainer's "go", and, after the R1.2 trial build, whether to
keep D2.

## Independent review (2026-11-06)

A fresh reviewer, with no part in writing either document, checked the review and this plan against the code
(reading the code and nanobind 3.1's source, and running throwaway programs). Its findings were checked again before
being applied: the ones marked *confirmed* were reproduced; the rest were checked by reading the code. All are now in
the steps above.

**Wrong in the review or the plan (corrected):**
1. The 37 `blitz_*_array_*d` typedefs are not unused: they are the arguments of `BOOST_SERIALIZATION_SPLIT_FREE`.
   Deleting them (old R2.9) would have broken all blitz serialization → moved to R3.2 with a replacement template.
   (Review §3.6 corrected.)
2. R1.1 fixed only the column-major save. A reversed array is read out of bounds on save (*confirmed*), and loading
   into a non-row-major target scrambles the values (*confirmed*) → R1.1 rewritten, R0.4 tests added.
3. R1.2: deleting `Observable`'s copy removes its implicit default constructor; `Observable() = default;` is needed.
   There is no runtime copy test to replace; move checks added.
4. R1.8: the old `range_check` template did not allow mixed argument types, and `std::format` would require a
   formatter for every checked type → one `T` deduced from `Val`, stream output kept.
5. R6.1: the `method<T>` adapter isn't needed (nanobind converts `self` as the bound class; checked in `nb_func.h`).
   D5 updated.
6. R6.2 and review §5: the director dispatch lambdas stop a python override running twice; nanobind already
   prevents the infinite recursion (checked in `trampoline.cpp`).
7. R6.6: the registration test can't select classes by `issubclass(GenericObject)` (a virtual base, never a nanobind
   base) → all bound classes minus an allow-list.
8. R2.12: `skeleton_config.h.in` is generated by autoheader, so its `#undef` can't be removed by hand; the CMake
   define is what to remove.
9. R7.2: the header filter and the bindings were already covered; `serialize_shared_ptr.h` was missing from the
   exclude list.
10. R9: the `Skeleton::` search missed `namespace Skeleton` and `SKELETON_*`; the `__FILE__` search hit the tests; a
    missing `-r` would have passed vacuously with GNU grep.
11. R0.3: three golden files aren't written by the generator; compare only its 104 files.
12. R1.7: `write_ptr` already checked first; only `read_ptr` needs the fix.
13. The "Notes for geocal" column-major row didn't reflect D1 or the load side (updated in `implementation_plan.md`).

**Missing (added):**
1. `None` for a `shared_ptr` parameter crashes python (*confirmed*: segmentation fault) → R1.4, R6.3, review §2.7.
2. C++ writes into read-only numpy arrays (*confirmed*) → new R5.3, review §2.7.
3. Namespace plumbing for R1.3 (the macro must come from a project header; installed headers) → R1.3.
4. `skeleton_config.h` isn't installed although installed headers include it → R2.11.
5. `gc_shared_ptr.h` triggers `-Wold-style-cast` in every binding file; `AX_CHECK_COMPILE_FLAG` and
   `AX_PREFIX_CONFIG_H` aren't in `config/m4` → R7.1, R2.11.
6. R4.1: `swig_shelve.py` (the live module, not only its sample) `chdir`s; objects built during a load; threads; the
   bytes conversion of the new binding → R4.1.
7. `ExceptionFormat` must stay implicit (`NOLINT`) → R2.1.
8. The R1.9 counter must be atomic and count copies → R1.9.
9. The CMake python minimum; missing rows in "Intended behavior changes" → R5.1, table.
10. The "not built with" error text prints the macro name literally → R3.3.
11. Nits: `clean_dead_ptr()` const (R2.7), `prefer-member-initializer` turned on (R7.2), per-rank name tables (R3.2),
    `jthread` declaration order (R1.6), `std_array.h` `bad_alloc` (R5.2).

## Progress log

| Phase / step | State | Notes |
|---|---|---|
| Plan written | done | 2026-11-06 |
| Decisions D1–D5 | done | 2026-11-06: D1 throw on nonzero base; D2 delete copying (B), revisit after the R1.2 trial build; D3 `#pragma once`; D4 move the mdspan files; D5 ordinary nanobind plus `docs`/`method` helpers and a registration test. |
| Independent review | done | 2026-11-06: 13 corrections and 11 additions, applied (section "Independent review"). |
