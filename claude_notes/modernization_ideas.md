# Modernization ideas for the skeleton C++ library

Status: **ideas only, no code changes.** Section 10 adds what the two real consumers (geocal and framework) change about
the conclusions below; where it contradicts an earlier section, section 10 wins. **Section 11 (2026-10-23) records what
the geocal planning and the nanobind probes changed, and `implementation_plan.md` is now the plan of record; where they
disagree with anything here, they win.**
 Based on a read of `lib/`, `python/`, `swig_rules/`, the `.am`/`configure.ac`
build, and `env/linux/pixi.toml`. Statements about third-party tools (nanobind, litgen, xtensor, ...) come from my
knowledge of those projects, not from trying them here. Verify version-specific claims before committing to a plan.

## 1. Where the code is today

| Area | Current state | Size / notes |
|---|---|---|
| Language level | Already `-std=gnu++17` (`AX_CXX_COMPILE_STDCXX([17],[ext],[mandatory])`), but the code style is C++03 | The standard is ahead of the code. |
| Smart pointers | `boost::shared_ptr` / `weak_ptr` / `make_shared` everywhere (about 60 uses in `lib/`) | `swig_rules/` has a 547-line forked `my_shared_ptr.i` and custom deleters. |
| Arrays | `blitz::Array<T,N>`, N = 1..5, in `numpy_sample.h` and `blitz_array_serialize_support.*` (391 lines of `.cc`) | The SWIG side is ~900 lines (`swig_array.i`, `swig_array_inc.i`). `swig_array_inc.i` subclasses `blitz::MemoryBlock` and `#define`s `blockLength()` to inject a `friend`. |
| Bindings | SWIG 4.x, 9 `.i` files in `lib/` plus 17 shared in `swig_rules/include/` | Doxygen to `doxy2swig.py` to docstrings. Everything is linked into one `_swig_wrap.so`. |
| Serialization/pickle | `boost::serialization` with polymorphic archives, bridged to python `pickle` | The most intricate part: `serialize_function.cc` (456 lines), `serialization_context.*`, `weak_ptr_serialize_support.h`, `serialize_shared_ptr.h`. |
| Tests | Boost.Test (C++), pytest (python) | Fine, but Boost.Test is heavy. |
| Build | autotools plus `m4` macros from a subtree, `make check` | A pip `setup.py` and `pyproject.toml` already exist for the pure-python part. |
| Typing | Python package has type hints in places (per recent commit), but SWIG output has no `.pyi` | |

The code is small, so migration is cheap here. The real cost is in the generic machinery in `swig_rules/`, which
downstream projects such as geocal depend on. That machinery is also where most of the benefit is.

## 2. Plain C++ modernization (low risk, independent of any binding choice)

These can be done first and make later steps easier. None of them need new dependencies.

1. **`boost::shared_ptr` to `std::shared_ptr`** (also `weak_ptr`, `make_shared`, `dynamic_pointer_cast`).
   - Mechanical in `lib/`, but it touches `serialize_shared_ptr.h`, `weak_ptr_serialize_support.h`, `my_shared_ptr.i`
     and `shared_ptr_type_mapper.i`.
   - `boost::serialization` supports `std::shared_ptr` (`<boost/serialization/shared_ptr.hpp>`). The repo's own
     `serialize_shared_ptr.h` / `weak_ptr_serialize_support.h` exist because of tracking and weak_ptr gaps, so check
     whether they still do what is needed.
   - Do this *before* the binding migration: nanobind has first-class `std::shared_ptr` support and no `boost::shared_ptr`
     support (see 4).
2. **`boost::optional` to `std::optional`**. Used in `numpy_sample.h` and the `swig_boost_optional.i` typemaps.
3. **`boost::array` to `std::array`**, `boost::assign` removed (`generic_object_map.h` includes it, apparently unused),
   `boost::range` adaptors to a plain loop or C++20 `std::views::keys`.
4. **Rule of zero and `override`/`= default`**:
   - Empty `virtual ~X() {}` can become `= default`.
   - Add `override` on `print`, `func` and `desc`.
   - `throw()` on `Exception::~Exception` is a deprecated exception specification. Use `noexcept`.
5. **`Printable<T>` CRTP**:
   - It uses C-style `((T*) this)` casts, virtual inheritance from `GenericObject`, and a 20-year-old "Mac 10.4.11 double
     free" workaround (`std::string buf("blah"); buf.reserve(1000)`), which is also copy-pasted into
     `skeleton_exception.h`. The workaround can almost certainly be deleted.
   - Use `static_cast<const T*>(this)`.
   - Consider a C++20 `concept` for "has `print(std::ostream&)`".
   - Longer term, `std::formatter` specializations (C++20/23 `std::format`) could replace ostream printing. Python `__str__`
     only needs a string, so this is optional.
6. **`Skeleton::Exception`**: it stores an `ostringstream` and hand-writes a copy constructor with `catch(...)`.
   Storing a `std::string` plus a streaming helper would be simpler, copyable, and safer for exception objects. Also check
   `what()` lifetime.
7. **`GenericObjectMap : public std::map<...>`**: publicly inheriting from a standard container is a known smell. Use
   composition and expose `keys()`, `get`, `set` explicitly. The python `__getitem__` / `__contains__` / `__getattr__`
   shims in the `.i` file would then be written against a clean interface.
8. **Pass-by-value-and-move** for the shared_ptr and `std::string` constructor arguments. Replace `const shared_ptr<T>&` where
   ownership is taken.
9. **`std::vector<std::vector<shared_ptr<...>>>` and `GenericObjectB(const shared_ptr&) : a_(V) {}` in `.i` files**:
   the `.i` file contains inline C++ initializers that are copied header code. This goes away under nanobind because there
   is no second hand-maintained declaration.
10. **Hygiene**: `.h~`, `.cc~`, `.i~` backup files in `lib/` and `python/`, `configure~`, `README.git~`, untracked
    `python/test*.xml`. Add them to `.gitignore`.
11. **Later, C++20**: `std::span` for array arguments, `<concepts>`, `std::format`. Requires bumping `AX_CXX_COMPILE_STDCXX`
    from 17 to 20 (the pixi compilers are surely new enough).

## 3. Array library: replacing blitz

### What blitz is actually providing here
- N-d array (1 to 5 dims), sharing storage, with **arbitrary base index, strides and storage order** and slicing that
  creates views (`blitz::Range`).
- Expression templates (`data_ = 1, 2, 3, 4;`, array math).
- A mature python bridge **written by this project** (`PythonMemoryBlock`, a `blitz::MemoryBlock` subclass that holds a
  reference to a numpy object). It depends on a `#define` hack to reach a private member.
- Boost serialization support (`blitz_array_serialize_support.cc`), instantiated per type and rank.

In the skeleton only `NumpySample` uses arrays, so replacement is easy here. In a real downstream project, array use is
probably pervasive, and this is the hard part of the modernization. Note that blitz is largely unmaintained, and the
conda-forge `libblitz` package is the only reason it is easy to install here.

