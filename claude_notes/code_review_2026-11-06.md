# Code review: quality and modernization (2026-11-06)

A fresh read of the C++ in `lib/`, `cxx_python_rules/lib/` and `cxx_python_rules/nanobind/`, done after phase S7 and
the rename. The python package and the autotools build were only looked at where they affect the C++. Uses the new
names (`cxx_python_rules`, `skeleton_cxx`).

## 1. Summary

**The code mostly looks like C++ written today.** After S1–S7 the library uses `std::shared_ptr`, `std::optional`,
`std::array`, concepts, a `std::formatter`, `std::format` with compile-time checked format strings,
`std::source_location`, ranges, `std::views::cartesian_product`, `mdspan` with `m[i, j]`, `= default`, `override` and
`noexcept`. clang-format is applied throughout. A compile of the library and its C++ tests with
`-Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast -Wsuggest-override -Wnon-virtual-dtor -Wconversion` (gcc 15.3)
gives no warnings except 101 `-Wsign-conversion` and 2 `-Wconversion` (§4.9).

What remains falls into three groups:

1. **Bugs** (§2). Two were confirmed with small test programs: blitz serialization silently scrambles column-major
   arrays, and copying an `Observer` makes the copy register the original. The rules library also hard-codes the
   `Skeleton::` namespace in one place, which will break when geocal copies it.
2. **Older idioms** (§3, §4). These are mostly small and local: implicit converting constructors, `typedef`,
   `const_cast` used instead of `mutable`, `std::map` used as a public base class, repeated code in the serialization
   files, macros using `__FILE__`/`__LINE__`, and two kinds of exception used for the same errors.
3. **Binding code style** (§5). The `*_nb.cc` files (2,663 lines) still look generated: a lambda wrapper for every
   member, python keyword names that don't match the C++ parameter names, and empty scope blocks.

Some legacy things are deliberate and should stay (§6).

## 2. Bugs and risks

Ordered by severity.

### 2.1 Column-major blitz arrays come back scrambled after serialization (confirmed)

