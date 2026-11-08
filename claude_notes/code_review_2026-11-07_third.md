# Code review: quality and modernization, third look (2026-11-07)

**Status:** fixed, see the third fix plan (`code_review_2026-11-07_third_fix_plan.md`, T0 to T9, done 2026-11-07). Each item is fixed or listed there under "Not doing, with reasons".

**Scope.** All C++ in `lib/`, `cxx_python_rules/lib/` and `cxx_python_rules/nanobind/`, the build (`configure.ac`,
`config/m4_local/`, `CMakeLists.txt`, `.clang-tidy`), and a shorter look at the python package. This review comes
after the second review's fix plan (F0 to F10, `code_review_2026-11-07_fix_plan.md`) was finished.

**Method.** I read the code, ran clang-tidy 22 with a stricter check set that the earlier reviews didn't use
(§5.2), and wrote small python scripts against the scratch build (`build`, gcc 15, python 3.13) to confirm each bug
in §3. `make check` in `build` passes: the C++ suite, and 229 python tests passed with 1 expected failure, 0
warnings. Nothing in the repository was changed except this file.

**Fix plan.** `code_review_2026-11-07_third_fix_plan.md`. Planning it corrected four fix suggestions here (§3.1,
§3.2, §4.2, §4.3), after checking them in scratch prototypes; the corrections are marked below.

**Earlier documents.** `code_review_2026-11-06.md`, `code_review_2026-11-07.md` and their fix plans. Their decisions
(D1 to D13, the "Not doing" table of the second fix plan, the "Kept on purpose" list) are not argued again; §5.3
and §5.4 list them so they aren't taken for oversights.

## 1. Verdict

**Yes, the code mostly looks like C++ written today.** It is strict C++23 (`-std=c++23`, no GNU extensions) with
gcc 14 as the minimum compiler, and it uses newer features where they fit, not just for show: concepts, format
strings checked at compile time through a `consteval` constructor, `std::source_location`, ranges (`fold_left`,
`ranges::to`, `cartesian_product`), `std::jthread`, `mdspan` with `m[i, j]`, `<spanstream>`, designated initializers
and heterogeneous lookup. Ownership is clear: `std::shared_ptr` where python shares ownership, RAII everywhere, and
no owning raw pointers apart from a few deliberate cases. The code builds with no warnings under a strict warning
set on three compilers and passes clang-tidy (both checked here), and the F10 log has it running clean under ASan
and UBSan. The stricter clang-tidy run in §5.2 found nothing of
substance: no exception escapes, no narrowing, no unchecked `optional` access, no slicing, no thread-unsafe libc
calls.

Where it doesn't look new, the cause is design inherited from geocal and framework and kept on purpose: Blitz++,
Boost.Serialization with its macros, a virtual `GenericObject` base under every class, CRTP `Printable`, `int`
sizes, autotools (§5.3).

The two earlier reviews worked mostly on the code itself. This one found its problems at the **boundaries**,
where errors from outside (a python file object, a full disk) or the environment (python version, the copies
geocal takes) come in:

1. **Python I/O errors are silently dropped** by the stream adapters that let C++ write to python files (§3.1).
   Writing to a text-mode file writes nothing and raises nothing.
2. **Archive writes can fail without an error, or hang** (§3.2). A small archive written to a full disk reports
   success. A `.gz` archive that hits a write error spins forever at 100% CPU. geocal's own copy has the same structure.
3. **Python 3.12 passes `configure`, but the code needs 3.13** (§3.3): `Py_IsFinalizing` is new in 3.13. Every
   scratch environment has 3.13, so nothing tests the stated minimum.
4. **The `.sample` files geocal copies have drifted** from the live files in `lib/` (§3.4): the F5/F6 changes to
   `GenericObjectMap` never reached `generic_object_map.h.sample`.
5. **`from skeleton import *` doesn't give the C++ classes** (§3.5), because the generated `__all__` lists only the
   pure python names.

