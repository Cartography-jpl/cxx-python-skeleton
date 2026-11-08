# Plan: fix what the third code review found

**Date:** 2026-11-07
**Status:** Ready; decisions D14 to D18 made (the recommendations). Set aside by the maintainer on 2026-11-07;
not started, no code changed. **Then scheduled (maintainer, 2026-11-07): at least T0–T2 are done before geocal's
Phase 0**, because T2 fixes an archive write bug that geocal has today (confirmed in geocal, see geocal's
`claude_documents/cpp_modernization_and_library_plan.md` §0.2); geocal ports T2 in its Phase 0 step 9. The rest
(T3–T9) may follow then or later; geocal takes those with its Phase 6 copy of `cxx_python_rules/`.
**Input:** `claude_notes/code_review_2026-11-07_third.md` ("the review"; its section numbers are given as §3.1 and
so on).
**Goal:** when every step is done, each item in the review is fixed or has a written reason for being left. The
archive formats and test results stay the same, and the python API changes only as listed in "Intended behavior
changes".

Rules (as in the earlier fix plans): work on `modernize`; stage with `git add`, never commit; `clang-format -i` on
C++ you change and `ruff format` on python you change; a step is done when its checks pass and the full test suite
passes. After each phase, add a line to the progress log at the end of this file, and a row to the "Notes for geocal"
table in `implementation_plan.md` §0 for anything geocal should also do. From T4 on, update the `.sample` files with
the script T4 adds (not by hand).

Phases are named T0 to T9 (T for third; the earlier plans used R and F).

## Corrections to the review found while planning

I checked the fixes in small scratch programs before writing the steps (outside the repository, in the session
scratch directory). Four things differ from the review, which is corrected to match:

1. **§3.1.** Set the stream's exception mask after `rdbuf(&buf)`. The `std::ostream(nullptr)` constructor sets
   `badbit`, and setting a mask that includes a bit already set throws at once. Checked: with
   `exceptions(std::ios::badbit)`, libstdc++ rethrows the `streambuf`'s own exception from `<<`, `flush()` and `>>`,
   and an ordinary end of file still doesn't throw.
2. **§3.2.** A sink that throws on a short write isn't enough by itself: **Boost.Iostreams swallows a sink's
   exceptions during `flush`, `reset`, `pop` and `close`** (checked; the sink was called 6 times and nothing came
   out). So the sink must also record the error, and `OutputFile` checks the record after closing. And `OutputFile`'s
   own stream must **not** get an exception mask: Boost's XML archive writes `</boost_serialization>` in its
   destructor (and the text archive writes a newline in its destructor), so a throw there would call
   `std::terminate`. The prototype also settles the cause of the hang: the same objects in C++ without python hang
   with an `std::ofstream` sink, and stop hanging with a sink that throws.
3. **§4.2.** The compile-time ("friend injection") form of the private member access doesn't fit. The friend
   function is only defined where the class is explicitly instantiated, and `to_blitz`, a header template, would
   call it from every file that uses it. That needs the explicit instantiations in the header, and an explicit
   instantiation definition may appear only once in a program. T7.2 adds a null check instead, which turns the
   (unlikely) use during static initialization into a clear exception.
4. **§4.3.** `init_module` can't simply call `support()`: that imports `<package>._nanobind_support`, whose first
   line imports `_bindings`, the module being initialized. T7.3 moves that import into the functions first.

## Intended behavior changes

Everything else must stay the same, which the T0 snapshot checks.

| Change | Step | Who notices |
|---|---|---|
| An exception raised by a python file object's `write`/`read` comes out of the C++ call that used it (a text-mode file gives `TypeError`) | T1 | Callers that passed a file that fails. They lost data silently before. |
| `serialize_write`/`serialize_write_binary` raise when any part of the write fails, including the last buffer and the gzip trailer; a gzip write error raises instead of hanging | T2 | Nobody, unless a disk fills up |
| Python 3.13 is the minimum (D14) | T3 | Builds with 3.12 fail at configure (they failed in the compile before) |
| `from skeleton import *` also gives the C++ classes | T5 | Scripts that use a star import |
| `read_with_pad` with offsets near `INT_MAX` gives the fill value (was undefined behavior); `SubRasterSample` rejects such a region | T6.2 | Nobody in practice |
| `CacheInvalidatedObserver::this_obj()` returns a `const` reference | T6.9 | C++ code that reset it (none in the skeleton) |
| `BOOST_CHECK_MATRIX_CLOSE` becomes `SKELETON_CHECK_MATRIX_CLOSE` | T6.7 | The skeleton's tests; geocal keeps its own name |
| `serialize_read<T>`, `GenericObjectMap::get<T>` and `to_blitz` with a wrong type fail with a short compile error; serializing a Blitz array of an unsupported type is a compile error, not a link error | T7.1 | C++ code with that mistake |
| `to_blitz` throws if it runs before `blitz_mdspan.cc`'s initializers (during static initialization) | T7.2 | Nobody today |
| numpy is imported when the extension module is imported | T7.3 | Import time (numpy is a required dependency) |
| `configure` reports whether Blitz is thread safe, and warns if not | T7.4 | Builds with a Blitz built without `--enable-threadsafe` |
| `--enable-debug` in an env with conda's compiler activation drops `-DNDEBUG` and adds `-DBZ_DEBUG` (Blitz bounds checks) | T6.11 | Debug builds: header `assert()`s and Blitz bounds checks are on; an out-of-bounds Blitz access stops the program |

## Decisions (maintainer, 2026-11-07: the recommendation in each case)

