# Code review: quality and modernization, a fresh look (2026-11-07)

**Scope.** All C++ in `lib/`, `cxx_python_rules/lib/` and `cxx_python_rules/nanobind/` (about 9,900 lines), the build
files (`configure.ac`, `config/m4_local/`, `*.am`, `CMakeLists.txt`, `.clang-tidy`), and a short look at the python
code. This review comes after the 2026-11-06 review's fix plan (R0 to R9), phase S7 and the rename.

**Method.** I read the code, ran clang-tidy 22 with a much wider check set than `.clang-tidy` (report only, §9),
compiled two small test programs, and ran two python scripts against the scratch build to confirm the findings in §4.
Nothing in the repository was changed except this file.

**Fix plan.** `code_review_2026-11-07_fix_plan.md`.

**Earlier documents.** `code_review_2026-11-06.md` and `code_review_fix_plan.md`. Decisions made there (D1 to D5,
and review §6 "Legacy that should stay") are not argued again here; §7 lists them so they aren't taken for oversights.

## 1. Verdict

The code is in good shape and mostly reads like C++ written today. It is C++23 with gcc 14 as the minimum compiler.
Boost smart pointers and containers are gone in favor of `std::`. Newer language and library features are used where
they fit: concepts, `std::format` with format strings checked at compile time, `std::source_location`, ranges,
`std::jthread`, `mdspan` with `m[i, j]` indexing, `consteval`. The code builds with no warnings under a strict warning
set, is formatted with clang-format, and passes the project's clang-tidy checks. A much wider clang-tidy run found
154 issues in project code, almost all trivial (§9). There are no owning raw pointers apart from a few deliberate
cases, no `NULL`, and no `typedef` outside a copy of Boost code.