All five are small fixes. After them, what is left is optional modernization (§4) and items that wait for the
toolchain (§5.5).

## 2. Recommendations, in order

| # | What | Kind | Effort | geocal too? |
|---|---|---|---|---|
| 1 | `py_ostream`/`py_istream`: let python exceptions out (§3.1) | bug, confirmed | S | yes, when it copies `cxx_python_rules/` |
| 2 | Archive writes: report errors at the end of a write; make a short write throw so the gzip filter can't spin (§3.2) | bug, confirmed | S to M | yes: `swig_rules/lib/serialize_function.cc` has the same structure today |
| 3 | Require python 3.13, or add a 3-line shim for 3.12 and a 3.12 build to the matrix (§3.3) | build bug, confirmed | S | check its minimum |
| 4 | Bring the `.sample` files up to date, and generate or check them by script (§3.4) | drift, confirmed | S | yes (it copies them) |
| 5 | Add the C++ names to `__all__` in `python/skeleton/__init__.py` (§3.5) | python bug, confirmed | S | no (geocal has no `__all__`), but new projects copy it |
| 6 | Smaller fixes (§3.6) | small bugs, docs | S | some |
| 7 | Concepts on the public templates; constant-initialized Blitz member access; module statics set at import; a check that Blitz is thread safe (§4.1 to §4.4) | modernization | S each | yes |
| 8 | Add the minimum python and a failing-stream test to the checks (§6.2) | tooling | S | yes |
| 9 | Optional style sweep: `const` locals, `views::enumerate`, `format_to` (§4.5) | taste | S | no |

## 3. Bugs

### 3.1 Python exceptions in a file object's `write()` or `read()` are dropped (confirmed)

`cxx_python_rules/nanobind/iostream.h` wraps a python file object in a `std::streambuf` so C++ can write to it
through a `std::ostream` (`py_ostream`, line 79) or read through a `std::istream` (`py_istream`, line 93). When
python's `write` or `read` raises, the `streambuf` throws `nb::python_error`. But iostreams catch any exception a
`streambuf` throws, set `badbit`, and rethrow only if `badbit` is in the stream's exception mask. It isn't, so the
python error is destroyed and the C++ code carries on with a failed stream. Checked against the build tree:

| Python call | What happens |
|---|---|
| `IostreamSwig("hello world").write(open(f, "w"))` (text mode: `write` wants `str`, gets `bytes`) | No exception; the file is empty |
| `.write(obj)` where `obj.write` raises `OSError("disk full")` | No exception |
| `.read(obj)` where `obj.read` raises `OSError` | No exception; returns `' '` |

The tests only use `BytesIO`, where nothing fails. Fix:

- In both constructors, `exceptions(std::ios::badbit)`. libstdc++ then rethrows the original exception (the
  `nb::python_error`), and the exception translator restores the python error. (Correction: set the mask after
  `rdbuf(&buf)`. The `std::ostream(nullptr)` constructor sets `badbit`, and setting a mask that includes a bit
  already set throws at once. A prototype confirmed the rethrow for `<<`, `flush()` and `>>`, and that an ordinary
  end of file still doesn't throw.)
- `~py_ostream()` (line 88) calls `flush()`, which can now throw from a destructor (that is `std::terminate`). Wrap
  it in `try`/`catch`, as `~py_streambuf` already does, and flush explicitly at the end of the binding lambda
  (`iostream_swig_nb.cc:96-99`) so the normal path reports a failed last write. The usage comment at the top of
  `iostream.h` should show the explicit flush too.
- Tests: a text-mode file gives `TypeError`; a `write` or `read` that raises gives that exception.

### 3.2 Archive writes: errors at the end are lost, and a gzip write error hangs (confirmed)

`OutputFile` (`serialize_function.cc:114-133`) stacks a `std::ofstream`, an optional gzip filter and a
`std::ostream`. The archive writes into the buffers; the last buffer, and the gzip trailer, are written when
`OutputFile` is destroyed. Destructors can't report errors, and nothing checks the file after the last write.
Checked with `/dev/full`, a device that accepts `open` and fails every `write` with "no space left on device":

| Archive | Plain file | `.gz` (a symlink to `/dev/full` named `full.gz`) |
|---|---|---|
| Small (one `GenericObjectA`), XML or binary | **No exception**, nothing written | **No exception**, nothing written |
| Large (a `GenericObjectMap` of 20,000 objects), XML or binary | `RuntimeError: output stream error` (the buffer filled during the write) | **Hangs** at 100% CPU (killed after 25 s; the first try ran over 2 minutes) |

So a disk that fills during the last few KB of any archive gives a truncated file and no error. With gzip, a write
error anywhere hangs the process: the gzip filter writes to the `ofstream`'s buffer with `sputn`, which reports a
short write by its return value instead of throwing, and Boost.Iostreams retries forever. (Confirmed while
planning the fix: a C++ program with the same objects and no python hangs the same way, and stops hanging when
the sink throws on a short write.)

Fix:

- Give `OutputFile` a `close()` that `serialize_write` and `serialize_write_binary` call after the archive is
  destroyed (so its closing tags are in the stream): flush `os`, close the filter chain (writes the gzip trailer),
  `f.close()`, and throw a `std::runtime_error` naming the file if `f` failed. The destructor stays for the error
  path.
- Make a short write throw instead of returning a count. A small sink device for the chain that throws on a short
  write would do, or one of Boost.Iostreams' file devices if it throws (check that, don't assume it). Then the gzip
  filter can't spin, and the error message comes from the first failed write.
- (Correction, from a prototype: Boost.Iostreams swallows a sink's exceptions during `flush`, `reset`, `pop` and
  `close`, so the sink must also record the error for `close()` to check. And `OutputFile`'s stream must not get
  an exception mask: Boost's XML archive writes its closing tag in its destructor, and a throw there would call
  `std::terminate`.)
- Test: write small and large archives, plain and `.gz`, to a symlink to `/dev/full` in a temporary directory
  (Linux only; skip elsewhere). Each must raise, quickly.

geocal's `swig_rules/lib/serialize_function.cc` (lines 47-56, 110) has the same stack of objects, so it very
likely has both problems today, not only after it copies the new code (not tested in geocal).

### 3.3 `configure` accepts python 3.12, but the code needs 3.13 (confirmed from the headers)

`blitz_array.h:79` calls `Py_IsFinalizing()`. CPython added it in 3.13; the 3.13 header declares it under
`Py_LIMITED_API >= 0x030D0000`, and 3.12 only has the private `_Py_IsFinalizing()`. But 3.12 is the stated minimum
everywhere: `config/m4_local/skeleton_common.m4:219-233`, `CMakeLists.txt:29`, `env/linux/pixi.toml:11`,
`cxx_python_rules/nanobind/README.md` and CLAUDE.md. With 3.12 the extension module wouldn't compile. I couldn't
build it, since there is no 3.12 environment here, and that is the underlying problem: the build matrix tests the
minimum gcc (`build-gcc14`) but not the minimum python.

Fix, either:

- raise the minimum to 3.13 in all five places, or
- keep 3.12 with `#if PY_VERSION_HEX >= 0x030D0000` … `Py_IsFinalizing()` … `#else` … `_Py_IsFinalizing()`, and
  add a 3.12 scratch build to the matrix (and later to CI).

### 3.4 The `.sample` files have drifted from `lib/` (confirmed)

`cxx_python_rules/lib/*.sample` are the versions of the project-specific files that geocal copies (with `GeoCal`
for `Skeleton`). Comparing each with its `lib/` file after replacing the names:

| Sample | Missing from the sample |
|---|---|
| `generic_object_map.h.sample` | F5/F6: `std::less<>`, `std::string_view` keys, `[[nodiscard]]`, the simpler `erase`, the "Thread safety:" line |
| `generic_object.h.sample`, `blitz_array_serialize_support.h.sample` | F6: `namespace boost::serialization {` (still two nested namespaces) |
| `printable.h.sample`, the `.cc` samples | nothing (only the names differ) |

