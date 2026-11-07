# Python subclasses of C++ classes (nanobind)

These notes replace the SWIG-era ones (directors, `my_shared_ptr.i`,
`PythonRefPtrCleanup`, `SHARED_PTR_NO_OWN`), which went away with SWIG in phase S6 of
`claude_notes/implementation_plan.md`. The reference example and tests are
`lib/director_example.h`, `lib/director_example_nb.cc`, `python/tests/unit/director_example_test.py`
and `python/tests/unit/nanobind_test.py`.

## Trampolines

A C++ class that python may subclass and override (SWIG `%feature("director")`) gets a
nanobind *trampoline*: a C++ class derived from it whose virtual functions call the python
override if there is one (`NB_OVERRIDE_NAME`), otherwise the C++ one. The class is bound with
`nb::class_<Base, ..., PyBase>` and `nb::init<...>` constructors, so nanobind constructs a
`PyBase` when python constructs a subclass. The trampolines here were first written by
`i_to_nanobind.py` from the SWIG `.i` files and are maintained by hand now; write one for a new
class the same way (below).

The binding of a virtual function calls the C++ base implementation directly when the object
is a python subclass (`dynamic_cast` to the trampoline), so `super().func(v)` in an override
doesn't run the override a second time. (Without it the bound function would call the virtual
`func`, which goes back through the trampoline; nanobind notices the re-entry and calls the C++
base, so it doesn't recurse forever, but the override has run twice by then.)

### Adding a class python can subclass

Follow `lib/director_example_nb.cc`:

1. In an anonymous namespace, a trampoline `class PyBase : public Base` with
   `NB_TRAMPOLINE(Base)`, a public default constructor (needed for serialization), and one
   override per virtual function python may override: `NB_OVERRIDE_NAME("name", name, args...)`
   (`NB_OVERRIDE_PURE_NAME` for a pure virtual). Include the virtual functions of the bases.
   If the one argument is a reference to a bound class (e.g. `notify_update(const T&)`), use
   `CXX_PYTHON_RULES_NB_OVERRIDE_REF` instead: `NB_OVERRIDE_NAME` passes python a *copy* of
   the object (and fails for a class that can't be copied).
2. If the class is serialized, `CXX_PYTHON_RULES_NB_DIRECTOR_SERIALIZATION(PyBase, Base, Name)`
   at namespace scope (see "Serialization of python subclasses").
3. `nb::class_<Base, FirstNonVirtualBase, PyBase>` (leave out a virtual base, see
   `cxx_python_rules/nanobind/README.md`) with `nb::init<...>` constructors, not `nb::new_`.
4. Bind each virtual function as a lambda that calls `self.Base::func(...)` when
   `dynamic_cast<const PyBase*>(&self)` succeeds, and `self.func(...)` otherwise.
5. `cxx_python_rules_nb::register_type<Base>()` and then
   `cxx_python_rules_nb::register_alias<PyBase, Base>()`, plus `pickle_serialization(c)` if
   it is serialized.
6. If C++ calls the overrides from worker threads, see "Threads and the GIL".
7. Add tests in python: a subclass overriding each function, called from C++; and, if
   serialized, a pickle/shelve round trip of the subclass (`director_example_test.py`).

## Who owns what

* A python constructed object of a director class (subclass or not) is owned by its python
  object. The C++ object lives inside it.
* Passing it to C++ as a `std::shared_ptr` (the cxx_python_rules caster, `gc_shared_ptr.h`) gives a
  `shared_ptr` whose deleter only drops a python reference. So C++ keeps the python object
  (and with it the C++ object) alive for as long as it holds the pointer, after python drops
  its own reference. This is the same rule as with SWIG.
* `GenericObject` has `std::enable_shared_from_this`. The first such `shared_ptr` becomes the
  object's `weak_this`; while any C++ `shared_ptr` to it exists, later conversions reuse it,
  so all C++ owners share one control block, and `shared_from_this()` works.
* Classes that are *not* directors are constructed with `nb::new_` returning
  `std::make_shared`, so they always have a C++ control block (held by the python object).
* The same C++ object returned twice gives the same python object (nanobind keeps a map from
  C++ pointers to python objects). Under SWIG each return was a new wrapper.

## weak_ptr

A `weak_ptr` is valid while some C++ `shared_ptr` owns the object. For an object of a director
class owned only by python there is no such owner: the `shared_ptr` made for a call is
released when the call returns, and a `weak_ptr` set from it expires at once. This is decided
(implementation plan decision 6, S6.1 item 4). SWIG kept such a `weak_ptr` valid with the
`SHARED_PTR_NO_OWN` typemap, which was unsafe to keep in a `shared_ptr`; there is no
equivalent. Give the object a C++ owner (e.g. put it in a `GenericObjectMap`) if a `weak_ptr`
to it has to stay valid. The Observer pattern isn't affected: observer lists hold `weak_ptr`s
made from the observer's own `this_obj` (`lib/observer.h`), which lives as long as the
observer. `python/tests/unit/nanobind_test.py` has the whole matrix.

## Serialization of python subclasses

`CXX_PYTHON_RULES_NB_DIRECTOR_SERIALIZATION(PyBase, Base, Name)` (`cxx_python_rules_nb.h`) makes the
trampoline serializable, so a python subclass object can be in a boost archive (and pickled
through one):

* The archive has the C++ part (as `Base`) and `python_object`, a pickle of the python class and
  `__dict__`. The class name in the archive is `SwigDirector_<Name>` and the pickle has the form
  SWIG wrote, so archives written with SWIG still load
  (`unit_test_data/golden/director_python*.xml`).
* Reading makes the python object first, constructs the C++ object in place (owned by the
  python object), then `director_hook` (`cxx_python_rules/lib/director_hook.h`, set by the
  extension module) gives the archive's `shared_ptr` a reference to the python object, as for
  any `shared_ptr` from python.
* Known limitation: C++ objects in a python subclass's `__dict__` are pickled as separate
  archives, so sharing between them and the C++ part is lost
  (`test_director_example_object_tracking`, a strict `xfail`; re-entrant pickling was set
  aside).

## Threads and the GIL

Bindings hold the GIL, as SWIG did. A trampoline takes the GIL itself when it calls python, so
a python override can run on a C++ worker thread, provided the thread waiting for the worker
doesn't hold the GIL: bind such a function with `nb::call_guard<nb::gil_scoped_release>()`
(`ThreadExample.run`, `lib/thread_example_nb.cc`). A python exception raised on the worker is a
`nb::python_error`; carried back through a `std::exception_ptr` and rethrown, it reaches python
with its type and message.