### Candidates

| Option | Pros | Cons / questions |
|---|---|---|
| **xtensor** (+ `xtensor-python`) | Closest to blitz and numpy in spirit: N-d, expression templates, views (`xt::view`, `xt::range`), row-major and column-major, broadcasting, active development. | `xtensor-python` is **pybind11-only** (as far as I know). There is no official nanobind bridge, so you would write a type caster (not hard, see below). It also has heavy compile times and a custom 3-library dependency stack (`xtl`, `xtensor`, `xtensor-python`). |
| **Eigen** (+ nanobind's Eigen support) | Ubiquitous, header-only, nanobind has built-in `Eigen::Matrix`/`Map` casters. Best-in-class linear algebra. | Only 1-D and 2-D in the core library. Higher ranks need `Eigen::Tensor` (unsupported module). No arbitrary base index. |
| **`nb::ndarray<T, nb::ndim<N>>` (nanobind) as the interface type, with plain storage internally** | The binding layer *is* the numpy bridge. It is zero-copy, handles lifetime (see below), and supports numpy, PyTorch, JAX and CuPy through DLPack/buffer protocol. | It is only a non-owning view. C++ code that wants arithmetic or slicing needs a library underneath. |
| **C++23 `std::mdspan`** (or the Kokkos reference implementation `mdspan` for C++17/20) | Standard. Zero dependencies. Non-owning N-d view with layouts (left, right, stride), which is the natural mate for `nb::ndarray`. Very cheap to adopt. | No arithmetic or expression templates. Owning storage is a `std::vector` or `std::unique_ptr<T[]>` plus an mdspan. |
| **Keep blitz** and only change the python bridge | No C++ churn in downstream code. | Keeps the unmaintained dependency. With nanobind there is no SWIG typemap so the `PythonMemoryBlock` bridge has to be rewritten as a nanobind caster anyway. |
| **Armadillo / Boost.MultiArray / Boost.uBLAS** | | Not recommended: weaker rank support, weaker python story, or stagnant. |

### Recommendation (for discussion)
- For the **skeleton**, a two-step demonstration is enough: (a) `NumpySample` stores a small owning container
  (xtensor `xt::xtensor<double,2>` or a `std::vector` + `mdspan`), and (b) the python boundary uses `nb::ndarray`.
- For **real projects** (geocal-like) that lean on blitz expression syntax, **xtensor is the closest drop-in**.
  Budget for the compile-time cost, and for writing the nanobind caster (xtensor to `nb::ndarray` and back, mirroring
  what `xtensor-python` does for pybind11).
- If most array usage turns out to be just "pass an N-d block of doubles around", **`mdspan` + `nb::ndarray`** has the least
  dependency risk and is where the standard is heading.
- **Behavioral differences to test explicitly** (the existing `numpy_sample_test.py` lifetime tests are good seeds):
  - Blitz arrays can be non-zero-based and the test suite exercises `blitz::Range`. xtensor and Eigen do not support
    arbitrary bases.
  - Storage order: blitz defaults to row-major, but supports Fortran order and mixed order.
  - Lifetime: the three existing lifetime tests (return, pass, round-trip) map directly onto nanobind's `ndarray`
    ownership rules (`nb::rv_policy`, capsule-owner). They stay valuable under any option.
  - Const correctness: returning `const blitz::Array&` of internal storage and having python mutate it. nanobind's
    `nb::ndarray<const double>` makes the read-only flag explicit.
- **`blitz::Range` to python** (via `boost::optional<blitz::Range>`): there is no equivalent in the numpy world. Replace
  with `std::optional<std::pair<...>>`, a plain `slice`, or drop it.

### Serialization of arrays
`boost::serialization` of a flat buffer plus shape works for any of the above. The 391-line blitz serializer, instantiated
for 5 ranks times 5 types, would be replaced by one template over rank or `std::span` of the data plus shape. With xtensor,
`xtensor` has no built-in boost::serialization support, so write one function. The `boost::serialization::make_array`
approach in the current code carries over.

## 4. Binding layer: SWIG vs nanobind (and litgen, pybind11, others)

### Options

| Option | What it is | Fit to this repo |
|---|---|---|
| **SWIG (status quo)** | Interface files, typemaps, doxygen-fed docstrings. | Works, supports many languages (not needed: Ruby support was dropped, only python is used). The maintenance burden is the 5,900 lines in `swig_rules/` (typemaps, forked `boost_shared_ptr.i`, patched `moduleimport`, `PYTHON_SILENT_MEMLEAK` workaround, etc.). SWIG<4.4 is pinned in the pixi file. Generates no type stubs. Slow compile of `*_wrap.cc`. |
| **pybind11** | Header-only, macros-free C++ API, widest ecosystem (xtensor-python, Eigen, many examples). | Fine, but nanobind is the same author's successor and is smaller/faster. Only choose pybind11 if you need `xtensor-python` or a library with only pybind11 support. |
| **nanobind** | Same API as pybind11 (mostly), compiles faster, smaller binaries, lower runtime overhead, requires C++17 and CPython >= 3.9 (check current floor). Supports stable ABI (`abi3`) builds on newer Pythons, free-threading, `ndarray` for numpy/PyTorch/JAX/CuPy, `std::shared_ptr` / `unique_ptr`, `std::optional`, `std::variant`, STL containers via `nanobind/stl/*.h`, trampolines for virtual overrides (`NB_TRAMPOLINE` / `NB_OVERRIDE`), and **generates `.pyi` stubs** (`nanobind.stubgen`). | Good fit overall. Details below. |
| **litgen** | *Not a binding library*. It is a **generator**: parses C++ headers (using srcML) and emits pybind11 **or nanobind** binding code plus `.pyi` stubs, **carrying C++ doc comments into python docstrings**. | Replaces the `.i` files and the entire doxygen -> `doxy2swig.py` pipeline. It targets exactly this repo's pain point (hand-maintained `.i` mirrors of headers). See below for caveats. |
| **cppyy / cffi / Cython / Boost.Python** | Different philosophy (JIT reflection / C ABI / C-like DSL / legacy Boost). | Not good fits for a C++ class library with inheritance and directors. Boost.Python is effectively the legacy predecessor of pybind11. |
| **Python's `nanobind` + manual bindings** | Hand-write about 10 lines per class. | For a library of this size (about 15 classes), hand-writing bindings is very likely less code than the `.i` files and typemaps. |

### How nanobind maps onto what the repo does today

| Current SWIG feature | nanobind equivalent / note |
|---|---|
| `%skeleton_shared_ptr(T)` plus auto-downcast via `GenericObject` RTTI (`shared_ptr_type_mapper.i`) | nanobind does polymorphic downcasting automatically for types with a virtual method (`nb::class_<Derived, Base>`), so the RTTI trick shouldn't be needed. |
| Forked `my_shared_ptr.i` with `PythonRefPtrCleanup` deleter, `SHARED_PTR_NO_OWN` | nanobind's `std::shared_ptr` caster ties the C++ `shared_ptr` to the python object's lifetime (the shared_ptr from python holds a reference to the python object). This is conceptually the same design as `DirectorNotes.md` describes, but supported by the library. `weak_ptr` is **the main unknown** and should be prototyped first (does it behave when a distinct shared_ptr is handed out? is there a `nanobind/stl/weak_ptr.h` in the version in use?). Consider `nb::intrusive_ptr` if objects exist mostly to be shared between python and C++. |
| `%feature("director")` | Trampoline classes (`struct PyDirectorExample : DirectorExample { NB_TRAMPOLINE(DirectorExample, 2); ... }`). One small class per director base. `DirectorExampleUser` and `director_example_test.py` are a ready-made regression test for this. |
| Exception propagation from python overrides (recent commits, `python_exception.h`) | nanobind propagates python exceptions through the trampoline as `nb::python_error` and re-raises the original. Needs testing but likely simpler than the SWIG-director version. |
| `%python_attribute`, `%python_attribute_with_set` | `.def_prop_ro(...)` / `.def_prop_rw(...)`. |
| `Printable::print_to_string` as `__str__` | `.def("__str__", &T::print_to_string)` or a generic helper template used with every `Printable`. |
| `%template(Vector_...)` and `std_vector.i` | `#include <nanobind/stl/vector.h>` (copying) or `nb::bind_vector` for reference semantics. |
| One shared library `_swig_wrap.so` | One `NB_MODULE`, with sub-registration functions per header group. Simpler than the SWIG 4 `moduleimport` override. |
| `%pickle_serialization`, `serialize_function.i` | See section 5. |
| `iostream_swig` (`swig_iostream.i`) | Python file-like object <-> `std::ostream` needs a custom `streambuf`. nanobind has no ready-made equivalent. This is a small hand-written piece. |
| `boost::rational` | Map to python `fractions.Fraction` with a small custom caster, or drop it. |
| Generated docs | `nb::class_` docstrings written by hand (or litgen). Sphinx autodoc works with the generated stubs. |

### Where litgen would fit
- Use **litgen + nanobind** to generate the bindings from the C++ headers. Comments become docstrings, and `.pyi` is produced.
- Caveats to evaluate in a spike:
  - litgen's generated code expects reasonably standard C++. Heavy CRTP (`Printable<T>`), `boost::shared_ptr`, and
    virtual inheritance from `GenericObject` may need preprocessing options or manual overrides. Doing the plain C++
    cleanup in section 2 first makes this much easier.
  - It needs `srcML` as a build-time tool (conda-forge has it), but only for *generating* the bindings, so generated files
    can be committed, or regenerated in a dev-only step.
  - For directors and custom shared_ptr/serialization pieces you still hand-write nanobind code alongside the generated
    code. litgen is designed for that mix. Verify how it handles virtual classes that need trampolines.
  - Compare its output with hand-written nanobind for *this* small a library; the benefit grows with the number of
    classes, so for the skeleton it is mostly a demonstration.

### Migration cost/risk
- The `.i` files are small (about 450 lines in `lib/`). The 5,900 lines under `swig_rules/` are the "real" framework and
  many of them have no counterpart because nanobind has them built in. The result would likely be **a few hundred lines
  of C++ plus an automake stanza** instead of a templated typemap framework. That is the strongest argument for the move.
- **Coexistence is awkward**: SWIG and nanobind each keep their own type registry, so an object can't cross between SWIG
  modules and nanobind modules. A hybrid migration is only practical at the level of whole libraries (whole python
  modules), not per-class. For a large downstream project (geocal) that implies either a big-bang port or a
  split by library boundary.
- The skeleton is the right place to prototype: it is small, has real tests for the hard cases (directors, weak_ptr,
  array lifetime, pickle with shared state), and can keep the SWIG version on `master` while a `nanobind` branch is
  developed for comparison.

## 5. Serialization and pickle

This is the piece with no drop-in replacement, and it is where most of the design work is.

- Today: `boost::serialization` (XML/binary polymorphic archives) to support C++ persistence and python `pickle`, with
  `SerializationContext` managing the case where C++ and python objects reference each other, so object identity is
  preserved (the `handle_duplicate_object` branch and `GenericObjectMap` are about this).
- Options:
  1. **Keep boost::serialization** and expose it through nanobind's pickle protocol (`__getstate__`/`__setstate__`, which
     nanobind supports by letting `__setstate__` construct in place). Smallest change, preserves file formats. The
     `SerializationContext` logic is binding-independent C++ and could be ported largely as-is. Note the code includes the
     python `Pickler` in the C++ library; check that nothing relies on SWIG internals there.
  2. **cereal**: header-only, supports `std::shared_ptr`, polymorphic types, `std::optional`, JSON/XML/binary, simpler API than
     Boost. Change is mechanical but touches every `serialize()` and removes the `BOOST_CLASS_EXPORT` macros in favor of
     `CEREAL_REGISTER_TYPE`. It breaks existing archived files, and I'm not sure about weak_ptr support.
  3. **nlohmann::json / hand-written `to_dict`/`from_dict`** plus python `__reduce__`: easy to read, human-debuggable,
     very little machinery, but loses automatic object-graph tracking and polymorphism. You would need to design identity
     tracking yourself.
  4. **No C++ serialization**: pickle at the python level only (`__reduce__` built from constructor args). Simplest and
     enough if C++ persistence is not required. Depends on whether the downstream project needs the XML/binary C++ files.