- **D14. Python minimum (§3.3). Decided: A, require 3.13** (T3 first checks the condition below). *A:* require 3.13 everywhere (`skeleton_common.m4`, `CMakeLists.txt`,
  `env/linux/pixi.toml`, `cxx_python_rules/nanobind/README.md`, CLAUDE.md). *B:* keep 3.12: a version check around
  `Py_IsFinalizing`/`_Py_IsFinalizing`, plus a new scratch build `build-py312` in the matrix (and later in CI).
  *Recommendation: A, unless a geocal or framework environment still runs 3.12.* Every scratch environment here is
  on 3.13, and a minimum that no build tests drifts again. B costs one more build directory to keep running.
- **D15. Samples (§3.4). Decided: A, a script.** *A:* a script that writes the samples from `lib/` and checks them, run by
  `script/check_all.sh`. *B:* bring them up to date by hand once. *Recommendation: A.* Six of the nine pairs already
  match exactly after replacing the names, so the script is short, and it stops the drift for good.
- **D16. `__all__` (§3.5). Decided: A, add the C++ names.** *A:* keep `mkinit`'s list and add the C++ names after it. *B:* run `mkinit --noall`
  and drop `__all__`, as geocal has none. *Recommendation: A.* It keeps an explicit list, which ruff and mypy use;
  B also exports the submodule names `dummy` and `swig_shelve`.
