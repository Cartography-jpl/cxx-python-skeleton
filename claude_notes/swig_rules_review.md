# Review: swig_rules SWIG-wrapping approach and its testing

Scope: `swig_rules/` (the shared subtree of `.i` rules and support `.h`/`.cc` files) and how the skeleton
(`lib/`, `python/lib/`) exercises and tests it. Grounded in the actual source, not just `DirectorNotes.md`.

## Overview

The approach wraps C++ for Python using SWIG, with all generated wrappers for a project compiled into a
single combined shared library (`_swig_wrap.la`, see `swig_rules/swig_rules.am`). The interesting design
problem this solves is C++/Python object lifetime for "directors" — Python subclasses of C++ virtual
classes. The chosen model (`swig_rules/lib/DirectorNotes.md`) is: Python always owns the object; when a
director is passed into C++, SWIG hands out a second `shared_ptr` that uses a custom deleter
(`PythonRefPtrCleanup`, `swig_rules/lib/python_ref_ptr_cleanup.h`) which decrements the Python refcount
instead of freeing the object. This is implemented as a hand-patched fork of SWIG's own
`boost_shared_ptr.i`, shipped as `swig_rules/include/my_shared_ptr.i` (544 lines), plus supporting pieces for
type-preserving downcast (`shared_ptr_type_mapper.i`, `swig_type_mapper_base.h/.cc`, `swig_type_mapper.h`,
`swig_to_python.h`), exception propagation across the Python/C++ boundary (`swig_exception.i`,
`python_exception.h`), and boost::serialization/pickle support for director objects
(`director_serialization.i`, `swig_pickle.i`, `serialize_function.cc`, `weak_ptr_serialize_support.cc`).

## What's good

- **The director lifetime scheme has real, non-trivial regression tests, not just documentation.**
  `python/lib/director_example_test.py` exercises: basic director override (`test_director_example`),
  reference counting through `std::vector<shared_ptr<T>>` and `std::vector<std::vector<shared_ptr<T>>>` of
  mixed C++/Python-derived objects (asserting the exact count of Python `__del__` calls via
  `DirectorPythonExample.deleted_list`), pickling and `shelve` round-tripping of director objects
  (`test_director_pickle_shelve`, `test_director_lifetime_shelve`), and exception propagation out of a
  director override (`test_exception_propagate`). These aren't smoke tests — they assert exact counts of
  destructor calls and exact exception message text, which is the right level of rigor for lifetime code.
- **The trickiest corner case (weak_ptr to a director) was deliberately captured as a failing test before it
  was fixed.** Commit history shows the progression cleanly: `c4dfa95` added a segfaulting test to pin down
  a real bug found in ReFRACtor, `ebde4c1`/`8c2c27b` documented the remaining weak_ptr issue with a
  commented-out assertion, and `8c8b959` introduced the `SHARED_PTR_NO_OWN` typemap and turned the
  commented-out assertions into real ones (`c.expired()`, `c.use_count() == 0` in
  `python/lib/director_example_test.py`). That's a good test-driven-fix pattern for genuinely hard
  concurrency-adjacent lifetime bugs.
- **Plain (non-director) weak_ptr serialization is tested and passing.** `python/lib/generic_object_map_test.py::test_generic_object_map_weak_ptr` and the C++-side `lib/generic_object_map_test.cc`
  (`serialization1_weakptr`, `serialization2_weak`) cover the "weak_ptr should not force serialization,
  should resolve to `None`/be dropped when not otherwise referenced, should be shared correctly when the
  target is serialized" cases for ordinary `GenericObject`s.
- **The upstream-diff strategy for `my_shared_ptr.i` is explicit and documented**, not just tribal knowledge:
  `swig_rules/README.md` ("Developer notes") tells the next maintainer to diff `my_shared_ptr.i` against
  SWIG's `boost_shared_ptr.i` when upgrading SWIG, and states which SWIG version it's verified against
  (4.2.1). The file itself also carries this note in its header comment.