- Suggested: keep (1) for the first port, because it isolates the binding migration from a format change, and revisit
  cereal later if boost::serialization becomes a build-time or maintenance burden.
- Regardless: keep the existing round-trip tests (`generic_object_map_test.cc`, `director_example_test.py`) as the
  acceptance suite.

## 6. Build system and packaging

- **Decision (from you): stay on autotools.** CMake's main advantage is Windows support, which you don't need, and it
  adds a second build language for little gain. Nothing in the nanobind plan requires CMake.
  - nanobind's documented path is CMake, but it also ships a pre-combined source file (`src/nb_combined.cpp`, meant for
    custom build systems **(verify in the version you install)**) plus its header include dir and the vendored `robin_map`
    headers. An automake rule can compile that into the module with the usual flags (C++17, `-fvisibility=hidden`,
    `-fPIC`). The conda/pip `nanobind` package exposes `nanobind.include_dir()` and `nanobind.source_dir()`, which a small
    `configure.ac` macro (like the existing `AFIDS_PYTHON`) can query, replacing the SWIG/doxygen checks.
  - `.pyi` stubs come from `python -m nanobind.stubgen`, which can run as a `make` rule after the module is built.
  - The existing `python/pyproject.toml` / `setup.py` pip install hook in `python.am` stays as it is.
  - The autotools pieces that go away anyway under nanobind are the SWIG/doxygen/`doxy2swig` rules,
    `create_python_init.py` and `swig_wrap.tmpl`. The `config/m4` distutils fixes still matter for `configure`.
  - Cost: you hand-maintain the module's compile rule, instead of getting `nanobind_add_module()` for free. That is
    one automake stanza, so it is small.