`lib/blitz_array_serialize_support.cc` (the `save`/`load` pairs for ranks 1–5) checks only that the array is contiguous,
then writes `A.data()` in memory order. A column-major array (`blitz::ColumnMajorArray`, framework's Fortran interop) is
contiguous, so it is accepted, written in column order, and read back as row-major with the same shape:

```
orig  [ 0  1  2 ;  10 11 12 ]
loaded[ 0 10  1 ;  11  2 12 ]      (2 x 3, ColumnMajorArray, XML archive round trip)
```

No error is raised. `array_sample.h` mentions the limitation in a comment, but the code accepts the array without any
check. The base index (a nonzero `lbound`) isn't written either, so it is lost too.
**Fix:** in `save`, throw unless the storage is row-major with zero base (or copy to a row-major temporary before
writing, which keeps the archive format). Copy the fix to geocal and framework, which have the same code.

### 2.2 Copying an `Observer` or `CacheInvalidatedObserver` copies its self pointer (confirmed)

`Observer()` sets `this_obj = to_ptr(*this)` (`lib/observer.h:41`), and the compiler-generated copy constructor copies
`this_obj`, so the copy's `this_obj` points to the **original**. Registering the copy registers the original:

```cpp
ObserverExample a("a");
ObserverExample b(a);
o->add_observer(b);
o->value(5);          // a notified 1, b notified 0
```

If `a` is then destroyed, the weak pointer expires and `b` is never notified. `CacheInvalidatedObserver`
(`observer.h:89`) has the same problem. The code comes from framework, so framework has it too.
**Fix:** define the copy constructor and copy assignment in both classes so they make a new `this_obj` (or delete
them). Moves have the same problem.

### 2.3 The rules library hard-codes `Skeleton::`

`cxx_python_rules/nanobind/blitz_array.h:96-100` uses `Skeleton::strided_mdspan`, `Skeleton::md` and
`Skeleton::to_blitz`, and includes `blitz_mdspan.h` from `lib/`. Everything else in `cxx_python_rules/` uses
`CXX_PYTHON_RULES_NAMESPACE`. In geocal (`GeoCal`) this won't compile unless geocal also defines these names in a
namespace called `Skeleton`. **Fix:** use `CXX_PYTHON_RULES_NAMESPACE::`, and either move `blitz_mdspan.{h,cc}` and
`skeleton_mdspan.h` into `cxx_python_rules/lib/` (they are generic) or document them as files every project has to
provide. This belongs in the "Notes for geocal" table.

### 2.4 Indices from python are not checked

- `DirectorExampleUser::apply_func(v, ind[, ind2])` (`director_example.h:141-145`) indexes with `operator[]` and an
  `int` from python. An out-of-range index is undefined behavior (it can crash the interpreter).
- `generic_object_index(int I)` (`file_reference_sample.h:62-65`): same problem.

Use `.at()`, or `range_check`, in any function that takes an index from python.

### 2.5 Archive loading changes the process's working directory

`serialize_read_generic` and `serialize_read_binary_generic` (`serialize_function.cc:391,414`) use `DirChange`
(`chdir`/`fchdir`, lines 149-176) for the whole load, so that relative file names in the archive resolve against the
archive's directory. The current directory is shared by the whole process. Another thread doing relative file I/O at
the same time (python threads, geocal's `ThreadPool`) uses the wrong directory, and two concurrent loads from different
directories interfere with each other. Unlike the write functions, the read functions don't take a lock.
**Better:** keep a thread-local "archive base directory" that objects like `FileReferenceSample` resolve relative
names against, and drop the `chdir`. Until then, document that loading isn't thread safe.

### 2.6 Smaller issues

| Where | Issue |
|---|---|
| `thread_example.cc:12-28` | If creating the k-th `std::thread` throws, the threads already started are destroyed while still joinable, and the program calls `std::terminate`. `std::jthread` avoids this. A negative `Number_thread` becomes a huge `vector` size. |
| `director_example.h:38`, `multi_base_example.h:68`, `generic_object_sample.h:13` | Default constructors (used for serialization) leave `int`/`double` members uninitialized (`value_to_add_`, `focal_length_`, `number_band_`, `v_`). Loading sets them, but `GenericObjectA()` is public and leaves `v_` uninitialized. Use default member initializers (`int v_ = 0;`). |
| `raster_sample.h:78-84` | `read_ptr` builds the mdspan before `check_region` runs, so a negative size breaks the mdspan constructor's precondition before the check can throw. Check first. |
| `skeleton_exception.h:183-273` | `range_check*` builds a default `Exception`, so its `source_location()` points into the header, not at the caller. |
| `lib/director_example.h:23,27,110,113` | Constructors and destructors write to `std::cerr`. This is test output inside the library. A counter the tests can read would do the same job without the noise. |
| repo root | `compile_commands.json` is a symlink to the pre-rename build directory (`swig-rules-skeleton-build`) and now points nowhere. It's git-ignored; run `make lint` again to recreate it. |

## 3. Library modernization still to do

These are not bugs. Each one is small. Where/what/today's idiom:

### 3.1 Constructors and ownership

- **Implicit converting constructors.** One-argument (or defaulted) constructors aren't `explicit`: `Exception(const
  std::string& = "")`, `ObservableExample(double)`, `ObserverExample(const std::string&)`,
  `CacheExample(shared_ptr)`, `ValueSample(double)`, `NumpySample(const blitz::Array&)`,
  `ColumnMajorSample(int, int = 3)`, `CxxPythonRulesSample(int = 0, ...)`, `GenericObjectA(int)`,
  `GenericObjectB/B2/C(...)`, `DirectorExampleWeakPtr(...)`, `MultiBaseExample(...)`, `VirtualDiamondExample(...)`,
  `FileReferenceSample(const std::string&)`, `IostreamSwig(...)`, and the `Filtering*`/`*Compress`/`DirChange`
  helpers. `MemoryRasterSample(const blitz::Array&)` is already `explicit`, which is the right pattern. Python isn't
  affected (nanobind uses the constructors explicitly).
- **Sink parameters.** Constructors that store a `shared_ptr` or `std::string` take `const&` and copy. Today's
  idiom is to take the parameter by value and `std::move` it into the member. `DirectorExampleNested` takes
  `const std::shared_ptr<DirectorExample> Obj` (a const value, which can't be moved). Also
  `add_observer_and_keep_reference(std::shared_ptr<Observer<T>>&)` takes a non-const reference it doesn't modify.
- **Default member initializers** instead of constructor initializer lists for constants:
  `CacheInvalidatedObserver::cache_valid(false)`, `CxxPythonRulesSample::scale_(1.0)`.
- **Redundant destructors.** Almost every class declares `~X() override = default;`. `GenericObject` already has the
  virtual destructor, so by the rule of zero these declarations can go (`NumpySample` already does without one). They
  aren't wrong, just noise. Pick one convention and follow it.

### 3.2 `Printable` and `print`

- `print` isn't declared in `Printable<T>`, so each class adds its own. The code uses three different forms:
  `virtual void print(...) const` (most), `void print(...) const override` (raster, camera), and a plain `void
  print(...) const` (`GenericObjectA`, `DirectorExampleWeakPtr`). Use `override` where a base already has `print`,
  and plain `void print` otherwise. The `PrintableObject` concept could also check that `T::print(std::ostream&)` exists.
- The class comment still talks about Ruby. `desc()` uses `""` to mean "none" (`if(d != "")`); `!d.empty()`, or
  `std::optional<std::string>`, would say that directly.

### 3.3 Exceptions

- **Two kinds of exception for the same kind of error.** `lib/` throws `std::runtime_error` in
  `ArraySample::matrix`, `SimpleCameraLike::parameter`, `FileReferenceSample::read` and `to_mdspan`/`to_blitz`, and
  `Skeleton::Exception` elsewhere. Python sees both as `RuntimeError`, but only `Exception` carries a stack trace and
  source location. Use `Exception` everywhere in `lib/`. The generic code in `cxx_python_rules/` can't, and
  `std::runtime_error` is fine there.
- **`range_check` macros.** `range_check(V, Min, Max)` and the others are macros only so they can capture `__FILE__`
  and `__LINE__`. Today these would be inline functions with a defaulted `std::source_location` argument, and they
  would use the format constructor of `Exception`. Keep the macro names as thin wrappers if geocal code uses them.
- `Exception` derives from `Printable`, and so from `GenericObject` and `enable_shared_from_this`. Every exception
  therefore carries a vtable and a weak pointer, and every copy also copies the stack-trace vector. This is
  acceptable, but it is heavier than necessary. `std::stacktrace` (C++23) could replace Boost.Stacktrace once
  the minimum gcc ships it without `-lstdc++exp`; keep Boost for now.

### 3.4 Containers and algorithms

- `GenericObjectMap` **publicly inherits** `std::map` (`generic_object_map.h:32-34`). This was flagged in
  `modernization_ideas.md` §2.7 and is still the case. Holding the map as a member, with `keys/get/set/contains/size`
  and iteration exposed explicitly, is cleaner. The archive format stays the same if `serialize` writes the member
  under the same name. Also: `keys()` can be `std::views::keys(*this) | std::ranges::to<std::vector>()` (gcc 14 has
  `ranges::to`), and `get_generic` can use `auto` instead of the spelled-out iterator type.
- Hand-written sum loops (`sum_values`, `sum_int_vector`, `ThreadExample::run`) → `std::ranges::fold_left` or
  `std::accumulate`.
- `Observable::PointerEqual`/`CPointerEqual` are hand-written function objects that lambdas could replace. Comparing
  `w.lock().get() == t_` takes a lock just to compare pointers.
- `std::lock_guard<std::mutex>` → `std::scoped_lock` (CTAD). `reinterpret_cast<std::size_t>(ptr)` →
  `std::uintptr_t`. `(int)V.size()` (`array_typemap_sample.h:163`) → `static_cast`.

### 3.5 `const` correctness

`Observable::notify_update_do` is `const`, but it uses `const_cast` to call `clean_dead_ptr()`, and
`CacheExample::value()` uses `const_cast` to set `cache_valid`. Declare the observer lists and `cache_valid` `mutable`
instead; that is what `mutable` is for. (The `const_cast`s in `ArrayTypemapSample::set_value*` are deliberate,
because those functions test aliasing.)

### 3.6 Declarations and headers

- `typedef` → `using`: `SKELETON_EXPORT_OBSERVER_KEY`, `generic_object_map.cc`, `blitz_array_serialize_support.h`.
- **Unused typedefs.** `blitz_array_serialize_support.h` declares 24 typedefs in the global namespace
  (`blitz_double_array_1d` …). Nothing uses them; they are left over from SWIG. Delete them.
- **Include guard names are inconsistent.** Some are prefixed (`SKELETON_*`), some aren't (`ARRAY_SAMPLE_H`,
  `GENERIC_OBJECT_MAP_H`, `SERIALIZE_FUNCTION_H`), and one is misspelled (`WEAK_PTR_SERIALIZED_SUPPORT_H`). Unprefixed
  names can collide when geocal and framework headers are included together. Either use `#pragma once` (gcc, clang and
  MSVC all support it) or prefix every guard.