- **Clean separation of "reusable rules" from "consumer conventions."** `swig_rules/` has no dependency on
  skeleton-specific code — `GenericObject`, `Printable`, etc. are conventions documented via `*.sample` files
  (`swig_rules/lib/*.sample`) rather than hard requirements, and `SWIG_MAPPER_NAMESPACE` is a compile-time
  `#define` rather than a hardcoded namespace, so the same rules subtree is reused unmodified across
  different consuming projects (this skeleton, geocal, etc.).
- **Exception propagation is handled centrally and correctly for the common cases.** `swig_exception.i`'s
  `%exception` block distinguishes `Swig::DirectorException` (already-reported Python error) from
  `PythonException` (needs `restore_python_exception()`) from generic `std::exception`, and
  `python_exception.h`'s `PythonException` constructor captures the *original* type/value/traceback via
  `PyErr_Fetch`, not just a stringified message — this is what lets `test_exception_propagate` assert on the
  original `RuntimeError` message rather than a generic wrapped error.
- **The doxygen → SWIG docstring pipeline keeps generated Python docstrings in sync with C++ doc comments**
  without hand-duplicated docs (`doxy2swig.py`, `swig_rules/swig_rules.am`'s per-module doxygen rule).

## What's concerning / bad

- **The weak_ptr-to-director test is currently skipped, and has been since the SWIG 4.2.1 upgrade, with no
  tracking issue or explanatory comment.** `python/lib/director_example_test.py:291` has
  `@skip` directly above `test_director_example_weak_ptr` — the exact test that commit `8c8b959` fixed and
  turned from placeholder into real assertions. It was disabled in `6c7105d` ("Add fix for newer versions of
  SWIG my_shared_ptr.i, and test with swig 4.2.1"), a commit that otherwise made substantial changes to
  `my_shared_ptr.i` and `shared_ptr_type_mapper.i` for SWIG 4.2.1 compatibility. The commit message doesn't
  explain *why* the test needed to be skipped, and there's no comment in the test file either — anyone
  reading `DirectorNotes.md` (which explicitly points at `DirectorExampleWeakPtr` as "an example illustrating
  the issue") would reasonably believe this path is verified, but it currently is not exercised at all. This
  is exactly the kind of code (weak_ptr + director + custom deleter) where a silent regression is most
  dangerous and least likely to be caught any other way.
- **`SHARED_PTR_NO_OWN` is an intentional footgun enforced by nothing but a comment.** Both
  `my_shared_ptr.i` (in the `%typemap(in) ... & SHARED_PTR_NO_OWN` block) and `DirectorNotes.md` state "it is
  important that SHARED_PTR_NO_OWN not actually be copied to a persistent shared_ptr... you are likely to get
  a seg fault." Nothing in the type system, a static check, or a debug-build assertion stops a future
  developer from doing exactly that — the only usage sites in the whole repo are the two parameters in
  `lib/director_example.i:44,47` (`DirectorExampleWeakPtr` constructor and setter), so there's exactly one
  worked example and zero tests that would catch someone misusing the pattern elsewhere.
- **Heavy reliance on `#error` stub typemaps for "shouldn't happen" cases, none of which are exercised.**
  `my_shared_ptr.i` has `#error` bodies for `directorout` of plain pointer, plain reference, and pointer-by-reference; and for `varin`/`varout` of `shared_ptr &`, `shared_ptr *`, and `shared_ptr *&`. These are
  compile-time errors (good — fail loud rather than corrupt memory), but since no `.i` file in this repo
  triggers them, there is no CI signal if a future refactor accidentally starts requiring one of these paths;
  the first person to hit it will get a cryptic SWIG-generated `#error` deep in generated code, not a clear
  message pointing at "unimplemented by design."
- **`serialize_function.cc` (413 lines: file I/O, gzip compression via `boost::iostreams`, `DirChange`
  directory-restore-on-exception logic, XML vs. binary archive variants) has no direct C++ unit test at all.**
  The only C++ tests under `lib/*_test.cc` (`generic_object_map_test.cc`, `printable_test.cc`,
  `blitz_array_serialize_support_test.cc`) exercise serialization indirectly through
  `serialize_write_string`/`serialize_read_string`, which cover the boost::serialization/archive plumbing but
  never touch the file-based entry points (`serialize_write`, `serialize_read_generic`,
  `serialize_read_binary_generic`) — so `DirChange`'s chdir-and-restore-on-exception behavior, the `.gz`
  filename-sniffing gzip logic, and the binary-archive file variants are untested by anything in this repo.
  (The Python `write_shelve`/`read_shelve` tests likely exercise the file path only.)
- **Recent history (last 3 commits, `6f81847`→`2afe0b6`→`3e7fce6`) shows the director-exception-propagation
  code churning three times in about a year**, each time touching `swig_rules/lib/python_lib_init.h` and/or
  `swig_rules/lib/python_exception.h`/`swig_exception.i` — "add better parsing," then "improve handling,"
  then "use the original exception rather than swallowing it into a new one." This is evidence the
  Python-exception-through-director abstraction is still not fully settled even after several iterations;
  each fix so far has come with a new/extended test in `director_example_test.py`, which is good, but the
  churn itself suggests this boundary is genuinely tricky and likely still has undiscovered edge cases (e.g.
  exceptions raised during `__del__`, exceptions during pickling as partially covered by
  `test_director_pickle_error`, but not e.g. exceptions raised from a director method called during
  interpreter shutdown).
- **The `my_shared_ptr.i` diff-against-upstream strategy is entirely manual and undocumented in tooling.**
  There is no script anywhere in `swig_rules/` or `config/` that actually performs the recommended diff
  against the installed SWIG's `boost_shared_ptr.i` — the advice in `swig_rules/README.md` is "you can
  directly do a diff," with no automation, so drift is only caught if someone remembers to do this by hand
  during a SWIG upgrade. Given `my_shared_ptr.i` already carries version-conditional branches
  (`#if SWIG_VERSION < 0x040000`, `< 0x040100`), the file has already needed several rounds of hand-patching,
  each a manual, error-prone process with no test forcing the comparison.
- **The single combined `_swig_wrap.la` module (`swig_rules/swig_rules.am`) means any change to any one `.i`
  file forces relinking/reloading of the entire Python extension**, and — more importantly for testing —
  means a SWIG-generation or compile failure in one module (e.g. a rarely-touched one like
  `swig_rational.i`) blocks `make check` for the whole build, including the C++ tests that have nothing to do
  with SWIG at all. This is a deliberate tradeoff documented in `skeleton_common.i`'s comment about SWIG 4's
  `moduleimport`, but it does concentrate build risk.
- **The doxygen dependency is coupled into SWIG wrapper generation** (`swig_rules/swig_rules.am`'s
  `$(srcdir)/$(swigsrc)/%_wrap.cc` rules require `$(DOXYGEN_TARGET)`), so a doxygen version bump or config
  incompatibility can break wrapper generation even though doxygen's only role is enriching docstrings —
  `configure.ac` does allow bypassing with `--without-documentation`, but that's an opt-out a developer has
  to know to reach for, not a decoupling of the two concerns.

## Testing assessment

**Has direct test coverage:**
- Director basic override + `func` dispatch — `director_example_test.py::test_director_example`
- Director lifetime via plain object, `vector<shared_ptr<T>>`, and `vector<vector<shared_ptr<T>>>` — same file, same test
- Director pickling / `shelve` round-trip, including a deliberately-bad-pickle case — `test_director_pickle_shelve`, `test_director_pickle_error`
- Python exception → C++ → back-to-Python propagation through a director — `test_exception_propagate`
- Non-director weak_ptr serialization (shared vs. dropped vs. `None`) — `generic_object_map_test.py`, `lib/generic_object_map_test.cc`
- `Printable`/`operator<<`-to-`__str__` mapping — `lib/printable_test.cc`
- blitz⇄numpy array lifetime (return, pass-in, round-trip; comments direct the reader to run under valgrind) — `python/lib/numpy_sample_test.py`
- Python-file-handle-as-`iostream` mapping — `python/lib/iostream_swig_test.py`

**Does not have direct test coverage:**
- **Director + weak_ptr combined (`SHARED_PTR_NO_OWN`)** — the one test for this (`test_director_example_weak_ptr`) is `@skip`ped since the SWIG 4.2.1 upgrade (`6c7105d`). This is the single highest-risk gap: it's the intersection of the two hardest things in this codebase (director ownership + weak_ptr), and it's currently unverified.
- `serialize_function.cc`'s file-based entry points (`serialize_write`/`serialize_read_generic`/`serialize_read_binary_generic`), the `.gz` auto-detection, and `DirChange`'s restore-on-exception path — no C++ test calls these directly.
- Any of the `#error` "unimplemented" typemap paths in `my_shared_ptr.i` (directorout for plain pointer/reference/pointer-ref; varin/varout for shared_ptr ref/pointer/pointer-ref) — untested by construction, since nothing in this repo's `.i` files needs them, but also undocumented *why* they're safe to leave unimplemented versus a ticking time bomb for a future consumer project.
- `swig_boost_optional.i`, `swig_rational.i`, `swig_python_attribute.i`'s various macro variants (`%python_attribute2*`, `_nonconst`, `_derived`) — none of these appear to be exercised by any `.i` file or test in this skeleton; they're presumably tested by consumer projects (e.g. geocal) but not here.

**Verdict:** for the parts of the design that are genuinely hard — director lifetime under refcounting,
exception propagation across the language boundary — the test suite is unusually rigorous (exact refcount
assertions, deliberately-introduced-then-fixed regression tests, real pickling round trips). That rigor makes
the currently-skipped weak_ptr/director test stand out as a real gap rather than an oversight equivalent to
the untested corners of `serialize_function.cc`: it's a *known* hard case that *was* covered and is *now*
silently uncovered. Test coverage is not proportional to risk here specifically because the highest-risk
combination (director + weak_ptr) is the one currently disabled.

## Suggestions

- Re-enable or replace `test_director_example_weak_ptr` (`python/lib/director_example_test.py:291`) —
  at minimum, change `@skip` to `@pytest.mark.xfail(reason=...)` with a comment explaining what broke under
  SWIG 4.2.1, so it's visible as a known gap rather than silently absent; ideally, root-cause and fix it.
- Add a comment or `NEWS`/README entry cross-referencing why the test was disabled in `6c7105d`, since
  `DirectorNotes.md` still describes this exact test as the reference example — the docs and the test suite
  currently disagree about whether this path is verified.
- Consider a small script (even a `make` target) that fetches/locates the installed SWIG's
  `boost_shared_ptr.i` and diffs it against `swig_rules/include/my_shared_ptr.i`, to at least flag (not
  necessarily block) drift on SWIG upgrades instead of relying on a README instruction.
- Add C++ unit tests for `serialize_function.cc`'s file-based entry points (`serialize_write`,
  `serialize_read_generic`/`_binary_generic`), including the `.gz` auto-detect and a test that `DirChange`
  restores the working directory even when the archive read throws.
- For the `#error` stub typemaps in `my_shared_ptr.i`, add a short comment at each site noting *why* that
  shape is unsupported (e.g. "no consumer needs a director out of a raw pointer-by-reference") so a future
  maintainer hitting the error knows whether to implement it or restructure their `.i` file instead.
- Given three consecutive commits touching director-exception handling, consider adding a stress/fuzz-style
  test that raises exceptions from more unusual points (a director destructor, a director method called
  during GC, a nested director-calls-director scenario) to get ahead of the next iteration of this fix cycle.