- **Dependency management**: `pixi.toml` is a good base. Add `nanobind`, `scikit-build-core`, `cmake`, `ninja`, `xtensor`
  (if chosen), `catch2` or `doctest`, and drop `swig`, `doxygen` and `libblitz`.
- **Tests**: see section 9. Short version: the framework can be swapped later via a compatibility header, and parallel
  runs are possible without swapping.
- **Python**: add `.pyi` stubs (nanobind `stubgen`), run mypy against them (mypy is already in the pixi env), and keep ruff.
  The `mkinit`-driven `from skeleton_swig.all import *` glob imports can become explicit imports from the single
  nanobind module.
- **CI/wheels**: with scikit-build-core, `cibuildwheel` becomes available. Nanobind's stable-ABI option (abi3, Python
  3.12+) can reduce the wheel matrix.

## 7. Suggested order of work

1. **Plain C++ cleanup** (section 2): `std::shared_ptr`/`optional`/`array`, `override`/`= default`, drop dead hacks. Keep
   SWIG working. Gives an immediate win and de-risks everything else. Update `swig_rules` typemaps in the same step.
2. **Spike: nanobind for the three hardest cases**, on a branch, in a scratch pixi env:
   a. `DirectorExample` + `DirectorExampleUser` + `weak_ptr` lifetime (the contents of `director_example_test.py`).
   b. `NumpySample` with `nb::ndarray` and the lifetime tests.
   c. Pickle round-trip of `GenericObjectMap` holding shared objects.
   If a, b and c pass, the migration is clearly feasible. If weak_ptr or directors are awkward, find out here.
3. **Array library decision** using the result of 2b and what downstream code needs (xtensor vs mdspan vs Eigen).
4. **litgen evaluation** on the headers after cleanup. Decide between generated and hand-written bindings.
5. **Build-system change within autotools** (nanobind module rule, drop SWIG/doxygen rules), once the binding choice is made.
6. Revisit serialization format (cereal vs boost) and C++20 only if there's a concrete reason.

## 8. Open questions for you

- How much must stay compatible with the downstream projects (geocal and others)? Is a "big-bang" port acceptable
  for them, or must SWIG keep working for some time?
- Are existing boost XML/binary archive files a compatibility requirement, or is python pickle the only persistence that
  matters?
- Is blitz's arbitrary base index / non-row-major storage actually used downstream, or is it effectively always zero-based
  row-major?
- ~~Are other target languages (Ruby) still relevant?~~ **Answered: no.** Ruby was an old requirement, so SWIG's multi-language
  advantage no longer applies and python-only tools (nanobind) are fine. The Ruby-era comments in `Printable` / `Exception`
  (the Mac 10.4.11 workaround) are dead and can go.
- Is the goal to explore (this repo is a test bed), or to produce a template that downstream projects will follow?

## 9. Unit testing (C++)

Counts below are from the skeleton, geocal (`/home/smyth/Local/geocal-repo`) and framework (`/home/smyth/Local/framework`).
They are grep counts of macro tokens, not a semantic analysis.

### What the existing tests use
| | skeleton | geocal | framework |
|---|---|---|---|
| `BOOST_AUTO_TEST_CASE` | 7 | 583 | 562 |
| `BOOST_FIXTURE_TEST_SUITE` (one per test file) | 3 | 184 | 200 |
| `BOOST_CHECK_EQUAL` | few | 763 | 1,561 |
| `BOOST_CHECK_CLOSE` | 0 | 1,420 | 1,034 |
| `BOOST_CHECK` | few | 669 | 620 |
| `BOOST_CHECK_MATRIX_CLOSE[_TOL]` (custom) | 2 | 123 | 746 |
| `BOOST_CHECK_THROW` | 0 | 6 | 21 |
| `BOOST_WARN_MESSAGE` | 0 | 52 | 4 |
| `BOOST_REQUIRE*`, `BOOST_FAIL`, `BOOST_ERROR`, `BOOST_CHECK_LT`, `..._SMALL` | 0 | ~16 | ~35 |
| `BOOST_CHECK_ARRAYAD_CLOSE[_TOL]` (custom) | 0 | 0 | 3 |

So the usage is shallow: basically auto test cases, one fixture suite per file, and a handful of check macros. There are
no data-driven tests, no custom test-tree manipulation, and no Boost.Test-specific plugins. Both big repos build all tests
into a single binary (`geocal_test_all`, `skeleton_test_all`) driven by a `test_all.sh` with `--run_test=<filter>`.
The matrix macros are defined in one place per repo (`unit_test_support.h`) as `BOOST_CHECK_LT(max(abs(A - B)), tol)`.
That is a blitz expression, so these macros have to be touched when blitz is replaced, whatever test framework is used.

### Parallelism: no framework change needed
Boost.Test has no built-in parallel execution, but it does have `--run_test=<suite>` and `--list_content`. Because the
tests are already one suite per file (about 180 to 200 suites per repo), a thin wrapper can fan out suites:
- **Automake's parallel test harness** (the default in modern automake): list one `TESTS` entry per suite, each running
  `./test_all --run_test=<suite>`, and then `make -j N check` runs them concurrently. The suite list can be generated at
  build time from `--list_content`.
- Or the same loop with `xargs -P` / GNU `parallel` in `test_all.sh`.
- Watch for tests that **share state**: temporary output files with fixed names, the `unit_test_data` directory (211 MB
  in geocal), working-directory changes, and global singletons. A process per suite isolates singletons (in-process
  state) but not files. Expect some tests to need unique temp paths before running all in parallel.
- Python tests already run in parallel with `pytest -n`.
- A bonus: per-suite processes make `valgrind` runs and failure triage easier.

### Should the framework change?
| Option | For | Against |
|---|---|---|
| **Stay on Boost.Test** | Zero migration work, already works, still maintained. Boost is staying anyway if Boost.Serialization is kept. The newer `BOOST_TEST(a == b)` gives expression-decomposition output, and `tolerance()` for floats. | Heavy template compile times, error output of the older `BOOST_CHECK_*` macros is plain, no sharding of its own. |
| **Catch2 v3** | Expression-decomposing `CHECK(a == b)`, readable output, `REQUIRE`/`CHECK` split, matchers, built-in sharding (`--shard-count` / `--shard-index`) **(verify)**, tags. In wide use, packaged on conda-forge. | v3 is a compiled library, not single-header. Fixtures are per test case (`TEST_CASE_METHOD`), not per suite. |
| **GoogleTest** | `TEST_F(Suite, name)` is the closest model to `BOOST_FIXTURE_TEST_SUITE`, sharding by env variables, `--gtest_filter`/`--gtest_list_tests`, ubiquitous. | No relative-percent comparison, so `BOOST_CHECK_CLOSE` needs a helper. A little more verbose than Catch2. |
| **doctest** | Very light, fast to compile, Catch-like syntax. | Smaller ecosystem, less sharding/matcher support. |