The fix plans' rule was to mirror each change into the samples by hand; the samples were last regenerated in R8.
Since the pairs differ only by names, a script can write the samples from `lib/` (a few `sed` substitutions, as R8
presumably did), and `script/check_all.sh` can check they match. Then they can't drift again.

### 3.5 `from skeleton import *` doesn't export the C++ classes (confirmed)

`python/skeleton/__init__.py` does `from skeleton_cxx.all import *` and then, in the block `mkinit` generates,
sets `__all__` (line 18) to the pure python names only. A star import uses `__all__` when it exists, so:

```
>>> from skeleton import *
>>> "IostreamSwig" in dir(), "read_shelve" in dir()
(False, True)
```

`import skeleton; skeleton.IostreamSwig` and `from skeleton import IostreamSwig` still work, which is what the
tests use. geocal's `__init__.py` has no `__all__`, so geocal isn't affected, but a project started from this
skeleton would be, and `from geocal import *` is common in geocal scripts. The same `__all__` was there in the SWIG
version on `master`. Fix: after the generated block, add the public names of `skeleton_cxx.all` to `__all__`, or
have `mkinit` (`make update-init`) not write `__all__`.

### 3.6 Smaller problems

- `python/skeleton/swig_shelve.py:98` and `:178`: `if ext not in (".gz"):`. `(".gz")` is a string, not a tuple,
  so this is a substring test. It gives the right answer for the extensions that occur, by accident. Use
  `ext != ".gz"`. The same line is in `cxx_python_rules/python/swig_shelve.py.sample`.
- `raster_sample.cc:77-78` (`read_with_pad`) and `:259` (the `SubRasterSample` constructor) compute
  `Lstart + Number_line` in `int`. With values from python near `INT_MAX` that is signed overflow, which is
  undefined behavior. `check_region` already does the same sum in `std::ptrdiff_t`; do that here too.
- `raster_sample.h:252-254`: `CalcRasterSample`'s comment says "not thread safe" twice (the F7 line was added next
  to the old sentence).
- `serialize_function.h:62-67`: `serialize_read_binary<T>` has no `[[nodiscard]]` and a redundant `inline`, unlike
  the other three read templates next to it.
- `observer_serialize_support.h:5-14`: a doc comment that switches from `///` to `//` halfway, so doxygen drops the
  second half.
- `skeleton_serialize_support.h:43-44`: `using boost::archive::polymorphic_iarchive;` (and `oarchive`) at global
  scope in a header. Only the instantiation macro in `blitz_array_serialize_support.cc` needs them; qualify the
  names there instead.
- `skeleton_common.m4:195-200` adds `/opt/afids_support/lib/pkgconfig` to `PKG_CONFIG_PATH`, an AFIDS path that
  doesn't belong in the skeleton (the same kind of leftover F9 removed from `configure.ac`).
- `skeleton_exception.cc:64-70`: `is_et_dyn` reads the ELF type as two bytes in little-endian order and doesn't
  check the ELF magic. Fine on x86_64 and aarch64 Linux; say so in the comment.
- `unit_test_support.h:12-14` defines macros named `BOOST_CHECK_MATRIX_CLOSE…`. The `BOOST_` prefix belongs to
  Boost; `SKELETON_CHECK_…` avoids a future clash.
- `observer.h:123`: `CacheInvalidatedObserver::this_obj()` returns a non-const reference to the private
  self-pointer, so any caller can reset it. `Observable` only needs to read it; return `const&`, or make it
  private with `Observable` as a friend, as `Observer` does.

## 4. Modernization opportunities still open

None of these are bugs. Each is small; together they are what is left between this code and a clean-sheet C++23
version of the same design.

### 4.1 Concepts on the public templates

The code defines one concept (`PrintableObject`) and uses `requires` in three places. Public templates that only
work for some types still fail deep inside, or only at link time:

- `serialize_read<T>` and its three siblings (`serialize_function.h:55-81`) and `GenericObjectMap::get<T>`
  (`generic_object_map.h:86-90`) `dynamic_pointer_cast` from `GenericObject`. `requires std::is_polymorphic_v<T>`
  (or `std::derived_from<T, GenericObject>`, if cross-casts to interfaces outside the hierarchy aren't wanted)
  turns a misuse into a one-line error.
- `to_blitz<Mdspan>` (`blitz_mdspan.h:139-141`) takes any type. Constrain it to an mdspan.
- The Blitz `save`/`load` templates (`blitz_array_serialize_support.h:22-25`) are declared for every `T` and `N` but
  defined only for the types and ranks in the `.cc`. Another type is a link error that names a mangled
  `boost::serialization::save<…>`. A constraint or `static_assert` listing the supported types would give a compile
  error instead, as F3.6 did for `to_blitz`.

### 4.2 Static initialization of the Blitz private-member pointer

`blitz_mdspan.cc:14` sets `BlockMember<T>::ptr` in the dynamic initializer of a static data member of a class
template. Those initializers run in no particular order relative to other files. If code in another file calls
`to_blitz` during static initialization, it uses a null member pointer, which is undefined behavior. Nothing does
that today. The "friend injection" form of the same access trick yields the member pointer at compile time with no
initializer at all: a friend function defined in the explicitly instantiated class returns the pointer. Do this
only if the code is changed anyway; it is the kind of thing that breaks quietly years later.

(Correction: the friend-injection form doesn't fit here. The friend function is defined only where the class is
explicitly instantiated, and `to_blitz`, a header template, would call it from every file that uses it. That
needs the explicit instantiations in the header, and an explicit instantiation definition may appear only once in
a program. A null check in `to_blitz` that throws a clear error is the fix; see the fix plan, T7.2.)

### 4.3 Module statics set at import

`support()` (`cxx_python_rules_nb.h:55-60`) and `numpy()` (`blitz_array.h:68-72`) import a python module the first
time they are called, inside a function-local static. If that first call runs python code that releases the GIL
(importing `numpy` for the first time), and another thread holding the GIL calls the same function, the second
thread waits for the static's guard while holding the GIL, and the first waits for the GIL. That is a deadlock.
pybind11 documents this case and added `gil_safe_call_once_and_store` for it. It is unlikely here (two threads have
to make the first conversion at the same moment), but calling both functions once in `init_module` removes it at
no cost, and helps free-threading later. The same applies to `registry()`, which is already first called at import.

(Correction: `init_module` can't call `support()` as things are. It imports `<package>._nanobind_support`, whose
first line imports `_bindings`, the module being initialized. Move that import into the functions that use it
first; see the fix plan, T7.3.)

### 4.4 Thread-safety claims depend on how Blitz was built

Many classes say their `const` functions are safe to call concurrently, and several of those copy `blitz::Array`s
(slices, return values), which changes the Blitz reference count. That is only safe if Blitz was built with
`--enable-threadsafe` (`BZ_THREADSAFE`). conda-forge's Blitz is, so the claims hold in the scratch environments, but
nothing checks it, and geocal's environments could differ. A two-line check in a library source file
(`#ifndef BZ_THREADSAFE` / `#error`), or a `configure` test, makes the assumption explicit.

### 4.5 Smaller idioms (optional)

- `misc-const-correctness` finds 49 local variables in library code that could be `const`. Cheap to fix with
  `--fix` (check the result), and worth adding to `.clang-tidy` if the maintainer likes the style.
- Index loops that use both the index and the element: `std::views::enumerate` (gcc 13), e.g. the two loops in
  `Exception::stacktrace()`. `std::format_to(std::back_inserter(res), …)` instead of `res += std::format(…)` saves a
  temporary string per line.
- `raster_sample.cc:204` and `:226`: `int status;` assigned in both branches of an `if`. A `const int status = …`
  from a conditional or a small lambda says it is set once.
- `symbolize()` (`skeleton_exception.cc:89`) has a cognitive complexity of 42 (clang-tidy's threshold is 25).
  Splitting the per-address lookup from the per-module `addr2line` run would make it easier to follow.
- `null_deleter::operator()` can be `static` (C++23), as can the stateless function objects in the bindings. Taste.

### 4.6 Not recommended

- **include-what-you-use** (`misc-include-cleaner`): 450 findings in library code, 420 of them "no header directly
  provides X". Nearly all come from the binding files getting nanobind and the rules through the umbrella header
  `skeleton_nb_opaque.h`, which is deliberate. Not worth adopting.
- **`std::expected`** for the serialization and file functions: exceptions are the right error model for a library
  driven from python, and the translator maps them well.
- **`std::print`**: decided against (it would replace stream output that has to stay a stream).

## 5. How it compares with C++ written today

### 5.1 By area

| Area | Rating | Notes |
|---|---|---|
| Language level | current | Strict C++23; the features listed in §1; nothing newer than gcc 14 |
| Ownership and lifetime | current, within the design | `shared_ptr` because python shares ownership; aliasing constructor for non-owning pointers; deliberate leaks only for python objects at exit; `unique_ptr` until a capsule takes over |
| Error handling | current, except at I/O boundaries | Exception copies can't throw, format strings checked at compile time, source location and stack trace; but §3.1 and §3.2 |
| Class design | current | Special members declared deliberately (protected copy in `GenericObject`, deleted copies with `static_assert` tests), `override` everywhere, `final` on helper classes (not on samples, by decision) |
| Generic code | mixed | `if constexpr` and one template instead of a macro line per type; but few concepts (§4.1), and macros where Boost.Serialization needs them |
| Concurrency | documented | A "Thread safety:" line on each class, per-thread state instead of globals, the GIL released around `ThreadExample`; free-threading is a later goal (D9) |
| Python interop | current | nanobind, zero-copy numpy through `mdspan`, keyword names from C++, docstrings from doxygen, a `static_assert` on the nanobind version for the code that uses its internals |
| Build and tooling | current, CI missing | Warning-clean on gcc 14, gcc 15 and clang 22 with a strict set, sanitizers, clang-tidy, clang-format, golden archives, a modern CMake reference build. CI is deferred (D11); the minimum python isn't tested (§3.3) |
| Comments and docs | current | History moved out of comments (F9); trusted-input warning; thread-safety section |

### 5.2 Clang-tidy with a stricter profile

To check "looks like today" with a tool rather than by eye, I ran clang-tidy with checks the project and the
earlier wide runs didn't use (list in §7). Findings in library and binding code, tests left out:

| Check | Findings | Assessment |
|---|---|---|
| `misc-const-correctness` | 49 | locals that could be `const`; optional (§4.5) |
| `readability-convert-member-functions-to-static` | 37 | sample functions that exist to be bound as members; leave |
| `cppcoreguidelines-macro-usage` | 21 | Boost.Serialization and export macros, the X-macro type list; needed |
| `misc-non-private-member-variables-in-classes` | 16 | `Observable`'s protected lists, `Exception::Data`, public sample members; deliberate |
| `cppcoreguidelines-owning-memory` | 4 | the deliberate leaks and capsule deleters |
| `pro-type-const-cast`, `pro-type-reinterpret-cast` | 4, 3 | each has a comment saying why |
| `cppcoreguidelines-avoid-non-const-global-variables` | 3 | atomics and a `thread_local` in anonymous namespaces; fine |
| `readability-else-after-return` | 3 | inside the `CXX_PYTHON_RULES_NB_OVERRIDE_REF` macro |
| `cppcoreguidelines-init-variables` | 2 | `int status;` (§4.5) |
| `readability-make-member-function-const` | 2 | `counter()` (deliberate) and `IostreamSwig::read` |
| `readability-function-cognitive-complexity` | 1 | `symbolize` (§4.5) |
| `cppcoreguidelines-rvalue-reference-param-not-moved` | 1 | `from_cpp(Value&&)`, nanobind's caster interface |

No findings at all for `bugprone-exception-escape`, `bugprone-unchecked-optional-access`,
`cppcoreguidelines-narrowing-conversions`, `cppcoreguidelines-slicing`, `cppcoreguidelines-missing-std-forward`,
`concurrency-mt-unsafe`, `cert-err58-cpp`, `modernize-use-constraints`, `modernize-use-std-numbers` or
`modernize-use-starts-ends-with` in library code. That is unusually clean.

### 5.3 What a project started today would do differently (kept on purpose)

These are the reasons the code doesn't look fully new. All were decided earlier, for compatibility with geocal and
framework and their archives; they are listed so a reader knows they were considered.

| Today's default | Here | Why kept |
|---|---|---|
| Value types; `unique_ptr`; `shared_ptr` only when shared | `shared_ptr` everywhere, a virtual `GenericObject` base with `enable_shared_from_this` under every bound class, `Exception` included | python shares ownership; the type registry needs the common base |
| `std::mdspan` / `std::vector` / Eigen / xtensor | Blitz++ (last release 2019; comma initializers like `stored_ = 1, 2, 3`) | geocal's API; the `mdspan` boundary is the way out, and new interfaces use it |
| A serialization library without macros, or reflection later | Boost.Serialization: export keys, `serialize` templates, explicit instantiation | existing archives must load (pinned by the golden tests) |
| `std::formatter` per class, or a non-template interface | CRTP `Printable<T>` with `desc()` for python | geocal's classes all derive from it; `std::format` works through it |
| `std::size_t`/`std::ptrdiff_t` sizes, a region type | `int Lstart, int Sstart, int Number_line, …` | geocal's API (`bugprone-easily-swappable-parameters` is off for this) |
| CMake or Meson; `#include <skeleton/x.h>` | autotools; a flat header layout | geocal; a modern CMake reference build exists for framework |
| `std::stacktrace` | Boost.Stacktrace | gcc 14 still needs `-lstdc++exp` |
| Owning observers or signals | `Observer` with a non-owning self-pointer | framework's and geocal's design |
| Fresh names | SWIG-era names (`SwigDirector_`, `geocal_object`, `IostreamSwig`, `swig_shelve`) | archives and python callers |

### 5.4 Earlier decisions (not argued again)

From the second fix plan's "Not doing" table: no `final` on the sample classes; `test_data_dir()` stays a string;
`FileReferenceSample` keeps a string; flat installed headers; `i_to_nanobind.py` stays until geocal is converted;
free-threaded python later (D9); CI deferred (D11). Also D6 (`bytes` for the XML strings), D7 (pickles stay raw in
XML) and D13 (observer lists as `std::vector`).

### 5.5 Waiting for the toolchain

| Feature | When | What it would replace |
|---|---|---|
| `std::mdspan`, `std::submdspan` | gcc 16 minimum | the Kokkos copy; a one-file change by design (`cxx_python_rules_mdspan.h`) |
| `std::stacktrace` without an extra library | when the minimum gcc no longer needs `-lstdc++exp` | Boost.Stacktrace and the `addr2line` code |
| `import std` and named modules | when gcc, clang, Boost, Blitz, nanobind and autotools handle them | header includes; not soon |
| Contracts (C++26) | C++26 compilers | `range_check` and the hand-written precondition checks |
| Static reflection (C++26) | C++26 compilers | much of the hand-written `serialize()` functions and binding code; the largest long-term simplification |
| Free-threaded python | D9 | the GIL assumptions listed in `cxx_python_rules/README.md` "Threads" |

## 6. Tooling and tests

### 6.1 What is good

The test setup is strong: golden archives pin the binary and XML formats byte for byte, `static_assert`s check type
properties, the python tests run in parallel against the build tree, and `script/check_all.sh` runs everything over
several builds. It is rare for a project of this size to build warning-clean on three compilers and run clean under ASan and
UBSan.

### 6.2 What the bugs in §3 show

All five §3 bugs sit where the tests use only the happy path: a `BytesIO` that never fails, a disk that never fills,
the newest python, the live files instead of the copies, `import skeleton` instead of a star import. Tests that
would have caught them:

- a python file object whose `write`/`read` raises, and a text-mode file (§3.1);
- archive writes to a failing device, with a timeout so a hang fails the test (§3.2);
- a build with the minimum python (§3.3);
- `check_all.sh` comparing the generated samples with the checked-in ones (§3.4);
- `from skeleton import *` followed by a C++ class name (§3.5).

### 6.3 Other

- **Hardening flags.** The OpenSSF compiler-hardening guide recommends `-D_GLIBCXX_ASSERTIONS` in release builds
  too, not only debug, plus `-D_FORTIFY_SOURCE=3` and `-fstack-protector-strong` (gcc 14 bundles these as
  `-fhardened`). conda's compiler activation already adds some of them. Optional: a `--enable-hardening` option.
- **CI** is still the main missing piece (deferred, D11). When it is set up, include the minimum python, as the
  compiler matrix already includes the minimum gcc.

## 7. Appendix: what I ran

- `make -j12 check` in `build`: the C++ suite passed; 229 python tests passed, 1 xfailed; 0 compiler warnings.
- The stricter clang-tidy run, over the files in `build/lint_files.txt` with `script/run_clang_tidy.sh`:

  ```
  -*,misc-const-correctness,modernize-use-nodiscard,cppcoreguidelines-macro-usage,
  cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-init-variables,
  cppcoreguidelines-narrowing-conversions,cppcoreguidelines-rvalue-reference-param-not-moved,
  cppcoreguidelines-missing-std-forward,cppcoreguidelines-pro-type-static-cast-downcast,
  cppcoreguidelines-avoid-const-or-ref-data-members,readability-convert-member-functions-to-static,
  readability-make-member-function-const,readability-qualified-auto,readability-else-after-return,
  readability-use-anyofallof,modernize-use-constraints,modernize-use-std-numbers,
  modernize-use-starts-ends-with,modernize-use-integer-sign-comparison,modernize-use-scoped-lock,
  cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-const-cast,
  cppcoreguidelines-pro-type-reinterpret-cast,cert-err58-cpp,bugprone-exception-escape,
  concurrency-mt-unsafe,misc-include-cleaner,cppcoreguidelines-noexcept-move-operations,
  cppcoreguidelines-slicing,cppcoreguidelines-interfaces-global-init,
  misc-non-private-member-variables-in-classes,readability-function-cognitive-complexity,
  cppcoreguidelines-avoid-do-while,cppcoreguidelines-pro-type-vararg,
  readability-static-accessed-through-instance,readability-container-size-empty,
  readability-simplify-boolean-expr,misc-misleading-identifier,bugprone-unchecked-optional-access
  ```

  983 findings in total: 696 `misc-include-cleaner` (§4.6) and the rest as in §5.2, plus tests.
- §3.1: `IostreamSwig.write`/`read` with a text-mode file, and with objects whose `write`/`read` raise, against
  `build/nb_pkg` (the build tree modules, as `make check` uses them).
- §3.2: `serialize_write` and `serialize_write_binary` of a small and a large object to `/dev/full` and to a symlink
  to it named `full.gz`, each case in its own process with a 25 s timeout.
- §3.3: `include/python3.13/pylifecycle.h` in the scratch environment (the `0x030D0000` guard); no 3.12
  environment exists here to build with.
- §3.4: `diff` of each `.sample` with its `lib/` file after replacing `GeoCal`/`GEOCAL` and `Skeleton`/`SKELETON`.
- §3.5: `from skeleton import *` against `build/nb_pkg`.
- §4.4: `BZ_THREADSAFE` in the scratch environment's `blitz/gnu/bzconfig.h` (defined; pthread mutexes).