Where the code does not look new, the cause is mostly design inherited from geocal and framework and kept on purpose:
every bound class derives virtually from `GenericObject`; Boost.Serialization with its macros and explicit
instantiations; Blitz++ arrays; an Observer built on non-owning `shared_ptr`s. A project started today would design
these differently, but they are needed for compatibility (existing archives, geocal's API), and the code handles them
carefully.

The gaps that matter:

1. **Two bugs, confirmed, and one latent bug, all in `cxx_python_rules/`**, which geocal copies: nested serialization
   loses `weak_ptr`s (§4.1), XML archives that contain a python subclass are not text (§4.3), and a header has no
   include guard (§4.2).
2. **Thread safety is handled case by case.** Several `const` member functions change shared state without a lock,
   and there is process-wide mutable state that relies on the GIL. None of this is written down per class (§5.1).
   It matters if free-threaded python becomes a goal.
3. **Tooling a new project would have:** CI, sanitizers as a `configure` option (today there is a hand-configured
   ASan build), UBSan, libstdc++ assertions in debug builds, and a clang build (§6).
4. **Comments still carry the history of the modernization** (SWIG, phase numbers, review sections, `claude_notes/`
   paths). Useful now, but noise in a skeleton that is meant to be copied (§5.6).

## 2. Recommendations, in order

| # | What | Kind | Effort | geocal too? |
|---|---|---|---|---|
| 1 | Give each serialization write its own `weak_ptr` mark set (§4.1) | bug, confirmed | M (add the test first; look at the old framework failure) | yes |
| 2 | Add `#pragma once` to `serialize_shared_ptr.h` (§4.2) | latent bug, confirmed | S | yes |
| 3 | Use `bytes` for the python XML string functions; decide how pickles go into XML (§4.3) | bug, confirmed | S to M | yes |
| 4 | Put the namespace in the type registry key; stop allocating a string on every lookup (§4.4) | latent bug, speed | S | yes |
| 5 | Give the file and in-memory `serialize_read_binary`/`serialize_write_binary` overloads different names (§4.5) | API trap | S, but an API change | yes |
| 6 | `configure`: require Boost 1.84, not 1.46 (§4.6) | build | S | check |
| 7 | Sanitizer `configure` option with UBSan, `_GLIBCXX_ASSERTIONS` in debug builds, a clang build, CI (§6.2, §6.4) | tooling | M | yes |
| 8 | Write down thread safety per class; decide on free-threaded python (§5.1) | design | M | yes |
| 9 | Make copying `Exception` unable to throw (§5.2) | modernization | S | yes |
| 10 | Smaller modernization items (§5.3, §5.4, §5.5) and the cheap clang-tidy checks (§6.3) | style | S each | some |
| 11 | Sweep the history out of comments once the modernization is finished (§5.6) | docs | M | yes |
| 12 | Make the CMake reference build modern CMake before framework copies it (§6.5) | build | M | framework |

## 3. What already looks like C++ written today

| Area | Where |
|---|---|
| C++23, strict mode (`-std=c++23`, no GNU extensions), library features newer than gcc 14 avoided | `config/m4_local/skeleton_common.m4:84-93` |
| `std::shared_ptr`/`weak_ptr` throughout; `enable_shared_from_this` on `GenericObject`; non-owning pointers made with the aliasing constructor so `weak_this` isn't taken over | `generic_object.h:50`, `null_deleter.h:28-46` |
| Format strings checked at compile time for exceptions: `throw Exception("Line {} …", ln)` through a `consteval` constructor and `std::format_string` | `skeleton_exception.h:26-39`, `92-99` |
| `std::formatter` for every `Printable`, constrained by a concept | `printable.h:77-100` |
| `std::source_location` instead of `__FILE__`/`__LINE__` macros; `range_check` is a function, and `std::type_identity_t` prevents mixed signed/unsigned deduction | `skeleton_exception.h:204-276` |
| Ranges and algorithms: `find_if`, `count_if`, `views::keys \| ranges::to`, `fold_left`, `views::cartesian_product` | `observer.h`, `generic_object_map.h:47`, `thread_example.cc:42`, `raster_sample.cc:12-19` |
| `std::jthread`, worker exceptions carried by `std::exception_ptr` | `thread_example.cc:20-41` |
| Per-thread archive base directory instead of changing the process's current directory | `serialize_function.cc:25-48` |
| `mdspan` for pointer-plus-size interfaces, `submdspan`, `layout_stride`; zero-copy numpy ↔ Blitz through an `mdspan` view | `raster_sample.{h,cc}`, `blitz_mdspan.h`, `blitz_array.h` |
| RAII throughout, with the member order that matters documented | `ArchiveBaseDirectory`, `PtrSerializeLock`, `OutputFile` (`serialize_function.cc:94-112`), `unique_ptr` until the capsule owns it (`blitz_array.h:128-132`) |
| Copying deleted where a copy would be wrong, checked by `static_assert` in the tests | `observer.h:54-55`, `observer_test.cc:10-19` |
| `override` used consistently (87 times); `explicit` enforced by clang-tidy; default member initializers; `= default` | throughout |
| `if constexpr` and one generic template instead of one function or macro line per rank and type | `blitz_array_serialize_support.{h,cc}` |
| Golden archives that pin the binary and XML formats; python tests run in parallel | `unit_test_data/golden/`, `golden_archive_test.cc` |
| No warnings with `-Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast -Wconversion -Wsign-conversion` and more; clang-format pinned; `make lint` | `skeleton_common.m4:119-133` |

## 4. Bugs and latent problems

### 4.1 Nested serialization drops `weak_ptr`s (confirmed)

How it fails:

- A `weak_ptr` is written only if its target was marked during a first pass over the object
  (`weak_ptr_serialize_support.h:7-62`). The marks are in one process-wide set.
- `mark_pointer()` clears the set at the start of every `serialize_write*` call (`serialize_function.cc:139-148`).
- Writing a python subclass object pickles its `__dict__` (director serialization). If the `__dict__` holds a C++
  object, pickling that object calls `serialize_write_binary` again, in the middle of the outer write. The inner call
  clears the outer write's marks.
- The header says the opposite: "only the outermost lock clears the mark state"
  (`weak_ptr_serialize_support.h:82-83`). The code that did that is commented out
  (`weak_ptr_serialize_support.cc:20-34`, with a note that it gave null pointers in framework's tests).
  `ptr_serialize_depth` is still counted but never read.

Reproduced against the scratch build: a `GenericObjectMap` holding `"a"` (a `GenericObjectA`), `"b"` (a python
subclass of `DirectorExample` with a `GenericObjectA` attribute) and `"c"` (a `GenericObjectC` with a `weak_ptr` to
`"a"`). After a binary round trip, `c.a` is `None`. Without `"b"`, it is kept. Only `weak_ptr`s written after the
nested pickle are lost, so the result depends on key order.

Suggested fix: one mark set per write instead of one global set. Keep a `thread_local` stack of sets; each
`serialize_write*` pushes a new set (RAII), the mark pass and the write use the top one, and it is popped at the end.
The nested write then has its own set and leaves the outer one alone. The mark state no longer needs the
`recursive_mutex`, since it is per thread. This also avoids the problem of the commented-out version, where the inner
write saw the outer write's marks and kept `weak_ptr`s to objects that weren't in the inner archive (pulling them in),
which may be what failed in framework. Add the python test above first, and fix the header comment either way.

### 4.2 `serialize_shared_ptr.h` has no include guard (confirmed)

It is the only header without `#pragma once` (decision D3 says every header). Including it twice fails to compile:
redefinitions of `version<std::shared_ptr<T>>`, `tracking_level`, `shared_ptr_helper_id`, `null_deleter` and
`our_shared_ptr` (checked by compiling a file that includes it twice). Nothing includes it twice today
(`skeleton_serialize_support.h` and `serialize_function.cc` are the only includers), but in geocal, with many more
headers, it is a trap. The upstream Boost file it was copied from has a guard.

While there: `<iostream>` isn't needed, and `shared_ptr_helper.hpp` is included outside
`#ifdef CXX_PYTHON_RULES_HAVE_BOOST_SERIALIZATION` (lines 17-25), so a `--without-boost-serialization` build still
needs the Boost.Serialization headers (as `weak_ptr_serialize_support.h:4-5` does too).

### 4.3 XML archives that contain a python subclass are not text (confirmed)

Director serialization writes `python_object`, a pickle in python's default binary protocol, as a `std::string`
(`cxx_python_rules_nb.h:353-359`, `_nanobind_support.py:101-103`). In an XML archive the bytes go in as they are.
Checked against the build:

- The XML file isn't valid UTF-8, so it isn't well-formed XML even though it declares UTF-8. Boost reads it back
  without trouble; other XML tools won't. The golden `director_python.xml` and `director_python_user.xml` are the same,
  so this format comes from the SWIG era.
- In python, `serialize_write_string(obj)` raises `UnicodeDecodeError` for such an object, because the `std::string`
  is converted to `str` (`serialize_function_nb.cc:32`). `serialize_read_generic_string` has the matching problem on
  input.

Fix in two steps: (1) have the python XML string functions return and accept `bytes` (or add `bytes` versions);
(2) only if well-formed XML matters to someone: write the pickle base64-encoded in new XML archives, marked by a class
version, and keep reading the raw form.

### 4.4 Type registry: one key shared by all projects, and a string built on every lookup

- The registry is stored in `builtins` under `"__cxx_python_rules_nb_type_registry_v1__"`
  (`type_registry.h:49`). Each project has its own `CXX_PYTHON_RULES_NAMESPACE::nbreg::Map` type. If geocal and
  framework are imported into the same python process, the second module casts the first module's map to its own
  type. That works only while both use the same layout and the same libstdc++ ABI, and it is undefined behavior
  regardless. The projects never look up each other's types (the caster only uses the registry for its own
  `GenericObject`), so sharing gains nothing. Fix: put the namespace in the key
  (`BOOST_PP_STRINGIZE(CXX_PYTHON_RULES_NAMESPACE)`), and perhaps nanobind's ABI tag.
- `find(T.name())` makes a `std::string` from the mangled type name every time a `shared_ptr` goes to python and on
  every converting argument (`type_registry.h:63-68`, `gc_shared_ptr.h:169`). Mangled names are longer than the
  short-string buffer, so this is a heap allocation per conversion. C++20 heterogeneous lookup (a transparent hash
  plus `std::equal_to<>`) or a small `std::type_index` cache in front fixes it. Changing `Map` means bumping the
  version in the key, as the header comment says.

### 4.5 `serialize_read_binary` means file or data depending on the argument type

In `serialize_function.h`, `serialize_read_binary(const std::string& Data)` (line 35) reads archive bytes, while
`serialize_read_binary<T>(const std::filesystem::path& Fname)` (line 47) reads a file. A call
`serialize_read_binary("x.bin")` without a template argument picks the first and tries to parse the text `x.bin` as an
archive. In the same way `serialize_write_binary(path, obj)` writes a file and `serialize_write_binary(obj)` returns
bytes, while the XML pair is named `serialize_write` and `serialize_write_string`. Today's style is names that say
file or memory (`…_file` / `…_string`, or `std::span<const std::byte>` for data). This changes the API, python names
included, so do it together with geocal's own switch.

### 4.6 `configure` accepts Boost 1.46

`configure.ac:47` has `AC_BOOST(required, 1.46.0)`. CLAUDE.md says Boost 1.84 or later, `CMakeLists.txt:22`
requires 1.84, and the code works around problems in recent Boost (`serialize_shared_ptr.h:4-16`, Boost 1.90). An old
Boost passes `configure` and fails later in the compile. Ask for 1.84.

### 4.7 Smaller problems

- `global_fixture_default.cc.in:19-23`: the error message has words missing ("but you need to" … "running in a
  debugger").
- `numpy_sample.h:12`: `NumpySample` derives from `Printable<NumpySample>` but has no `print()`, so `cout << s` or
  `std::format("{}", s)` would not compile. Add `print` or derive from `GenericObject` instead.
- `observer_example.cc:61-64`: `CacheExample::load` uses `observed_` without a null check. The constructor checks;
  a damaged archive doesn't go through it.
- `raster_sample.cc:54-60`: `RasterSample::read` allocates the result before checking the region, so negative sizes
  reach the Blitz constructor first. Check first.
- `blitz_array.h:41-66`: `numpy_dtype_name` maps any type it doesn't know to an unsigned integer name of the same
  size (`std::complex<double>` becomes `"uint64"`). Today a binding with such an element type fails at link time
  instead, because `blitz_mdspan.cc:21-32` defines `BlockMember<T>::ptr` only for the listed types (I checked), but
  the error names `BlockMember<T>::ptr` and is hard to read. A `static_assert` listing the supported types, in both
  places, gives a clear compile error.
- `unit_test_support.h:5` includes `boost/test/floating_point_comparison.hpp`, which Boost marks deprecated
  (`BOOST_HEADER_DEPRECATED`). Use `boost/test/tools/floating_point_comparison.hpp`, or drop it
  (`unit_test.hpp` already includes it).
- `weak_ptr_serialize_support.cc:5` includes `<iostream>` without using it. `cxx_python_rules_nb.h` includes
  `generic_object.h` twice (lines 10 and 22).
- `serialize_function.cc:99`, `119` use `std::strerror(errno)`. It isn't thread safe, and `std::ofstream` isn't
  required to set `errno`, so the message can be wrong.

## 5. Modernization opportunities in the C++ code

### 5.1 Thread safety and `const`

The convention today, which the standard library assumes, is that `const` member functions are safe to call from
several threads at once. Here several `const` functions write `mutable` state without a lock:

- `Observable::notify_update_do` and `clean_dead_ptr` remove expired entries from `olist`/`colist`
  (`observer.h:251-263`, `322-329`).
- `CacheExample::value` and `CacheInvalidatedObserver::cache_valid` (`observer_example.h:143-151`, `observer.h:124`).
- `CalcRasterSample::read_into` (documented as not thread safe) and `BufferRasterSample::number_copy_`.

The Observer's `weak_ptr` is made from a non-owning pointer (`observer.h:41`, `85`), so `lock()` doesn't keep the
observer alive. An observer destroyed on another thread while it is being notified is a use after free. With one
thread this is fine; the design comes from framework and geocal.

Process-wide mutable state: the `weak_ptr` mark set (§4.1), the `pending_director` map
(`cxx_python_rules_nb.cc:8-12`), the type registry and its unguarded lazy initialization (`type_registry.h:45-61`),
`director_hook()`, and the cached module objects (`support()`, `numpy()`). All of it relies on the GIL.
`ThreadExample` calls python overrides from C++ threads, so threads are already in scope.

Suggestions: (a) one line in each class's documentation saying whether it is thread safe; (b) decide whether
free-threaded python (3.13t/3.14t, which nanobind supports with `FREE_THREADED`) is a goal for geocal. If it is, the
shared state above needs locks or per-thread storage (as in §4.1), and `Observable` needs a mutex around its lists.

### 5.2 `Exception`

- Copying can throw: `msg_` is a `std::string` and `trace_` a `std::vector<void*>` (`skeleton_exception.h:166-175`).
  Exceptions are copied by `throw e` (in `range_error`), by `std::exception_ptr`, and by the python translator
  (`cxx_python_rules_nb.h:178`). The C++ Core Guidelines ask that copying an exception never fail, and
  `std::runtime_error` uses a reference-counted message for this reason. Fix: keep the message, location and trace in
  a `std::shared_ptr<const Data>`. Copies become cheap and can't throw; `operator<<`, which is used before the throw,
  makes a new `Data` when it appends.
- `operator<<` builds a new `ostringstream` (with its locale) for every value and copies the stream state in and out.
  Acceptable for errors.
- Boost.Stacktrace instead of `std::stacktrace` is decided (gcc 14 needs `-lstdc++exp`). Revisit when the minimum gcc
  no longer needs it.

### 5.3 Class design

- `GenericObject` (`generic_object.h:50-59`) declares its destructor, so its implicit copy operations are deprecated
  and it has no move operations. Its copy is public, so a derived object can be sliced through a `GenericObject&`.
  Declare all five special members, with copy and move `protected` (Core Guidelines C.67). Derived value types such as
  `ValueSample` stay copyable.
- Classes not meant to be derived from (most samples) can be marked `final`. That documents the intent and lets the
  compiler skip virtual calls.
- Out-parameters: `ArrayTypemapSample::split` and `scaled` (`array_typemap_sample.h:138-152`) return results through
  non-const references, and the comment still uses SWIG's `OUTPUT` name. New code returns a struct or tuple (F.21).
  If the sample is there to show geocal's out-parameter pattern, say so in the comment.
- `get_scale(const double&)` and `weight(const double&)` (`cxx_python_rules_sample.h:103`, `113`) pass a `double`
  by const reference. Pass by value, unless the point is to test binding that signature (then say so).
- `[[nodiscard]]` isn't used anywhere. Use it where ignoring the result is always a mistake: the `serialize_read*`
  functions, `RasterSample::read`/`read_with_pad`, `resolve_archive_path`, `Exception::stacktrace`,
  `GenericObjectMap::get`. Not everywhere, which only adds noise.
- The observer lists are `std::list` (`observer.h:328-330`). `std::vector` with `std::erase_if` is the usual choice
  today (simpler and faster). Check that the golden archives still load and write the same bytes before changing it.
- `std::map<std::string, …>` without `std::less<>` (`generic_object_map.h:31`, `docstring_table()`): a transparent
  comparator lets lookups take a `std::string_view` or `const char*` without building a `std::string`.
- `FileReferenceSample` keeps the file name as `std::string` (`file_reference_sample.h:42`); `std::filesystem::path`
  says what it is. Its error message gives the name before `resolve_archive_path`, not the path it tried to open
  (line 39).

### 5.4 Smaller code-level items

- Nested namespace definitions (`namespace boost::serialization {`) in 6 places, e.g. `generic_object.h:9-13`,
  `weak_ptr_serialize_support.h:93-94`, `blitz_array_serialize_support.h:11-12`.
- C arrays where `std::array` fits: `skeleton_exception.cc:64`, `124`; `blitz_array.h:134-135`; `std_array.h:64`, `69`;
  `iostream.h:73`.
- `inline` is redundant on member functions defined in the class (`skeleton_exception.h:108`) and on function
  templates (`serialize_function.h:40-63`).
- Things in odd places: `generic_object_list()` and `generic_object_index()` are in `file_reference_sample.h:58-73`
  rather than `generic_object_sample.h`; `director_hook()` is defined in `weak_ptr_serialize_support.cc:60-65`.
- `iostream_swig.h` includes `<iostream>` where `<iosfwd>` is enough.
- `golden_archive_generator.cc` has 49 file-local `static` functions; an anonymous namespace is the usual form today.
- `GlobalFixture` has a virtual destructor but no virtual functions. `test_data_dir()` returns a `std::string` ending
  in `/`; a `std::filesystem::path` is clearer. `char* srcdir = getenv(…)` should be `const char*`.

### 5.5 Bindings

- The lambda that calls the C++ implementation directly when `self` is a python subclass is repeated for every
  virtual function (`director_example_nb.cc:41-56`, `exception_sample_nb.cc:34-41`, `observer_nb.h:84-107`, and
  others). The first version of this review suggested a helper template, `base_impl<Py>(&Base::func)`. That can't
  work: a call through a member function pointer is always virtual, and the non-virtual call `self.Base::func(…)`
  needs the function's name, which only a macro with the full parameter list could supply. Keep the lambdas.
- The rules depend on nanobind internals: `CXX_PYTHON_RULES_NB_OVERRIDE_REF` uses `nanobind::detail::ticket` and
  `str_hash` (`cxx_python_rules_nb.h:323-332`), `gc_shared_ptr.h` is a fork of nanobind 3.1.0's caster, and
  `blitz_array.h`/`std_array.h` use `detail::make_caster`. pixi allows nanobind `>=3.1,<4`. A `static_assert` on
  `NB_VERSION_MAJOR`/`NB_VERSION_MINOR` in `gc_shared_ptr.h` would force the re-check its header asks for at every
  nanobind upgrade.

### 5.6 Comments carry the history

The C++ comments mention SWIG 51 times, refer to plan phases (S3, S6.1, …) or review sections 27 times, and point into
`claude_notes/` from 15 files. That helps during the modernization. In a skeleton that gets copied (and in geocal
once it is done), comments should say what the code does now, with the history in git and the ChangeLog. Some text is
already out of date:

- `iostream_swig.h:8-9`: "test that we use SWIG to map …".
- `exception_sample.h:63-64`: "exercises the PythonException copy" (a SWIG-era class).
- `array_typemap_sample.h:135`: "OUTPUT/OUTPUT1/OUTPUT2 arguments".
- `configure.ac:83`: help text says "GeoCal objects"; `configure.ac:26-42` sets up `LD_LIBRARY_PATH` for AFIDS, which
  the skeleton doesn't use; `common.am:39` has an unused `CYTHON_SRC`.
- `claude_notes/implementation_plan.md` line 5: the status line still says the next phase is ST (§0 is current).

## 6. Build, tooling and tests

### 6.1 Warnings

Already strict and clean. Optional additions: `-Wnull-dereference`, `-Wduplicated-cond`, `-Wduplicated-branches`,
`-Wlogical-op` (gcc), `-Wcast-qual`, `-Wformat=2`, `-Wextra-semi`, and `-Wdeprecated-copy-dtor` (it would flag
`GenericObject`, §5.3).

### 6.2 Checking at run time

There is valgrind (`valgrind=1 lib/test_all.sh`) and an AddressSanitizer scratch build (`build-asan`, configured by
hand with `CXXFLAGS=-fsanitize=address`; the recipe and the one accepted leak are in `implementation_plan.md`, S0.8
and ST finding B), which R9 ran for the C++ and python tests. (Correction: the first version of this review said
there was nothing faster than valgrind.) What is missing:

- A `configure` option that encodes that recipe, so any build can turn it on, and UndefinedBehaviorSanitizer
  (`-fsanitize=undefined`), which hasn't been run.
- `-D_GLIBCXX_ASSERTIONS` in debug builds: bounds checks in `std::vector`, `std::array`, `std::string` and other
  precondition checks in libstdc++, at little cost.

### 6.3 clang-tidy

The project's check list is narrow (17 checks plus `bugprone-*`). The wide run (§9) shows which checks are cheap to
turn on because they have few findings: `modernize-concat-nested-namespaces` (6), `modernize-avoid-c-arrays` (8; C
APIs may need a `NOLINT`), `readability-redundant-typename` (7), `readability-redundant-inline-specifier` (1),
`performance-*` (8 in total), `misc-use-anonymous-namespace` (54, nearly all in one file),
`modernize-use-designated-initializers` (1). `cppcoreguidelines-special-member-functions` (12) after the
`GenericObject` decision in §5.3. Leave off `readability-avoid-const-params-in-decls` (all 37 findings are Boost's
`const unsigned int version` idiom) and `modernize-use-trailing-return-type`.

### 6.4 Compilers and CI

clang 22 parses all project code without errors (it is the clang-tidy front end), but every build uses gcc. The build
matrix (gcc 14, gcc 15, `--without-boost-serialization`) is run by hand (plan §0), and the repository has no CI
configuration. A CI job that builds, runs `make check`, `make format-check` and `make lint`, plus one sanitizer build
and one clang build, is normal today and would keep the code warning-free without relying on memory. Whether
github.jpl.nasa.gov has runners for it needs checking.

### 6.5 CMake reference build

The plan sets CMake aside until framework. Before framework copies `CMakeLists.txt`:

- `file(GLOB …)` for sources without `CONFIGURE_DEPENDS` (lines 44, 61, 85, 108): new files are missed until CMake is
  rerun.
- `CMAKE_SOURCE_DIR`/`CMAKE_BINARY_DIR` throughout: wrong when the project is pulled in with `add_subdirectory` or
  `FetchContent`. Use `PROJECT_SOURCE_DIR` or `CMAKE_CURRENT_SOURCE_DIR`.
- The C++ standard is set globally (line 13). `target_compile_features(skeleton PUBLIC cxx_std_23)` passes the
  requirement on to users of the library.
- No `install()`, export or package configuration, so another CMake project can't `find_package` the library.
- Release builds don't get `-DBOOST_DISABLE_ASSERTS`, which `configure.ac:21-24` adds.

### 6.6 Installed header layout

Headers are installed flat with generic names (`printable.h`, `observer.h`, `serialize_function.h`) and included as
`"x.h"`. Current practice is a project directory, `#include <skeleton/printable.h>`, to avoid name clashes. geocal uses
the same flat convention, so this is low priority.

### 6.7 Tests

The test setup is good: golden archives pin the formats, `static_assert`s check type properties, and the python tests
run in parallel. Missing tests found here: nested serialization (§4.1) and the XML string functions with a python
subclass (§4.3).

## 7. Kept on purpose (decided earlier)

From the 2026-11-06 review §6 and the plan's decisions:

- **Blitz++.** This is the largest long-term item. Upstream activity is low (1.0.2, the version in the scratch build,
  is from 2019), and parts of its interface predate modern C++ (the comma initializer `p_ = P0, P1;` in
  `virtual_diamond_example.h:48`). The `mdspan` boundary is the right way out; keep new interfaces on `mdspan`.
- **Boost.Serialization** with its macros and explicit instantiations, for archive compatibility; **Boost.Test**;
  **Boost.Stacktrace**.
- `GenericObject` as a virtual base of every bound class, `Exception` included.
- The Observer design (non-owning self pointer).
- The private-member access trick in `blitz_mdspan.cc`.
- SWIG-era names in archives and the public API (`SwigDirector_`, `geocal_object`, `IostreamSwig`, `swig_shelve`).
- `int` line and sample sizes in `RasterSample`.
- `read_shelve` changing directory while it reads (commit 975bf86), until geocal's classes use
  `resolve_archive_path`.

One addition: archives and shelves contain pickles, and Boost.Serialization doesn't validate its input, so loading one
can run arbitrary code. They must be treated as trusted input; the documentation should say so.

## 8. Python

The python code is in good shape: type hints throughout, python 3.12 syntax, `ruff format`, a mypy configuration.
Small items:

- `python/skeleton/swig_shelve.py:42`: `isinstance(value, (int, int, float, bytes, str))` has `int` twice (a python 2
  `long` left over).
- Parameters named `dir` shadow the builtin (`_nanobind_support.py:21`, `29`).
- `cxx_python_rules/nanobind/i_to_nanobind.py` (1,223 lines) is a SWIG to nanobind migration tool. It could leave
  the skeleton once geocal is converted.

## 9. Appendix: the wide clang-tidy run

Run from the scratch build directory, over every file in `compile_commands.json`, with these checks:

```
-*,modernize-*,-modernize-use-trailing-return-type,-modernize-use-std-print,performance-*,
readability-redundant-*,readability-make-member-function-const,
readability-inconsistent-declaration-parameter-name,readability-avoid-const-params-in-decls,
cppcoreguidelines-special-member-functions,cppcoreguidelines-virtual-class-destructor,
cppcoreguidelines-avoid-c-arrays,misc-use-anonymous-namespace,misc-unused-*,misc-redundant-expression,
misc-misplaced-const,misc-definitions-in-headers
```

with the project's `HeaderFilterRegex`, excluding `gc_shared_ptr.h` and `serialize_shared_ptr.h`. nanobind's own
sources, which are compiled into the module, gave about 1,100 findings and are left out below. Project code:

| Check | Findings | Note |
|---|---|---|
| `misc-use-anonymous-namespace` | 54 | 49 in `golden_archive_generator.cc` |
| `readability-avoid-const-params-in-decls` | 37 | Boost's `const unsigned int version` idiom; leave off |
| `cppcoreguidelines-special-member-functions` | 12 | `GenericObject`, the Observer classes, RAII helpers |
| `modernize-return-braced-init-list` | 8 | taste |
| `modernize-avoid-c-arrays` | 8 | §5.4 |
| `readability-redundant-typename` | 7 | C++20 allows dropping them |
| `modernize-concat-nested-namespaces` | 6 | §5.4 |
| `modernize-use-auto` | 5 | taste |
| `performance-*` | 8 | string concatenation, copies in tests, loop reserve |
| everything else | 9 | 2 or fewer each |
| **Total** | **154** | |

No errors: clang 22 parses all of it.