Gotchas that matter for any migration:
- `BOOST_CHECK_CLOSE(a, b, pct)` takes the tolerance **in percent**, and (by default) checks the relative difference against
  *both* values (a "strong" check). Catch2's `Approx` and gtest's `EXPECT_NEAR` do not behave the same way, and this macro
  is used about 2,450 times across geocal and framework. A compat helper that reproduces the Boost semantics exactly
  avoids silently loosening or tightening about two thousand checks. Do not just map it to `Approx(...)`.
- Fixture lifetime: Boost constructs the suite fixture fresh for each test case, which matches gtest `TEST_F` and Catch2
  `TEST_CASE_METHOD`.
- `BOOST_WARN_MESSAGE` (52 uses in geocal) has no direct equivalent in the others. Map to `WARN`/`INFO` or a log call.

### A path that brings the existing tests along
Write a **compatibility header** (it can live in `unit_test_support.h`, which each repo already has) that defines
the handful of macros above, plus `BOOST_FIXTURE_TEST_SUITE` / `BOOST_AUTO_TEST_CASE` / `BOOST_AUTO_TEST_SUITE_END`, on top
of the new framework. For example, the suite macro can open a namespace and record the fixture type, and the test-case
macro can expand to `TEST_CASE_METHOD(fixture, name)` (Catch2) or `TEST_F` (gtest) **(verify the expansion in a
prototype)**. The existing ~580 test cases per repo then compile unchanged, which lets you:
1. switch the runner without touching test files,
2. change the macro spellings gradually (or never), and
3. keep Boost.Test and the new framework side by side during the transition.
This is the part I would prototype first on the skeleton: its 7 test cases are enough to prove the pattern.

### Recommendation
- **Do parallelism now with suite fan-out**, independent of any framework decision.
- **Keep Boost.Test for now.** Decide on a switch only together with the Boost-dependency decision (serialization,
  section 5 of `serialization_options.md`). If Boost stays for serialization, there is little to gain from leaving
  Boost.Test; if Boost is being removed, choose Catch2 v3 or GoogleTest then.
- **Centralize the array-dependent checks now.** The matrix and AutoDerivative macros are the only tests that depend on
  the array library. Putting them behind one function (for example `check_close(A, B, tol)`) lets the array migration and the
  framework migration proceed independently.

## 10. The two real consumers: geocal and framework

The skeleton is a toy; `/home/smyth/Local/geocal-repo` and `/home/smyth/Local/framework` are what would actually use
anything developed here. Both were surveyed read-only (grep and wc counts, token occurrences not semantic analysis,
excluding generated wrappers, build output and `thirdparty/`). Numbers are approximate; the survey notes where something
could not be determined.

### 10.1 Side by side

| | skeleton | geocal | framework |
|---|---|---|---|
| C++ (non-generated) | ~2.4k lines | 412 `.cc` / 330 `.h`, ~125k lines (incl. 182 test files, thirdparty) | 494 `.cc` / 368 `.h`, ~124k lines |
| Other languages | | thirdparty libraw, libsdbx | **~360k lines of Fortran** (LIDORT, 2stream), Lua + luabind (~221 files mention Lua) |
| SWIG | 9 `.i` | 247 `.i`, 14.6k lines, ~310-330 exposed classes | 324 `.i`, 20k lines, ~400-424 exposed classes |
| Python | small | 17k lib + 10k tests | 8k lib + 4k tests |
| Build | autotools | **autotools** (29 `.am`) | **CMake** (16 `CMakeLists.txt`, since 2017; no autotools) |
| C++ standard | 17 | 17 | **14** |
| SWIG / Python pins | any | any | `swig<4`, `python<3.9` in `environment.yml` |
| `boost::shared_ptr` | ~60 | ~3,900 (+295 `make_shared`) | ~3,700 (+204 `make_shared`) |
| `std::shared_ptr` | 0 | 2 | 0 |
| `weak_ptr` | 2 | 15 | 13 (the Observer pattern) |
| blitz `Array<T,N>` uses | ~10 | ~2,400 (rank 1-2: ~86%, up to rank 7) | ~6,200 (rank 1-3 dominate, max rank 5) |
| Classes with `serialize()` | 8 | ~190 files | ~290 methods in ~274 files |
| Export macros | 8 | ~290 `GEOCAL_EXPORT_KEY` | ~325 `FP_EXPORT_KEY` |
| `%feature("director")` | 1 | 9 | **47** (incl. 6 `Observer<...>` template instances) |
| `%python_attribute` | ~5 | ~960 | ~1,270 |
| C++ tests | 7 cases | 583 cases in one binary | 562 cases in one binary |
| Test data | tiny | 211 MB | 304 MB (+ multi-GB ABSCO tables) |

Both repos share the `swig_rules` heritage (`GenericObject`, `%python_attribute`, `my_shared_ptr.i`, the
`SerializationContext` machinery), so they are "the same problem twice", but they differ in build system and in how
deep the blitz and autodiff usage goes.

### 10.2 What this changes in the conclusions

1. **The scale is ~700 exposed classes and >2,200 `%python_attribute` uses.** Hand-porting `.i` files is not realistic;
   a tool is. The `.i` files are very regular (class declaration, a few constructors, `%python_attribute(name, type)`,
   a list of methods). That makes the `.i` the *specification of the Python API* (which members are exposed and which
   are properties), whereas the C++ header is not. **A converter script that reads the `.i` and emits nanobind code**
   preserves the existing Python API exactly. litgen works from the header, and so does not know which members the
   `.i` turned into properties or hid. That reverses part of my earlier lean toward litgen: it is still worth testing
   for docstrings and stubs, but it is probably not the primary migration route for these two repos. Keeping the Python
   API stable matters, because there are ~27k lines of Python library code (geocal) and ~8k (framework) plus user scripts.