- **D17. Hardening flags (§6.3). Decided: A, not now.** *A:* not now; record it in the deferred list with CI. *B:* a
  `--enable-hardening` option (`-D_FORTIFY_SOURCE=3`, `-fstack-protector-strong`, `-D_GLIBCXX_ASSERTIONS` in
  release builds; or gcc's `-fhardened`) and the same in CMake. *Recommendation: A.* conda's compiler activation
  already adds part of it, and release-mode library assertions should be measured on geocal's hot loops first.
- **D18. Style sweep (§4.5). Decided: B, the targeted changes only.** *A:* the targeted changes in T8 plus `misc-const-correctness` (fix the 49 findings,
  add the check to `.clang-tidy`). *B:* the targeted changes only. *C:* skip T8. *Recommendation: B.* The targeted
  items make specific code clearer; a `const`-everything check is taste, and geocal shares `.clang-tidy`, so it would
  have to be cleaned for it too.

## T0. Baseline

1. Rebuild `build`, `build-gcc14`, `build-noserial`, `build-asan`, `build-clang` and `build-cmake` from `HEAD` and
   record the counts (now: 229 passed, 1 xfailed, 0 warnings).
2. API snapshot as `baseline/api_t0.txt` (`script/api_snapshot.py`); golden archives identical
   (`make golden-check`).
3. Save the stricter clang-tidy command from the review's appendix as `baseline/tidy_strict.sh`, and its output as
   `baseline/tidy_strict_t0.txt` (983 findings, 696 of them `misc-include-cleaner`).
4. Add the reproductions as tests marked `xfail(strict=True)`, so T1, T2 and T5 show them going from failing to
   passing:
   - `iostream_swig_test.py`: `write` to a text-mode file raises `TypeError`; `write` to an object whose `write`
     raises `OSError` raises that `OSError`; `read` from an object whose `read` raises `OSError` raises it.
   - New `serialize_write_error_test.py`: small and large objects (a `GenericObjectMap` of 20,000 `GenericObjectA`),
     XML and binary, plain and `.gz`, written to a symlink to `/dev/full` in `tmp_path` (named `full.xml`,
     `full.xml.gz` and so on). Each write must raise `RuntimeError`. Each case runs in a subprocess
     (`subprocess.run(..., timeout=20)`), so the gzip hang fails the test instead of blocking the suite; pytest-timeout
     isn't in the environment and `--strict-markers` is on. Skip if `/dev/full` doesn't exist. The large plain cases
     already pass, so they aren't `xfail`.
   - New `package_test.py`: after `from skeleton import *`, `IostreamSwig` and `read_shelve` are defined.

## T1. Stream adapters let python errors out (§3.1)

In `cxx_python_rules/nanobind/iostream.h`:

1. `py_ostream` and `py_istream` constructors: `exceptions(std::ios::badbit)` after `rdbuf(&buf)` (correction 1).
   Comment: why the mask is there, and that end of file still doesn't throw.
2. `~py_ostream()`: put `flush()` in `try`/`catch`, as `~py_streambuf` does. For a `nb::python_error` caught there,
   call `discard_as_unraisable`, so python prints "Exception ignored in …" rather than losing it silently. That is
   the fallback; the next step is the normal path.
3. Flush explicitly in the binding: `iostream_swig_nb.cc` calls `os.flush()` after `self.write(os)`. Update the
   usage comment at the top of `iostream.h` and the line in `cxx_python_rules/nanobind/README.md` ("stream
   arguments") to show it.
4. `i_to_nanobind.py` (the SWIG to nanobind tool geocal will use) generates `py_ostream` arguments
   (`call_args`, around line 733). Have it emit a flush of each `py_ostream` after the call: it already takes a
   `post` list that is unused. For a non-void function, keep the result in a local first. The generator knows the
   return type, so it can pick the void or non-void form.
5. While here (not in the review): `py_streambuf::flush` ignores the count python's `write` returns. A raw file
   (`open(f, "wb", buffering=0)`) may write less than it was given. Loop until everything is written when `write`
   returns an `int` smaller than the size.

Checks: the T0 iostream tests pass (remove `xfail`); the existing `BytesIO` tests pass; a new test writes 100 KB
through a raw file object (`buffering=0`) and reads it back the same; ASan build passes.

## T2. Archive writes report every error, and can't hang (§3.2)

In `cxx_python_rules/lib/serialize_function.cc` (the design was checked in a scratch prototype: every failure
reported, no hang, normal plain and `.gz` files unchanged and valid):

1. A sink device for the Boost.Iostreams chain, inside `OutputFile`. It holds a pointer to the `std::ofstream` and a
   shared error record (the file name and the first error; `std::shared_ptr`, because the chain copies the sink).
   `write` passes the data to `f.rdbuf()->sputn`; on a short write it records `std::generic_category().message(errno)`
   (or "write failed" if `errno` isn't set) and throws `std::runtime_error`. The throw ends the gzip filter's
   retry loop; the record covers the places where Boost swallows the throw (correction 2).
2. `OutputFile::close()`: flush the stream, `sb.reset()` (closes the filter chain: writes the gzip trailer),
   `f.close()`, then throw `std::runtime_error("Trouble writing file <name>: <reason>")` if the record has an error or
   `f.fail()`. No exception mask on `OutputFile`'s `std::ostream` (correction 2: the XML archive's destructor writes
   to it). Comment both points in the class.
3. `serialize_write` and `serialize_write_binary` call `out.close()` after `write<…>()` returns. By then the archive
   object has been destroyed, so its closing tags are in the stream.
4. Error message: if the archive throws while the record has an error (Boost reports the failure as
   `archive_exception` "output stream error"), throw the recorded error instead, so the message names the file and
   the reason. A small `catch` in `serialize_write*` that asks `OutputFile` for its record does this.
5. `InputFile` is unchanged; read errors already throw (`gzip_error`, archive exceptions).
6. The in-memory functions (`serialize_write_string`, `serialize_write_binary_string`) write to an
   `std::ostringstream` and are unchanged.

Checks: the T0 `/dev/full` tests pass (remove `xfail`), and each finishes in well under the 20 s timeout. Add the same
cases to `serialize_function_test.cc` (C++: a symlink to `/dev/full` in `TempDir`, skipped if it doesn't exist): with
the fix they can't hang, so no timeout is needed. Golden archives identical (plain files are byte for byte the same).
A `.gz` written through the new path decompresses to the same bytes as before (`gzip -t` plus comparing with a
plain write). ASan and UBSan builds pass.

## T3. Python minimum version (§3.3; D14)

0. First check the condition of D14: the python version in geocal's environments (`env/linux/pixi.toml`,
   `env/osx`, and what its lock file resolves to) and framework's. If one of them runs 3.12, stop and ask the
   maintainer before raising the minimum (option B, a version check plus a `build-py312` build, is the
   alternative).
1. `config/m4_local/skeleton_common.m4`: `SKELETON_PYTHON_MINIMUM` checks `(3, 13)`. The comment gives both
   reasons: `PyErr_GetRaisedException` (3.12) and `Py_IsFinalizing` (3.13). `./bootstrap`.
2. `CMakeLists.txt`: `find_package(Python 3.13 …)`. `env/linux/pixi.toml`: `python = ">=3.13"` (then
   `pixi lock`).
3. `cxx_python_rules/nanobind/README.md` (the requirements list) and CLAUDE.md ("python >= 3.12").
4. Check: configure in `build` prints `whether … is at least python 3.13... yes`. To see the error path, run configure
   once with `PYTHON` set to a fake script that reports 3.12, rather than building a real 3.12 environment.

## T4. The `.sample` files (§3.4; D15)

1. `script/samples.py`, with `--check` (the default) and `--write`. It has the list of pairs (live file → sample):
   `lib/generic_object.{h,cc}`, `lib/generic_object_map.{h,cc}`, `lib/printable.h`,
   `lib/blitz_array_serialize_support.{h,cc}`, `lib/skeleton_serialize_support.h` → `serialize_support.h.sample`,
   `python/skeleton/swig_shelve.py`, `python/tests/unit/swig_shelve_test.py`. It applies the substitutions R8 used
   (`Skeleton`→`GeoCal`, `SKELETON`→`GEOCAL`, `skeleton_cxx`→`geocal_cxx`, `skeleton`→`geocal`); checked: six pairs
   match exactly that way today.
2. Lines that only the sample has go between `// sample only:` and `// end sample only` (`#` in python). `--check`
   ignores them, and `--write` copies them from the existing sample to the same place in the new one. Today one
   pair needs this: `serialize_support.h.sample` includes `boost/serialization/boost_array.hpp`, since geocal still
   uses `boost::array`. T6.6 adds a second block to the same file.
3. `--write`, then review the diff: it should add exactly what the review lists (F5/F6 in
   `generic_object_map.h.sample`, the nested namespaces in two headers) and nothing else.
4. `script/check_all.sh`: run `script/samples.py --check` once per invocation (not per build directory). Add a line
   to its summary table.
5. CLAUDE.md and `cxx_python_rules/README.md`: the samples are generated; edit the `lib/` file and run
   `script/samples.py --write`.

Checks: `script/samples.py --check` passes; changing a line in `lib/printable.h` makes it fail; `check_all.sh` fails
too.

## T5. `from skeleton import *` (§3.5; D16)

In `python/skeleton/__init__.py`, end the generated block with `# </AUTOGEN_INIT>`, so `mkinit` keeps what
follows (check with `make -C python update-init` that it does). After the marker:

```python
# The C++ classes too: mkinit's __all__ above lists only the python names.
import skeleton_cxx.all as _cxx_all

__all__ += [n for n in dir(_cxx_all) if not n.startswith("_")]
```

Checks: the T0 `package_test.py` passes (remove `xfail`); `make -C python update-init` leaves the file as it
is; the installed package passes it too (`make installcheck`). `python/README_developer.md` (if it describes
`update-init`) and the comment at the top of `__init__.py` say why.

## T6. Smaller fixes (§3.6)

1. **`swig_shelve.py`** (lines 98, 178): `if ext != ".gz":`. The behavior is the same for every extension used.
   The sample follows through T4.
2. **Overflow in `raster_sample.cc`**: in `read_with_pad`, compute `l1`/`s1` in `std::ptrdiff_t` (as
   `check_region` does) and convert back to `int` after `std::min`; the same for the `SubRasterSample` constructor's
   check. The "C library" stand-ins (`legacy_read_block`/`legacy_write_block`) get the same, for consistency.
   Tests (C++ and python): `read_with_pad(INT_MAX - 5, 0, 10, 10)` returns all fill values; `SubRasterSample` with
   `Lstart = INT_MAX - 5` throws. Run them in `build-asan` (UBSan reports overflow).
3. **`raster_sample.h:252-254`**: delete the old sentence, keep the "Thread safety:" line.
4. **`serialize_function.h:62-67`**: `[[nodiscard]]` on `serialize_read_binary<T>`, drop `inline`. A test that
   calls it only to check that it throws needs `std::ignore =` (as in F5.5).
5. **`observer_serialize_support.h:5-14`**: `///` on every line of the comment.
6. **`skeleton_serialize_support.h:43-44`**: remove the two global `using` declarations; qualify
   `boost::archive::polymorphic_[io]archive` in the instantiation macro of `blitz_array_serialize_support.cc`. In
   `serialize_support.h.sample`, keep them in a `// sample only:` block that says geocal's code uses the short
   names (5 files in geocal's `lib/`). geocal can qualify those later.
7. **`unit_test_support.h:12-14`**: rename to `SKELETON_CHECK_MATRIX_CLOSE_TOL` / `SKELETON_CHECK_MATRIX_CLOSE`;
   update the 11 uses in `lib/`. (geocal keeps its own macro names; no sample is involved.)
8. **`skeleton_common.m4:195-200`**: drop `/opt/afids_support/lib/pkgconfig`, and change the comment. Keep
   `$THIRDPARTY` and `$CONDA_PREFIX`. `./bootstrap`.
9. **`observer.h:123`**: `this_obj()` returns `const std::shared_ptr<CacheInvalidatedObserver>&`. Its three uses in
   `Observable` only read it.
10. **`skeleton_exception.cc:64-70` (`is_et_dyn`)**: use `<elf.h>`: check `ELFMAG`, read `e_type` (offset 16 for
    both ELF classes) in the byte order `e_ident[EI_DATA]` gives, and compare with `ET_DYN`. Then it is correct on
    any Linux, not just little-endian. The stack trace test still names functions.
11. **Debug flags in environments with conda's compiler activation** (added 2026-11-07, after the decisions; found
    by the review of geocal's modernization plan, checked here). `AC_ENABLE_DEBUG` (`config/m4/ac_enable_debug.m4`)
    replaces `CXXFLAGS`/`CFLAGS` with conda's `DEBUG_CXXFLAGS`/`DEBUG_CFLAGS` but leaves `CPPFLAGS` alone, and
    conda's activation puts `-DNDEBUG -D_FORTIFY_SOURCE=2 -O2` in `CPPFLAGS`. The S1.2 fix only removed the same
    flags coming from Blitz's `.pc` file. So `--enable-debug` builds in activated envs (`build-gcc14`,
    `build-rpath`, `build-asan` on frenchpeas; checked in their `Makefile`s) still compile with `-DNDEBUG`, which
    turns off `assert()` in library headers (Boost's `BOOST_ASSERT`; neither the skeleton nor geocal calls
    `assert()` itself). `-O2` is harmless there (the later `-Og` in `CXXFLAGS` wins), and `_GLIBCXX_ASSERTIONS`
    doesn't depend on `NDEBUG`. The same branch also uses conda's `DEBUG_CXXFLAGS`, which have no `-DBZ_DEBUG`, so
    those builds have **no Blitz bounds checking** (the non-activated fallback branch adds it). Fix in
    `ac_enable_debug.m4` (a shared m4 file, like the S1.2 change): in the conda branch, `CPPFLAGS="$DEBUG_CPPFLAGS"`
    when it is set (conda's is `-D_DEBUG -D_FORTIFY_SOURCE=2 -Og -isystem $PREFIX/include`), otherwise strip
    `-DNDEBUG` from `CPPFLAGS`; and add `-DBZ_DEBUG` in both branches. Check: in `build-gcc14` a compile line has no
    `-DNDEBUG` and has `-DBZ_DEBUG`, a release build is unchanged, and the full suite passes in `build-gcc14` and
    `build-asan` (Blitz bounds checks may find something; fix it). geocal copies the file.

Checks: full suite; golden identical; `script/samples.py --check`.

## T7. Modernization (§4.1 to §4.4)

**T7.1 Concepts on the public templates (§4.1).**
- `serialize_function.h`: `requires std::derived_from<T, GenericObject>` on `serialize_read<T>`,
  `serialize_read_binary<T>`, `serialize_read_string<T>`, `serialize_read_binary_string<T>`; the same on
  `GenericObjectMap::get<T>`. Before choosing `derived_from` over `std::is_class_v`, grep geocal for
  `serialize_read.*<` and `->get<` with a type outside the `GenericObject` hierarchy. If there is one, use
  `std::is_class_v<T>` instead (cross-casts work with any class).
- `blitz_mdspan.h`: a concept for `to_blitz`'s argument (a requires-expression: `element_type`, `rank()`, `extent`,
  `stride`, `data_handle`, `mapping().required_span_size()`).
- `blitz_array_serialize_support.h`: move the type list from the `.cc` into the header as an X-macro
  (`SKELETON_BLITZ_SERIALIZE_TYPES`, ranks 1 to 5, and `std::string` for ranks 1 and 2), and use it both for the
  explicit instantiations in the `.cc` and for a `static_assert` in the header's `serialize` template that names
  the list. This is the pattern F3.6 used for `to_blitz`.
- Tests: compile-failure checks by hand (as F4 did), recorded in the progress log: `serialize_read<int>`,
  `to_blitz(42)`, a `blitz::Array<std::complex<double>, 1>` member in a serialized class. A C++ test that serializes
  one array of every listed type and rank, so a list entry without an instantiation fails to link the test.

**T7.2 `to_blitz` before static initialization (§4.2, correction 3).** In `to_blitz`, if
`detail::BlockMember<T>::ptr` is null, throw `std::runtime_error` saying `to_blitz` was called during static
initialization, before `blitz_mdspan.cc`'s initializers ran. Extend the comment at `blitz_mdspan.cc:14` to say why
the initializer order matters and why the compile-time form wasn't used. No test (it needs a static initializer in
another file, and the order isn't defined).

**T7.3 Module statics set at import (§4.3, correction 4).**
1. `cxx_python_rules/_nanobind_support.py`: import `_bindings` inside the three functions that use it
   (`_new_from_serialization`, `_new_from_serialization_dir`, `archive_base_directory_scope`), not at the top. Then
   the module can be imported while `_bindings` is still being initialized. A comment says why.
2. `cxx_python_rules_nb.cc`, `init_module`: call `support()` and `CXX_PYTHON_RULES_NAMESPACE::nbblitz::numpy()`
   once, so both statics are set during import, under the import lock. The downstream module calls `init_module` as
   well, so its own copies are set the same way.
3. `cxx_python_rules/README.md` "Threads": add that the module objects are set at import.
4. Checks: the full suite, including `downstream_test.py` and the pickling tests; `python -c "import skeleton_cxx"`
   in a fresh process; `import skeleton_cxx._bindings` as the first import. If importing `_nanobind_support` from
   `init_module` still causes trouble in some import order, fall back: keep `support()` lazy, and write in
   `iostream.h`/`cxx_python_rules_nb.h` why its first call can't release the GIL (the package imports
   `_nanobind_support` right after `_bindings`). The numpy part stays either way.

**T7.4 Blitz thread safety (§4.4).** In `skeleton_common.m4`, after `AC_BLITZ`: compile a test program with
`#include <blitz/blitz.h>` and `#ifndef BZ_THREADSAFE` / `#error`. On failure, `AC_MSG_WARN` that classes copying
Blitz arrays aren't safe to use from several threads. Add a line to the configure summary ("Blitz thread safe:
yes/no"). A warning, not an error, so a geocal environment with a different Blitz still builds. Mention it in the
README "Threads" section.

Checks: full suite; golden identical; `script/samples.py --check` (T7.1 changes a sampled file).

## T8. Style (§4.5; D18)

The targeted items (D18):
1. `Exception::stacktrace()` (`skeleton_exception.cc`): `std::views::enumerate` for the two index loops, and
   `std::format_to(std::back_inserter(res), …)`.
2. `raster_sample.cc:204`, `:226`: `const int status = …` (a conditional expression, or a small lambda for the copy
   branch).
3. `symbolize()`: split into one function that maps addresses to modules and offsets, and one that runs `addr2line`
   for a module and fills in the names. Check: the stack trace test, and a python `cpp_exception.stacktrace()`, give
   the same text as before.
4. `null_deleter::operator()`: `static` (C++23 static call operator).

Not done (D18): the `misc-const-correctness` sweep and adding that check to `.clang-tidy`.

Checks: full suite; `make lint`; `make format-check`.

## T9. Documents and final check

1. **Documents.** CLAUDE.md: the python minimum (T3), the samples script (T4), `SKELETON_CHECK_MATRIX_CLOSE` (T6.7). `cxx_python_rules/README.md`: streams flush explicitly (T1), write errors
   (T2), "Threads" (T7.3, T7.4). The review: add the status line "fixed, see the third fix plan". The deferred CI
   notes in the second fix plan: add the hardening flags (D17) as a later item, with what the review says about
   them (conda adds part already; measure release-mode `_GLIBCXX_ASSERTIONS` on geocal's hot loops first).
2. **Final check.** `script/check_all.sh` over `build`, `build-gcc14`, `build-noserial`, `build-asan`,
   `build-clang`, now including `script/samples.py --check`. Then `make installcheck` in
   `build`, and the CMake build with `ctest` in `build-cmake`. Each must pass with 0 warnings.

| Review item | Check |
|---|---|
| §3.1 | The T0 iostream tests pass without `xfail`; the raw-file test passes |
| §3.2 | The `/dev/full` tests pass in python and C++, each in seconds; golden identical |
| §3.3 | configure demands 3.13, and fails with a fake 3.12 `PYTHON` |
| §3.4 | `script/samples.py --check` passes and is in `check_all.sh` |
| §3.5 | `package_test.py` passes against the build tree and the installed package |
| §3.6 | `grep -n '(".gz")' python cxx_python_rules -r` finds nothing; the overflow tests pass under UBSan; `grep -rn BOOST_CHECK_MATRIX lib` finds nothing |
| T6.11 (not in the review) | A `build-gcc14` compile line has no `-DNDEBUG` and has `-DBZ_DEBUG`; suite passes in `build-gcc14` and `build-asan` |
| §4.1 to §4.4 | The compile-failure checks recorded; configure prints "Blitz thread safe: yes" |
| §4.5 | The four T8 items done; `misc-const-correctness` not added (D18) |
| everything | API snapshot vs `baseline/api_t0.txt`: only the docstring of `IostreamSwig.write` (if changed); `baseline/tidy_strict.sh`: no new findings outside the ones T8 removes; `make golden-check` identical; `ruff format --check` clean |

Last, a fresh read of the changed files, and the result in the progress log.

## Order and dependencies

T0 → T1 → T2 → T3 → T4 → T5 → T6 → T7 → T8 → T9. Notes:

- T1 and T2 come first: they are the bugs that lose data, and they are in files geocal copies verbatim. T2 matters to
  geocal now, since its SWIG-era `serialize_function.cc` has the same problem.
- The decisions are made (D14 to D18). T3 starts with the one condition left (geocal's and framework's python).
- T4 comes before T6 and T7, which change files that have samples (`swig_shelve.py`,
  `skeleton_serialize_support.h`, `blitz_array_serialize_support.h`), so those changes go through the script.
- T7.3 changes `_nanobind_support.py`, which pickles refer to by name. Function names stay the same, so old pickles
  still load (the golden and pickling tests check this).

## Not doing, with reasons

| Review item | Reason |
|---|---|
| Hardening flags (§6.3) | D17: later, with CI; conda adds part already, and release-mode library assertions need measuring on geocal first |
| `misc-const-correctness` (§4.5) | D18: taste; geocal shares `.clang-tidy` and would have to be cleaned for it |
| CI (§6.3) | Deferred by the second fix plan (D11), to be set up at JPL |
| include-what-you-use, `std::expected`, `std::print` (§4.6) | The review recommends against them |
| The design choices in §5.3 and the toolchain items in §5.5 | Decided earlier, or waiting for compilers |

## Notes for geocal (to copy into `implementation_plan.md` §0)

| Step | What geocal does |
|---|---|
| T1 | Comes with the copied `iostream.h`; generated bindings flush streams once `i_to_nanobind.py` has T1.4. |
| T2 | **Now, in `swig_rules/lib/serialize_function.cc`**: the same sink and `close()`; it has the same write-error and gzip-hang problems today. Later, the copied `serialize_function.cc` brings it. |
| T3 | Check geocal's own python minimum against 3.13 (T3 step 0 looks at its environments first). |
| T4 | Copy samples only from the generated ones; `serialize_support.h.sample` keeps the `boost_array.hpp` include and the `using` declarations geocal needs. |
| T6.6 | Optional: qualify `polymorphic_[io]archive` in its 5 files, then drop the `using` declarations. |
| T6.9 | The same `const&` for `this_obj()` in its `observer.h`, when it next updates its copy. |
| T6.11 | Copy `ac_enable_debug.m4` (geocal plan Phase 0 step 4a); geocal's env has conda's compiler activation, so its debug builds have the same problem today. |
| T7.1 | Run the `serialize_read`/`get<T>` grep before taking the constraint. |
| T7.3 | Comes with the copied `_nanobind_support.py` and `cxx_python_rules_nb.cc`. |
| T7.4 | The same configure check in geocal's m4; see what its Blitz says. |

## Progress log

| Phase / step | State | Notes |
|---|---|---|
| Plan written | done | 2026-11-07. The review was corrected at the same time (corrections 1 to 4 above), after checking the stream and gzip fixes in scratch prototypes. |
| Decisions D14 to D18 | done | 2026-11-07 (maintainer): the recommendations: D14 A (python 3.13, after T3's check of geocal and framework), D15 A (samples script), D16 A (add the C++ names to `__all__`), D17 A (no hardening option now), D18 B (targeted style changes only). |
| Set aside | — | 2026-11-07 (maintainer): plan ready, set aside for now. Start at T0. |
| T6.11 added | — | 2026-11-07: debug flags in activated envs (`-DNDEBUG` kept, no `-DBZ_DEBUG`), found by the review of geocal's plan and checked in this repository's build directories. Added after D14–D18; no decision needed (a bug fix). |
| Scheduled | — | 2026-11-07 (maintainer, from the geocal plan update): do at least T0–T2 before geocal's Phase 0. Checked for geocal on frenchpeas: `serialize_write` to `/dev/full` returns with no error for a small archive (plain and `.gz`), raises for a large plain one, and hangs for a large `.gz` one, as in review §3.2. T3 step 0: geocal's envs are on python 3.13.7 (linux and osx locks); framework is still `python<3.9`, which it has to leave anyway, so D14 (3.13) stands. |
| T0 baseline | done | 2026-11-07. `check_all.sh` over `build`, `build-gcc14`, `build-noserial`, `build-asan`, `build-clang`: all ok, 0 warnings, 229 passed and 1 xfailed before the new tests (golden, format, lint ok); `build-cmake` builds with 0 warnings, ctest 3/3. Baselines in the build area's `baseline/`: `api_t0.txt` (with `snapdiff_t0.sh`), `tidy_strict.sh` and `tidy_strict_t0.txt` (983 findings, 696 `misc-include-cleaner`, as in the review), `check_all_t0.txt`. New tests: three in `iostream_swig_test.py`, `serialize_write_error_test.py` (8 cases, 6 `xfail`; the two large plain ones pass), `package_test.py`. With them: 231 passed, 11 xfailed (`build`, `build-asan`, `build-clang`). |
| T1 stream adapters | done | 2026-11-07. `iostream.h` as planned: `exceptions(std::ios::badbit)` after `rdbuf`; `~py_ostream` catches, `nb::python_error` goes to `discard_as_unraisable`; `py_streambuf::flush` empties the buffer before writing (a failed write isn't retried by the destructor) and completes short writes (an `int` result smaller than the size; a result of 0 raises `RuntimeError` instead of looping). `iostream_swig_nb.cc` flushes explicitly; README and the usage comment show it. `i_to_nanobind.py`: `call_args` fills `post` with the flush; `method_def` runs it after the call (void: `call; flush; return;`, otherwise `decltype(auto) r = call; flush; return r;`, also in the director branch and with OUTPUT arguments). Checked on the old `.i` files from history plus a director class with four stream functions: only stream functions change, the rest of the output is byte for byte the same. Tests: the three T0 tests pass; new raw file (100 KB, `buffering=0`), short-write (1000 bytes per call) and zero-write tests. `build` and `build-asan`: 237 passed, 8 xfailed, 0 warnings; golden, format, lint ok. |
| T2 archive writes | done | 2026-11-07. `serialize_function.cc`: `WriteError` record (shared), `CheckedSink` (a Boost.Iostreams `sink_tag` device: `sputn` into the `ofstream`'s buffer, on a short write records `generic_category().message(errno)` or "write failed" and throws; after the first error every write throws at once), `OutputFile::close()` (flush, `sb.reset()`, `f.close()`, then `throw_if_error()` or a `f.fail()`/`os.bad()` check) and `throw_if_error()`; no exception mask on the stream (commented). `serialize_write` and `serialize_write_binary` go through a new `write_file<OArchive>`, which calls `close()` after `write<>()` returns and, if the archive throws, throws the recorded error instead. Messages: `Trouble writing file <name>: No space left on device`. Tests: the 8 python `/dev/full` cases pass (no `xfail`; they also check the message), the whole file in 1.4 s; new C++ test `serialize_function/write_full_disk` (same 8 cases, 0.4 s). A normal `.gz` passes `gzip -t` and decompresses to the same bytes as the plain write (XML and binary, a 20,000-object map); golden identical. `check_all.sh` over the five autotools builds: all ok, 243 passed, 2 xfailed (the old one and T5's), 0 warnings; `build-asan` included. |
| T3 python minimum | done | 2026-11-07. Step 0 was done earlier ("Scheduled" row: geocal 3.13.7). `SKELETON_PYTHON_MINIMUM` checks `(3, 13)`, its comment names both reasons; `./bootstrap` (the `configure` diff is only those lines). `CMakeLists.txt` `find_package(Python 3.13 …)`; `env/linux/pixi.toml` `python = ">=3.13"` (`pixi lock`: already up to date, the lock has 3.13.15); `cxx_python_rules/nanobind/README.md` and CLAUDE.md. Checked: `build` configure prints `… is at least python 3.13... yes`; a configure with a fake `PYTHON` that reports 3.12 stops with `python 3.13 or later is required, … is Python 3.12.0`; `build-cmake` finds 3.13.15 ("minimum required is 3.13") and builds. |
| T4 samples | done | 2026-11-07. `script/samples.py` (`--check` default, `--write`): the ten pairs, the four substitutions, sample-only blocks between `// sample only:`/`// end sample only` (`#` in python), kept on `--write` after the same line (the k-th occurrence of the line before the block; if that line is gone from the live file the script stops and asks for the block to be moved by hand). `serialize_support.h.sample`: its `boost_array.hpp` include put in a sample-only block by hand, under the live file's comment. `--write` changed exactly what the review lists: F5/F6 in `generic_object_map.h.sample`, `namespace boost::serialization` in `generic_object.h.sample` and `blitz_array_serialize_support.h.sample`. Checked: a line added to `lib/printable.h` makes `--check` fail; a changed sample makes `check_all.sh` report `(source) samples FAILED`. `check_all.sh` runs it once (summary row "(source) samples"); `samples.py` in `EXTRA_DIST`; CLAUDE.md (Formatting) and `cxx_python_rules/README.md` say the samples are generated. |
| T5 `__all__` | done | 2026-11-07. `python/skeleton/__init__.py`: `# </AUTOGEN_INIT>` after the generated block, then `__all__ +=` the public names of `skeleton_cxx.all` (70 names in all, no duplicates); the top comment says which part mkinit writes. `make -C python update-init` keeps the tail, but re-wrapped mkinit's block even before this change (`--black` needs black, which the env lacks), so the target now runs `mkinit` without `--black` and then `ruff format __init__.py`: the file comes out unchanged. `package_test.py` passes (no `xfail`; `# noqa: S102` on its `exec`) against the build tree and after `make install` (`make installcheck` in `build`: 244 passed, 1 xfailed). `python/README_developer.md` doesn't describe `update-init`; nothing to change there. mypy on `python/skeleton` clean. |
| T6 smaller fixes | done | 2026-11-07. 1: `ext != ".gz"` (sample through `samples.py`). 2: `read_with_pad` computes the end points *and* the offsets into the result in `std::ptrdiff_t` (`l0 - Lstart` overflows too for a very negative start); `SubRasterSample` and the legacy stand-ins check in `std::ptrdiff_t` (the stand-ins now also reject negative sizes). Tests: C++ `read_with_pad` at `INT_MAX - 5` and `INT_MIN + 5` in both directions, `SubRasterSample` at `INT_MAX - 5` both ways; python the same for `INT_MAX - 5`. 3, 4, 5 as planned (both `serialize_read_binary<T>` uses already use the result). 6: `using` declarations gone from `skeleton_serialize_support.h`, macro qualified; kept in a sample-only block in `serialize_support.h.sample`. 7: `SKELETON_CHECK_MATRIX_CLOSE[_TOL]` (11 uses). 8: `/opt/afids_support` dropped from the pkg-config path (it is still in the shared `config/m4` boost and library searches, not ours). 9: `this_obj()` returns `const&`. 10: `is_et_dyn` with `<elf.h>` (`ELFMAG`, `EI_DATA`, `ET_DYN`). 11: `ac_enable_debug.m4` conda branch: `CPPFLAGS=$DEBUG_CPPFLAGS` (or `-DNDEBUG` stripped) and `-DBZ_DEBUG`; `build-gcc14` compile lines have `-Og -DBZ_DEBUG` and no `-DNDEBUG`, `build`'s flags unchanged, and the suite passes with Blitz bounds checks on (nothing found). **Found:** in `build-asan`, UBSan's `vptr` check together with `-DBZ_DEBUG` made gcc 15 spend over 35 minutes on `blitz_array_serialize_support.cc` without finishing (measured: without `-DBZ_DEBUG` 34 s, without UBSan 19 s, with `-fno-sanitize=vptr` 43 s; turning off alignment, null, bounds/pointer-overflow or integer checks, or var-tracking, didn't help). So `skeleton_common.m4` drops `-DBZ_DEBUG` when the sanitizers include `undefined` (configure prints a notice): the `vptr` check covers the `GenericObject` casts, and the Blitz checks run in `build-gcc14`. `check_all.sh` over the five builds (`build-gcc14` and `build-asan` from clean): all ok, 245 passed, 1 xfailed, 0 warnings; golden, format, lint, samples ok. |
| T7 modernization | done | 2026-11-07. **T7.1:** geocal grep first: every type in its `serialize_read*<T>` calls derives from `GenericObject` (the value-like ones, `Time`, `TiePoint`, `Datum` and so on, through `Printable`), the template `Out` of `boost_pcall` is only used with `Rsm`, `ImageGroundConnection`, `SpiceKernelList`, and it has no C++ `get<T>` calls; so `requires std::derived_from<T, GenericObject>` on the four read templates and `GenericObjectMap::get<T>`. `to_blitz` takes a `mdspan_like` argument (concept in `blitz_mdspan.h`). `SKELETON_BLITZ_SERIALIZE_TYPES(X)` in `blitz_array_serialize_support.h` as `X(T, highest rank)` (2 or 5); the `.cc` instantiates from it (`_INSTANTIATE_2`/`_5` by token pasting), `serialize()` has a `static_assert` through `detail::blitz_serialize_max_rank<T>`, and the `all_types` test runs every listed type and rank, `std::string` now included (it wasn't). Compile failures checked by hand with `build`'s flags: `serialize_read<int>` and `GenericObjectMap::get<int>` ("constraints not satisfied"), `to_blitz(42)` (same), a `blitz::Array<std::complex<double>, 1>` serialized (the `static_assert` message). **T7.2:** null check and `runtime_error` in `to_blitz`; the comment in `blitz_mdspan.cc`. **T7.3:** `_nanobind_support.py` imports `_bindings` in the three functions. The plan assumed the downstream module calls `init_module`; it doesn't (it doesn't compile `cxx_python_rules_nb.cc`), so a new inline `init_statics()` (`support()` and `numpy()`) is called by `init_module` and by `downstream_nb.cc` after importing `_bindings`. **Bug found** (there before this change, hidden because the downstream module never used them): with gcc the guard variables of `support()::m` and `numpy()::m` are `STB_GNU_UNIQUE` symbols, one for all modules, while the pointers are per module (the type `nb::module_` is hidden). The main module sets the shared guard, and the downstream module's pointers stay null: UBSan reported "reference binding to null pointer" at import, and without sanitizers a list passed to a downstream function taking a Blitz array segfaulted (new test `test_downstream_module_statics`, `array_sum` in `downstream_nb.cc`; checked that it crashed before the fix). Fix: constant-initialized pointers set on first use, so there is no guard (the GIL is held, and `init_statics` sets them at import). The type registry's pointer and guard are both unique symbols, so it was consistent (comment added). clang has no unique guards. Checked: `import skeleton_cxx`, `import skeleton_cxx._bindings` first, the downstream module first, pickling. README "Threads". **T7.4:** `SKELETON_BLITZ_THREADSAFE` in `skeleton_common.m4` (compile test for `BZ_THREADSAFE` with `BLITZ_CFLAGS`, warning if missing), called after `AC_BLITZ` in `configure.ac`; `build` prints `checking whether Blitz is thread safe... yes` and `Blitz thread safe: yes` in the summary; README "Threads". Samples regenerated. `check_all.sh` over the five builds: all ok, 246 passed, 1 xfailed, 0 warnings; golden, format, lint, samples ok. |
| T8 style | done | 2026-11-07. 1: `stacktrace()` uses `std::views::enumerate` and `std::format_to(std::back_inserter(res), …)` in both branches. 2: `const int status` from a lambda in `BufferRasterSample::read_into` and `write_from` (`nl`, `ns` const too). 3: `symbolize()` split into `locate()` (addresses → fallback text and the frames per module) and `addr2line()` (one run per module, fills in the names); a `ModuleFrames` alias for the per-module list. Checked: two stack traces from python (`caught_exception` and a raised `cpp_exception`, 39 lines, run under `setarch -R` so the addresses repeat) are byte for byte the same before and after; the C++ stack trace test passes. 4: `static void operator()` in `null_deleter`. Not done (D18): `misc-const-correctness`. `check_all.sh` over the five builds: all ok, 246 passed, 1 xfailed, 0 warnings; golden, format, lint, samples ok. |
| T9 final check | | |