- **Configuration macros in public headers.** `generic_object.h` has to `#undef PACKAGE`, `VERSION` and so on before
  including `skeleton_config.h`, because the autoconf macros leak into installed headers. A configuration header with
  prefixed names (e.g. `AX_PREFIX_CONFIG_H`, or a hand-written `skeleton_config.h.in` that defines only `SKELETON_*`
  macros) removes the need for the `#undef`s.

## 4. Rules library (`cxx_python_rules/`)

1. **`serialize_function.cc` repeats the same code many times.** Seven functions each end in the same
   `#else throw ...` block, and the file and string read/write paths are copied per archive type. A small template
   (`write<Archive>(ostream&, obj)` / `read<Archive>(istream&)`) plus one `#if` would make it about half the size.
2. It uses POSIX `open`/`chdir`/`fchdir` (§2.5), takes `std::string` paths where `std::filesystem::path` would fit,
   and calls `.c_str()` on strings only to convert them straight back to `std::string`. The error messages don't
   include `errno` ("Open failed"). `#include <iostream>` is unused. `skip_weak_ptr_handling_val` is a plain `static
   bool`; the rest of the serialization state is guarded by `PtrSerializeLock`, so this flag should be atomic too.
3. **Deprecated Python C API.** `register_exceptions` (`cxx_python_rules_nb.h:142-157`) uses `PyErr_Fetch`,
   `PyErr_NormalizeException` and `PyErr_Restore`, which are deprecated since Python 3.12. Use
   `PyErr_GetRaisedException`/`PyErr_SetRaisedException` (or nanobind's `nb::python_error`). The environment
   uses 3.13.
4. **Free-threaded Python** (3.13t/3.14t) isn't supported. `nbreg::registry()` starts with an unguarded check of a
   static pointer, and `pending_director()` is an unguarded map; both are safe only because the GIL serializes access.
   This is fine for now. Revisit it if free-threaded builds are ever wanted.
5. **Blitz serialization repeats per rank.** `blitz_array_serialize_support.{h,cc}` (about 500 lines) has one
   `save`/`load` pair for each rank from 1 to 5. One template over `N`, with a table of the XML element names
   (`size`, `rows`, `cols`, `depth`, …), would write identical archives in about 60 lines. The fix for §2.1 would
   then go in one place.
6. `blitz_array.h`: `(uint32_t)cast_flags::convert` → `static_cast`. In `to_numpy`, the `new blitz::Array` leaks if
   the capsule constructor throws; create the capsule from a `unique_ptr` and `release()` it afterwards.
7. `serialize_shared_ptr.h` and `gc_shared_ptr.h` are copies of upstream code and are excluded from formatting on
   purpose; they weren't reviewed for style.
8. **Lint coverage is thin.** `.clang-tidy` enables only 7 `modernize-*` checks. Candidates that would have caught
   items in this review: `google-explicit-constructor`, `cppcoreguidelines-pro-type-member-init`,
   `modernize-use-using`, `modernize-use-default-member-init`, `bugprone-*`, `performance-unnecessary-value-param`,
   `readability-container-contains`. Per the file's own rule, add each one only after the code is clean for it.
9. **Compiler warnings.** Release builds get `-Wall` only. Debug builds in the scratch environment get
   `-Wall -Wextra -Wdeprecated`. The library is already clean under `-Wpedantic -Wshadow -Wold-style-cast
   -Wsuggest-override -Wnon-virtual-dtor`, so those could be turned on. The remaining `-Wsign-conversion` warnings
   are int/`size_t` mixing: 38 in `blitz_mdspan.h`, 35 in a test, 12 in `director_example.h` (§2.4), and the rest
   scattered. Fixing those is optional.

## 5. The bindings (`lib/*_nb.cc`)

The bindings are correct and well tested. Their style still comes from `i_to_nanobind.py`, which matters now that they
are maintained by hand:

- **Lambda wrappers everywhere.** 197 `[](const T& self, ...) -> decltype(auto) { return self.f(...); }` wrappers.
  For non-virtual members, `c.def("name", &T::name)` (or `nb::overload_cast<...>(&T::f, nb::const_)` for overloads)
  does the same thing in one line. **Keep** the `dynamic_cast<const PyX*>` dispatch lambdas for virtual functions of
  director classes, because they stop a python override that calls `super()` from recursing forever.
- **Python keyword names that don't always match the C++.** Most match the C++ parameter names, but
  `DirectorExample(int value_to_add, const std::string& name)` is bound as `nb::arg("V"), nb::arg("Name")`, and three
  parameters are called `arg0`. Keyword names are part of the python
  API, so it's worth settling them before geocal copies the pattern.
- `{ auto c = nb::bind_vector<...>(...); }` blocks bind a variable `c` that is never used. Drop the variable and the
  braces.

A single pass with the generator's rules turned around (bind member pointers by default, use lambdas only for
director dispatch and type conversions) would roughly halve the 2,663 lines.