2. **Blitz replacement is the dominant cost and risk, bigger than I implied in section 3.**
   - framework: ~6,200 array uses, ~1,200 `.reference()` rebinds, ~850 `.dataFirst`, ~650 `ColumnMajorArray` (Fortran
     layout used for zero-copy Fortran interop), ~600 `Range::all` slicing, ~215 tensor index-notation uses
     (`firstIndex`, `secondIndex`, `thirdIndex`), and **autodiff built on blitz**: `AutoDerivative<T>` (126 files) and
     `ArrayAd<T,D>` (a value array plus a rank D+1 Jacobian, mentioned in ~313 files).
   - geocal: ~2,400 uses, mostly rank 1-2 `double`, with ~320 `Range::all`, ~100 `Range(`, ~85 `.reference()`.
     `setBase`, Fortran storage and `GeneralArrayStorage` are unused.
   - In both, **non-zero base is essentially unused** (`setBase` 0; `lbound`/`ubound` in 3 files for geocal and 8 for
     framework), which removes one of the main blitz-vs-xtensor worries.
   - `.reference()` (rebinding an array handle to other storage) has **no clean equivalent in xtensor/Eigen/mdspan**
     for owning containers. Those 1,300 uses would need a shared-buffer handle type or a rewrite to value/view
     semantics.
   - **Conclusion: decouple the array migration from the binding migration.** nanobind can bridge blitz with a
     custom `nb::ndarray` caster, so the first port can keep blitz internally (the same way the SWIG typemaps do now,
     but ~900 lines of macro kludge replaced by one caster that supports ColumnMajor and non-contiguous strides). Then
     replace blitz later, one library at a time, only if there is a payoff. This also means `libblitz` stays a dependency for
     a while, which is fine: the framework repo already vendors a blitz tarball and a conda recipe. In my first
     version I treated the array library change as part of the same migration, and that now looks like the wrong plan
     for repositories of this size. Blitz is also the *least urgent* thing to replace: it is unmaintained but stable.
   - If blitz is eventually replaced, framework's autodiff makes **xtensor's expression templates + views** (needed for a rank
     D+1 Jacobian slice) the closest fit, and any choice must support Fortran order. Prototype `ArrayAd` first, since it
     is the hardest case.
