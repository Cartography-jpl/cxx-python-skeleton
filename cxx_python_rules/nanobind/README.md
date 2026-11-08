# nanobind rules

The nanobind replacement for the SWIG rules.

| File | What |
|---|---|
| `cxx_python_rules_nb.h`, `cxx_python_rules_nb.cc` | Include first in every binding file. Exception translation, pickling helpers (`pickle_serialization`, `pickle_init`, ...), `printable`, docstring lookup (`doc`, and `docs` for one class), director serialization (`CXX_PYTHON_RULES_NB_DIRECTOR_SERIALIZATION`), `blitz::Range` and `boost::rational<int>`. |
| `gc_shared_ptr.h` | Fork of nanobind's `std::shared_ptr` caster (nanobind 3.1.0), for classes with virtual bases. The changes are marked `CXX_PYTHON_RULES FORK`; re-check against upstream at each nanobind upgrade. Never include `nanobind/stl/shared_ptr.h` as well. |
| `type_registry.h` | Bound classes by C++ type: most derived python type for a returned `shared_ptr`, and passing a derived object for a parameter typed as a secondary or virtual base (or `GenericObject`). Shared by all extension modules through a capsule in `builtins`. |
| `blitz_array.h` | numpy <-> `blitz::Array<T, N>` through an `mdspan` view (`cxx_python_rules/lib/blitz_mdspan.h`). |
| `std_array.h` | `std::array<number, N>` <-> numpy. Use instead of `nanobind/stl/array.h`. |
| `iostream.h` | A python file object as `std::istream` / `std::ostream`. |
| `nanobind.am`, `nb_combined.cc` | Automake rules: nanobind's source compiled once, the extension module, docstrings, `.pyi` stubs. |
| `doxy2nanobind.py`, `doxygen_nanobind.cfg` | Docstrings from the doxygen comments. |
| `i_to_nanobind.py` | One-time converter from SWIG `.i` files (written with the old `swig_rules` macros) to `*_nb.cc` binding code. Run it on all the `.i` files of a project together; see its docstring for what it converts and what it leaves for hand conversion (marked `HAND CONVERT`). It writes the style below (member pointers, `docs`, `"name"_a`); check the keyword names (taken from the `.i`) and add `.none()` where C++ accepts null. |

## Writing bindings

The bindings are ordinary nanobind code: anyone who knows nanobind (or pybind11) can read them,
and nanobind's documentation applies. One file per header, `lib/<name>_nb.cc`, with
`void init_<name>(nb::module_& m)`. A class looks like this:

```cpp
#include "skeleton_nb_opaque.h"   // the project's opaque vectors; includes cxx_python_rules_nb.h
#include "foo.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_foo(nb::module_& m)
{
  const auto doc = cxx_python_rules_nb::docs("Skeleton::Foo");
  nb::class_<Foo> c(m, "Foo", doc());
  c.def(nb::new_([](int Count) { return std::make_shared<Foo>(Count); }), "Count"_a);
  c.def("sum", &Foo::sum, "A"_a, doc("sum"));
  c.def("value", nb::overload_cast<>(&Foo::value, nb::const_), doc("value"));
  c.def("value", nb::overload_cast<int>(&Foo::value), "V"_a, doc("value"));
  c.def_prop_ro("count", &Foo::count, doc("count"));
  cxx_python_rules_nb::printable(c);
  cxx_python_rules_nb::pickle_serialization(c);
  cxx_python_rules_nb::register_type<Foo>();
}
```

* **Member pointers**, with `nb::overload_cast<Args...>(&C::f[, nb::const_])` for an overloaded
  name. This works for methods declared in a base that isn't the nanobind base (a secondary or
  virtual base) too: `class_<T>::def` calls the member pointer through a `T*`.
* **Keyword names are the C++ parameter names** in the header (`"Count"_a`), with the same
  defaults. They are part of the python API. `tests/unit/nanobind_test.py` pins a few.
* **Docstrings** come from the doxygen comments: `doc()` for the class, `doc("f")` for a member
  of it. A member documented under another class (its base) uses
  `cxx_python_rules_nb::doc("Ns::Base::f")`.
* **`register_type<T>()` for every class** (and `register_alias<Tramp, T>()` for a
  trampoline). `nanobind_test.py::test_every_class_registered` fails if one is missing.