## 6. Legacy that should stay

- **blitz, Boost.Serialization, Boost.Test, Boost.Stacktrace.** These were chosen on purpose in the plan; mdspan now
  covers the interface boundaries.
- **The private-member trick in `blitz_mdspan.cc`.** It's the only way to attach an owner to a `blitz::Array`; it is
  documented, and it replaced a worse `#define` hack.
- **Names kept for archive compatibility:** `"SwigDirector_"` prefixes, the `"geocal_object"` element name, and
  `IostreamSwig`/`swig_shelve` (public names in geocal).
- **`int` line/sample sizes in `RasterSample`.** They match geocal's API.
- **The `new OwnerMemoryBlock` in `to_blitz`.** Blitz takes ownership of the block through its own reference count.

## 7. Suggested order

1. Fix §2.1 (column-major serialization) and §2.2 (Observer copy). Add both, plus §2.3, to the "Notes for geocal"
   table, since geocal and framework have the same code.
2. Fix §2.3 (namespace in `blitz_array.h`) before geocal copies `cxx_python_rules/`.
3. Quick, mechanical fixes: `explicit`, default member initializers, `typedef` → `using`, delete the unused
   typedefs, `mutable` instead of `const_cast`, `.at()` for python indices, `std::jthread`, consistent include
   guards, `PyErr_GetRaisedException`. Turn on the matching clang-tidy check after each one.
4. Medium refactors: rank-generic blitz serialization, a deduplicated `serialize_function.cc`, `GenericObjectMap` by
   composition, `range_check` with `source_location`, one exception type in `lib/`.
5. A bindings cleanup pass (§5), and agree the python keyword names first.
6. Design change: replace the `chdir` during archive loads with a thread-local base directory (§2.5).
