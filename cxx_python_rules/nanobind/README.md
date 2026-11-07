# nanobind rules

The nanobind replacement for the SWIG rules (phase S6 of
`claude_notes/implementation_plan.md` in cxx-python-skeleton).

| File | What |
|---|---|
| `cxx_python_rules_nb.h`, `cxx_python_rules_nb.cc` | Include first in every binding file. Exception translation, pickling helpers (`pickle_serialization`, `pickle_init`, ...), `printable`, docstring lookup (`doc`), director serialization (`CXX_PYTHON_RULES_NB_DIRECTOR_SERIALIZATION`), `blitz::Range` and `boost::rational<int>`. |
| `gc_shared_ptr.h` | Fork of nanobind's `std::shared_ptr` caster (nanobind 3.1.0), for classes with virtual bases. The changes are marked `CXX_PYTHON_RULES FORK`; re-check against upstream at each nanobind upgrade. Never include `nanobind/stl/shared_ptr.h` as well. |
| `type_registry.h` | Bound classes by C++ type: most derived python type for a returned `shared_ptr`, and passing a derived object for a parameter typed as a secondary or virtual base (or `GenericObject`). Shared by all extension modules through a capsule in `builtins`. |
| `blitz_array.h` | numpy <-> `blitz::Array<T, N>` through an `mdspan` view (`cxx_python_rules/lib/blitz_mdspan.h`). |
| `std_array.h` | `std::array<number, N>` <-> numpy. Use instead of `nanobind/stl/array.h`. |
| `iostream.h` | A python file object as `std::istream` / `std::ostream`. |
| `nanobind.am`, `nb_combined.cc` | Automake rules: nanobind's source compiled once, the extension module, docstrings, `.pyi` stubs. |
| `doxy2nanobind.py`, `doxygen_nanobind.cfg` | Docstrings from the doxygen comments (replaces `doxy2swig.py`). |
| `i_to_nanobind.py` | One-time converter from SWIG `.i` files (written with the old `swig_rules` macros) to `*_nb.cc` binding code. Run it on all the `.i` files of a project together; see its docstring for what it converts and what it leaves for hand conversion (marked `HAND CONVERT`). On the skeleton, 92 of the 2327 generated lines needed hand changes. |

## Rules for binding code

* Never declare a virtual base as a nanobind base (`nb::class_<Derived, VirtualBase>`): nanobind
  uses a static offset, which is wrong for a virtual base and crashes. Bind the first
  non-virtual base only; bind the other bases' methods on the class (the converter does).
* Bind constructors of non-director classes with `nb::new_` returning `std::make_shared`, and
  call `cxx_python_rules_nb::register_type<T>()` for each class (`register_alias` for a trampoline).
* `shared_ptr` arguments need `nb::arg(...).none()` to accept `None` (SWIG accepted it).
* Compile the binding files *without* `-fvisibility=hidden` (see `nanobind.am`); nanobind's own
  source keeps it.
* Run the binding tests under AddressSanitizer at each change: a wrong pointer adjustment can
  give the right answer by luck.