* A **lambda only where it does something**:
  * the *director dispatch* for a virtual function of a class with a trampoline:
    `if(dynamic_cast<const PyFoo*>(&self)) return self.Foo::f(); return self.f();`. A python
    override that calls `super().f()` reaches the bound function, which would call the
    virtual `f` and come back through the trampoline. nanobind notices that re-entry and
    calls the C++ base (so there is no infinite recursion), but by then the python override
    has run a second time. Calling `Foo::f` directly avoids that. Write one comment per file
    saying so;
  * a parameter that is a *reference to a secondary or virtual base*, taken as a `shared_ptr`
    (the type registry converts any derived object) and dereferenced;
  * *output arguments* returned as a tuple, an argument C++ should get a *copy* of
    (`A.copy()`), *stream* arguments (`py_ostream`/`py_istream`, `iostream.h`; call
    `os.flush()` after the C++ call, so an error in the last write is raised), and a
    `std::array` member set with `to_std_array`;
  * `nb::new_` constructors returning `std::make_shared` (non-director classes);
  * conversions the python API needs (e.g. `std::filesystem::path` results as `str`, archives
    as `bytes`).
* A **virtual property** (one a python subclass overrides) goes through `_v_<name>` methods and
  a `def_prop_rw` that calls them (the SWIG `%python_attribute_virtual` pattern).
* Bind a template instance, or a pattern used by several classes, with a function template
  rather than copies (`lib/observer_nb.h` for `Observer<T>`/`Observable<T>`).
* **Module init** is plain code: `skeleton_nb_module.cc` declares the `init_<name>` functions
  and calls them, base classes first.

## Rules for binding code

* Never declare a virtual base as a nanobind base (`nb::class_<Derived, VirtualBase>`): nanobind
  uses a static offset, which is wrong for a virtual base and crashes. Bind the first
  non-virtual base only; bind the other bases' methods on the class (the converter does).
* Bind constructors of non-director classes with `nb::new_` returning `std::make_shared`, and
  call `cxx_python_rules_nb::register_type<T>()` for each class (`register_alias` for a trampoline).
* A `shared_ptr` argument accepts `None` only with `nb::arg(...).none()`. Add it only where C++
  handles a null pointer (a setter that clears, an optional link). Elsewhere leave it off, so
  nanobind rejects `None` with a `TypeError`; with `.none()` on a parameter C++ dereferences,
  python crashes. (SWIG accepted `None` everywhere, and the converter added `.none()` to every
  `shared_ptr` parameter.) C++ functions that dereference a pointer argument also check it and
  throw, since C++ callers can pass null too.
* Arguments from python are untrusted: check indices (`.at()`, `range_check`), including blitz
  `A(i, j)`, which only checks bounds in a `BZ_DEBUG` build. A C++ exception becomes a python
  exception through our translator: `std::out_of_range` becomes `RuntimeError` (not
  nanobind's `IndexError`, since our catch-all translator is tried first), and
  `Exception`/`TypeError` from the library become `RuntimeError`/`TypeError`.
* A trampoline override whose argument is a reference to a bound class uses
  `CXX_PYTHON_RULES_NB_OVERRIDE_REF`, not `NB_OVERRIDE_NAME`: the latter copies the object
  for the call into python.
* Compile the binding files *without* `-fvisibility=hidden` (see `nanobind.am`); nanobind's own
  source keeps it.
* Run the binding tests under AddressSanitizer at each change: a wrong pointer adjustment can
  give the right answer by luck.

## Python versions

* Python 3.12 or later: the exception translator uses `PyErr_GetRaisedException` /
  `PyErr_SetRaisedException` (the `PyErr_Fetch` family is deprecated since 3.12). `configure`
  checks it (`SKELETON_PYTHON_MINIMUM` in `config/m4_local/skeleton_common.m4`).
* **Free-threaded python (3.13t, 3.14t) is not supported.** The modules aren't declared
  `FREE_THREADED` in `NB_MODULE`, so a free-threaded interpreter turns the GIL back on when it
  imports them (with a warning), and everything works with the GIL as usual. Supporting it for real
  would need at least: a lock (or `std::call_once`) around the first use of the type registry
  (`nbreg::registry()` in `type_registry.h`) and of the cached module handles (`support()`,
  `numpy()`), a lock around `pending_director()` (director serialization), and a review of every
  binding for objects shared between threads.
