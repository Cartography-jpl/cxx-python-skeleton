# nanobind probes (2026-10-23)

Throwaway test modules that produced the results in `../implementation_plan.md` §2.1 and §2.4. They are **not** part of
the build; they are kept as reference material for phase S6.1, where they should be turned into real tests.

Environment used: a disposable pixi env (`pixi.toml` here: conda-forge, `python=3.13`, `nanobind` 3.1.0,
`gxx_linux-64=15.*`). Build any probe without CMake:

```sh
E=<pixi env>; PY=$E/bin/python
NBI=$($PY -c "import nanobind;print(nanobind.include_dir())")
NBS=$($PY -c "import nanobind;print(nanobind.source_dir())")
PYI=$($PY -c "import sysconfig;print(sysconfig.get_paths()['include'])")
EXT=$($PY -c "import sysconfig;print(sysconfig.get_config_var('EXT_SUFFIX'))")
$E/bin/x86_64-conda-linux-gnu-c++ -std=c++20 -O1 -fPIC -fvisibility=hidden -shared \
   -I. -I$NBI -I$NBS/../ext/robin_map/include -I$PYI probeN.cpp $NBS/nb_combined.cpp -o probeN$EXT
$PY probeN_test.py            # probe6/probe4 tests take the module name as an argument
```
(`probe4.cpp` needs `-DMODNAME=...` and optionally `-DVIRTUAL_BASE`; `probe6.cpp` optionally
`-DBIND_VIRTUAL_BASE_IN_CHAIN`; `observer_probe.cpp` is plain C++: build with `-DUSE_TO_PTR` or `-DALIAS`.)

| File | Question |
|---|---|
| `probe.cpp`, `probe_test.py`, `run_each.py` | Downcasting, multiple inheritance, virtual `GenericObject`, directors, identity, exceptions, `weak_ptr` |
| `probe2.cpp` | Type-mapper workaround for `shared_ptr<GenericObject>`; `weak_ptr` with/without `nb::new_` |
| `probe3.cpp` | `enable_shared_from_this` + `weak_ptr` |
| `probe4.cpp` | `enable_shared_from_this` on a non-virtual vs virtual `GenericObject` root (virtual: compile error) |
| `probe5.cpp` | Non-virtual `GenericObject` root with mixins |
| `probe6.cpp` | geocal-style virtual diamond (`IgcRollingShutter`) |
| `probe7.cpp` / `probe8.cpp` | Forked caster `gc_shared_ptr.h` with virtual `GenericObject : enable_shared_from_this` (probe8: virtual base declared as nanobind base, crashes) |
| `gc_shared_ptr.h` | Fork of nanobind 3.1.0 `nanobind/stl/shared_ptr.h`; changes marked `GEOCAL FORK` |
| `observer_probe.cpp` | Observer `this_obj`: `to_ptr` vs aliasing constructor with `enable_shared_from_this` (C++ only, run with ASan) |
| `obs_nb.cpp` | Observer pattern through nanobind with the aliasing `this_obj` and the forked caster |