3. **`std::shared_ptr` conversion is confirmed as the right first step, and it is mechanical.** ~7,600 uses, with
   essentially zero `std::` smart pointers today, and **no** `enable_shared_from_this` / `shared_from_this` /
   `intrusive_ptr` in either repo, which removes the usual hard cases. It can be scripted (a textual replacement of
   `boost::shared_ptr` / `weak_ptr` / `make_shared` / `dynamic_pointer_cast` followed by compile-driven fixes) and done
   while SWIG still works. It does require `swig_rules` to work with `std::shared_ptr` first (`my_shared_ptr.i` is a
   fork of the *boost* SWIG header; SWIG's `std_shared_ptr.i` is the starting point for a new fork), and both
   repos have to move together because `swig_rules` is shared. One oddity to understand first: framework's
   `Observer<T>` holds a private `shared_ptr<Observer> this_obj` (built with `to_ptr(*this)`), a self-referencing pattern
   that the conversion and the nanobind lifetime rules both have to respect.
4. **Virtual inheritance from `GenericObject` is now a first-class issue.** `public virtual GenericObject` (via
   `Printable`) appears ~153 times in framework, and ~18 of its `.i` classes have multiple bases. As I understand it,
   nanobind supports only single inheritance for bound class hierarchies, and downcasting through virtual bases needs
   care **(verify both in the spike)**. If so, the `Printable<T>`/`GenericObject` design (a virtual base used as a
   marker/mixin) should be reworked first, in plain C++, so each class has one primary bound base and `Printable`
   is a non-virtual mixin or free function.
5. **Directors are a bigger piece of the work in framework (47 vs 9).** 10+ core interfaces (`Pressure`, `Altitude`,
   `Absorber`, `ForwardModel`, `StateMapping`, `Level1b`, ...), the `*ImpBase` classes, template `Observer<...>`
   instances, and `LuaCallback`. Each needs a trampoline class (a mechanical pattern, and also generatable from the
   `.i` files' `%feature("director")` list). The Observer instances are the unusual ones.
6. **Boost cannot be dropped from these repos, so "remove Boost" is not a goal.** Besides serialization (below), they use
   `boost::math` (~560 uses in geocal), `lexical_cast`, `format`, `spirit`/`phoenix`/`fusion`/`geometry` (single uses in
   framework), `iostreams` gzip, `date_time`, and `rational`. The trivially replaceable part (`shared_ptr`, `array`, `optional`,
   `foreach` (~110 uses each), `filesystem`, `regex`, `noncopyable`, `bind`/`function`) is worth doing for its own
   sake, but the remaining Boost dependency is real. That undercuts the main argument for switching test and
   serialization frameworks purely to shed Boost.
7. **Build systems differ, which matters given your autotools preference.**
   - geocal is autotools, so the "stay on autotools, compile nanobind via `nb_combined.cpp`" plan applies directly.
   - **framework is CMake**, not autotools, and has Fortran (`enable_language(Fortran)`), `ExternalProject` for
     `thirdparty/`, and Findxxx modules for Eigen, NumPy, Luabind, Gperftools, plus a conda-recipe set. Moving it to
     autotools would be a project in its own right (Fortran + libtool is workable but not trivial) and is *not* needed for
     the nanobind migration, since nanobind's own CMake support is the best-documented path. Decision needed (see 10.4).
   - `swig_rules` already ships both an `.am` and a `CMakeLists.txt`, so whichever replaces it should keep both
     flavors; the nanobind-side glue is small in either.
   - framework also pins `swig<4`, `python<3.9` and C++14. Those need to move forward in any case; C++17 is the
     precondition for nanobind (and for `std::optional`, `std::filesystem`, etc.).
8. **Testing** (section 9) was already framed with these numbers; the survey confirms Boost.Test is used in the same
   shallow way in all three repos. With Boost staying, the case for changing frameworks is weak. Parallelism by suite
   fan-out helps most for geocal (581 cases) and framework (562 cases, 304 MB data).
9. **Serialization: see `serialization_options.md` section 7.** In short, the two repos make a per-class rewrite
   (cereal or pickle-native) much more expensive than I assumed, and make the weak_ptr/Observer scheme a real feature
   rather than a corner case.

### 10.3 Revised order of work (superseded by 10.7, which adds the archive corpus and your build and schedule decisions)

1. **Plain C++ cleanup in the skeleton**, then roll the same scripted changes into both repos:
   `boost::` to `std::` smart pointers (with the `swig_rules` typemaps updated to match), `foreach` to range-for,
   `boost::array/optional/filesystem/regex/noncopyable/bind/function` to `std::`, framework to C++17.
   Rework the `Printable`/`GenericObject` virtual-base structure at this point (item 4 above).
2. **Skeleton nanobind spike**, extended with what the two repos actually stress:
   - a director class that is also a template instance (`Observer<T>`),
   - a class with a virtual/multiple base,
   - a blitz array through a custom `nb::ndarray` caster including `ColumnMajor`,
   - the Observer/`weak_ptr` two-pass serialization, and pickling a Python subclass.
   - Decide whether a `.i` to nanobind converter is feasible by writing it for the skeleton's 9 `.i` files first.
3. **Pilot port: geocal first.** It is autotools (matching your preference), has fewer directors (9) and no autodiff arrays,
   and mostly rank 1-2 `double` arrays. It is the realistic test of the converter and of the pickle path.
4. **Then framework**, which adds directors (47), autodiff (`AutoDerivative`, `ArrayAd`), Fortran-layout arrays, the
   Observer pattern, Lua bindings and the CMake question.
5. **Blitz replacement**: only after the binding layer has moved, only if wanted, and with the `ArrayAd` prototype first.
6. **Serialization and test framework**: leave on Boost unless a concrete need appears (see the other document).

### 10.4 Decisions from you, and what follows from them

| Question | Your answer | Consequence |
|---|---|---|
| framework's build system | **Keep CMake for framework, autotools for geocal.** Different maintainers, not worth forcing to match. Supporting both is easy. | No build-system conversion in scope. The shared pieces (`swig_rules`, and its nanobind replacement) ship **both** an `.am` fragment and a CMake file, as `swig_rules` does today. nanobind's native CMake support is used for framework; geocal uses the autotools rule (compiling `nb_combined.cpp`). Both flavors should be exercised by the skeleton, which can carry a small CMake file alongside its autotools build as the reference for framework. |
| Lua/luabind | **Old, not needed, clean up at some point, low priority.** | Out of scope for the migration. Do not port `LuaCallback` or the Lua bindings; leave them behind (or delete) so they are not carried into nanobind. It also removes a director class and a chunk of framework's `.i` surface. Cleanup can happen before or after, but doing it *before* the binding port avoids converting code that will be thrown away. |
| Same schedule? | **No.** Different schedules, but they should not drift too far apart. | Treat `swig_rules` (and its replacement) as the shared contract, versioned and developed in the skeleton. Each repo adopts changes when ready, as it does today with the subtree. The risk to manage is the shared subtree diverging: keep the C++ cleanup steps (`std::shared_ptr`, etc.) in lock-step enough that the same `swig_rules` works for both, and keep SWIG working until both have moved or one has a stable fallback. |
| Long-lived pickles/archives | **Some long-term archives exist. Backward compatibility is not strict, but is a definite nice-to-have.** | See 10.5. This pushes the serialization decision firmly toward staying on Boost.Serialization, and makes an archive-compatibility test suite a priority deliverable. |
| Python API changes | Not yet answered. | Still open (below). |

### 10.5 Compatibility with existing archives

Because long-term archives exist and compatibility is wanted but not mandatory:
1. **Staying on Boost.Serialization is the lowest-risk path.** Class layouts and `serialize()` methods are unchanged, so
   old archives keep loading.
2. **The `std::shared_ptr` conversion is the main compatibility risk, and needs to be tested rather than assumed.** The
   repo's own `serialize_shared_ptr.h` writes a `shared_ptr` as its raw pointer (`"px"`) with class version 1 and
   `track_never`. If the converted `std::shared_ptr` save/load keeps the **same element name, version and tracking
   traits**, the on-disk form should be unchanged. Whether Boost's built-in `std::shared_ptr` support (if used instead
   of the repo's copy) writes the same thing is **not verified**; I would keep the repo's own save/load and only change the
   type.
3. **Build a golden-archive corpus now, before changing anything.** Collect archives written by the *current* code: framework's
   ~10 `.bin.gz` and the XML fixtures, geocal's `.xml` fixtures, plus any real long-term archives you are willing to share
   or regenerate. Add a test (C++ and via pickle) that loads every file and checks key values. Run it before and after each
   step (`std::shared_ptr` conversion, `Printable`/`GenericObject` rework, Boost upgrade, nanobind). Each step either
   keeps it green or knowingly breaks it. This corpus is what turns "nice to have" into something enforceable.
4. **`Printable`/`GenericObject` rework is the second compatibility risk.** If `GenericObject` is a polymorphic base
   registered with `void_cast_register` / `base_object`, changing the hierarchy (dropping `virtual` inheritance, changing
   which class is the registered base) can change the class-id or the base-object records in the archive. The golden
   corpus will say so. Where it does, the options are a Boost class-version bump with a `load()` that accepts both
   forms, or a one-off converter.
5. **Python pickles:** pickles embed the Boost binary blob, so they follow the same rule as archives, plus the pickled
   *Python class path* (module and class name). nanobind's module layout (`geocal_swig.*` vs a single module) changes
   the module name a pickle refers to. A `copyreg`/alias shim for old names could keep old pickles loading, and
   should be tested with a pickle taken from the SWIG version.
6. **If a format change is ever chosen** (cereal, or anything else), the safe way to keep old archives readable is a
   **one-time converter**: a program built against the old code that reads the old archive and writes the new one, kept
   in the repo or released with the format change. Without it, backward compatibility is lost for the existing
   archives.

### 10.5a Refinement: the archives that matter are Boost XML files; pickle is secondary

You said the long-term archives are Boost XML files, and pickle compatibility matters less. That sharpens 10.5:
- **Corpus focus**: XML only. Framework's 9 `*.bin.gz` fixtures are test data, not long-term archives, so they can be
  regenerated (or converted to XML) rather than preserved. The XML files already in the test trees
  (framework `omi_underlying_forward_model.xml` and similar, geocal `image_coordinate_test.xml` and similar) plus the real
  long-term files form the corpus.
- **What an XML archive depends on** (this is the checklist for "will old files still load"):
  1. the element names (`SKELETON_NVP_` / `*_NVP_` names, so renaming a member's NVP breaks old files),
  2. the `class_id`/`class_name` and export-key strings from `X_EXPORT_KEY`/`X_EXPORT_KEY2` (renaming or moving a class
     without keeping its key breaks it; geocal's CLAUDE.md notes this already happened once),
  3. class `version` numbers and `tracking`/`object_id` attributes for pointers (affected by changes to `shared_ptr`
     save/load traits, 10.5 item 2),
  4. the base-object / `void_cast` records (affected by the `GenericObject`/`Printable` rework, item 4),
  5. the array element names and shape (`rows`, `cols`, `data`, ...). **If blitz is ever replaced, the array save/load must
     keep these exact names and ordering**, or old files with arrays stop loading.
  6. the archive header version (`<boost_serialization ... version="N">`). A newer Boost reads older archives, but not
     the reverse, so upgrading Boost is safe for old files and old Boost cannot read files written by new Boost.
  Nanobind, the build system, and the module layout are not on this list, so they cannot affect XML files.
- **A converter is cheap for XML.** Because the files are text, a format fix-up (a renamed tag, a moved class key, a bumped
  version) can be done with an XSLT stylesheet or a short Python/lxml script, with no C++ converter needed. That lowers the
  cost of any future rename or class move, which is the realistic source of breakage here, much more than a library
  change would.
- **Pickle** compatibility is now optional: no alias shim is required for the module-name change. It is still worth one
  smoke test (pickle round-trip under the new build), but old SWIG-era pickles are allowed to break.

### 10.5b Compatibility policy (from your answer on file age and acceptable breakage)

The XML files that matter are recent (about the last year); older ones matter less the older they are; **some breakage is
acceptable, particularly if it is clearly identified.** That suggests a light policy, not a heavy guarantee:
1. **Compatibility window**: files written by releases from roughly the last year must keep loading. Older files are
   best-effort.
2. **Corpus stays small**: one or two representative archives per class family from the last year (the real files you
   have, plus the XML test fixtures already in the repos), refreshed as releases are made. Rolling snapshot rule:
   "archives written by release N must load under release N+1".
3. **Breakage must be loud and attributable.** A load failure should name the class, member or version at fault rather than
   produce wrong values. Boost XML archives generally throw on a missing or misnamed element or unregistered class **(verify the
   message quality for the common cases: renamed member, unknown export key, newer class version)**. Where messages are poor,
   wrap the `serialize_read*` entry point to add the file name and a hint. Also make sure no `serialize()` silently
   defaults a missing member.
4. **Intentional breaks are recorded**: a short list (e.g. `ChangeLog` or a `claude_notes`/`doc` page) of format-breaking
   changes, with the class version bumped (`X_CLASS_VERSION`) and, where cheap, a `load()` that accepts N-1. The
   geocal CLAUDE.md note about Ecostress objects is an example of an unrecorded break; this makes those explicit.
5. **A small XSLT or Python script** can upgrade older files for a deliberate rename, if one is ever wanted.

This lowers the stakes of the serialization work: a format change is no longer ruled out by old data. It remains unattractive
for cost reasons (section 7 of `serialization_options.md`), but if one ever becomes worthwhile, a year's worth of XML
files can be converted in one pass rather than supported indefinitely.

### 10.6 Remaining open questions
- **How much of the Python API can change?** An `.i`-based converter preserves it; litgen-style generation would not.
  Pickled class/module paths (10.5 item 5) are part of this.
- ~~Which long-term archives matter most?~~ **Answered: Boost XML files; pickle is less important.** **Age answered: recent, about the last year; older files matter less and some breakage is acceptable if clearly
  identified** (10.5b). Still useful to know which classes the real files contain, so the corpus covers them.
- **Is a dual-flavor build (autotools + CMake) of the skeleton wanted as the reference?** It is easy but is an extra
  maintenance item.

### 10.7 Updated order of work (reflecting 10.4)
1. **Golden-archive corpus of XML files** from the current code in geocal and framework (10.5 item 3 and 10.5a). Do this first.
2. **Lua/luabind cleanup in framework** if you want it, before porting (optional, low priority).
3. **Plain C++ cleanup** in the skeleton, then each repo on its own schedule: `std::shared_ptr`, `std::` replacements,
   framework to C++17, `Printable`/`GenericObject` rework. Re-run the corpus after each step.
4. **Skeleton nanobind spike** (both build flavors), then the `.i` converter.
5. **geocal pilot port**, **framework port** on its own timeline.
6. **Blitz replacement** only if wanted, afterward.

*(10.7 is superseded by the phase plan in `implementation_plan.md`.)*

## 11. Revisions after the geocal planning and the nanobind probes (2026-10-23)

The geocal plan (`/ssdata/smyth/geocal-repo/claude_documents/cpp_modernization_and_library_plan.md`) made decisions with
the maintainer, and a set of throwaway nanobind probes (`implementation_plan.md` §2.1) answered several "verify"
items above. Changes to this document's conclusions:

| Section | Earlier conclusion | Now |
|---|---|---|
| 2 item 11 | C++20 "later" | **C++20 now** (minimum gcc 14), in phase S1. C++23 after SWIG is gone. All skeleton sources already compile as gnu++20. |
| 2 item 1, 10.2 item 3 | `std::shared_ptr` first, mechanical | Unchanged, but done **after** C++20/std cleanup (S4), keeping the repo's own `px`/v1/`track_never` save/load, with byte-identical XML archives as the acceptance test. |
| 3, 10.2 item 2 | xtensor as the likely Blitz successor | **Keep Blitz.** Use the conda-forge `mdspan` reference implementation (pinned snapshot; C++17/20; has `submdspan` and owning `mdarray`) as the boundary/interop type. Tested over Blitz storage including strided slices and column-major (`layout_left`), which framework needs. |
| 4 (weak_ptr "main unknown") | To prototype | **Answered: a real problem.** nanobind makes a new control block for every Python→C++ `shared_ptr` conversion unless the type uses `enable_shared_from_this`, so `weak_ptr`s expire immediately (for every class, not only directors). Fix: `GenericObject : std::enable_shared_from_this<GenericObject>` plus a forked caster (below). |
| 10.2 item 4 | Rework `Printable`/`GenericObject` into non-virtual mixins first | **Dropped.** Not practical (geocal diamonds; framework has 169 virtual bases). Instead: keep virtual inheritance; the rules library ships a **fork of nanobind's 138-line `stl/shared_ptr.h`** (two changes: `dynamic_pointer_cast` in the `enable_shared_from_this` path, most-derived address via `dynamic_cast<void*>` on return). Never declare a virtual base as a nanobind base (it crashes). Tested in probes. Archive base-object records are unaffected, which removes the risk in 10.5 item 4. |
| 4, 10.2 item 4 | Multiple inheritance "verify" | nanobind allows one bound base. Secondary-base methods are bound on the derived class directly (tested); parameters typed as a secondary base or `GenericObject` need a small conversion helper (registry). |
| 6 | Autotools via `nb_combined.cpp` "(verify)" | **Verified** with nanobind 3.1.0: plain `g++` + `nb_combined.cpp`, about 5 s, no `libpython` link. |
| 6 | Drop `libblitz`, `doxygen` from the env | Keep `libblitz`. Doxygen stays for docstring generation (doxygen XML → nanobind docstrings) unless a better tool is chosen in S6.3. |
| 9 | Keep Boost.Test | Unchanged. |
| serialization | Stay on Boost.Serialization | Unchanged. Keep `--without-boost-serialization` while cheap; it is currently broken in 4 skeleton files (12 in geocal), so fix it and build it regularly. |
| new | — | `enable_shared_from_this` conflicts with geocal's/framework's `Observer::this_obj = to_ptr(*this)` (a non-owning self pointer would become `weak_this`). The Observer pattern must move to `weak_from_this()`. The skeleton needs an Observer sample to prove this (S5). |
| new | — | Bound virtual bases and wrong pointer adjustments can return correct answers by luck. Run binding tests under AddressSanitizer at each nanobind step. |
| 10.4 build | geocal autotools, framework CMake | Unchanged. The skeleton adds a small CMake reference for the nanobind part (resolves the open question in 10.6). |
| 10.6 | Python API stability open | Still open, but the `.i` → nanobind converter (S6.3) keeps the API close; geocal 2.0 allows documented breaks. |

framework-specific notes that came out of the new survey are in `implementation_plan.md` §2.3 (GSL is core there,
`ColumnMajor` arrays, 52 multiple-base SWIG classes, 47 directors, `python<3.9`/`swig<4` pins).
