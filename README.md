This is a skeleton showing the use of the [cxx-python-rules repository](https://github.jpl.nasa.gov/Cartography/cxx-python-rules). This includes some sample code and
tests of the python bindings.

**Naming.** The python bindings used SWIG until the 2026 modernization (phase S6 of
`claude_notes/implementation_plan.md`); they now use [nanobind](https://nanobind.readthedocs.io).
So in 2026-11 the repository was renamed from `swig-rules-skeleton` to `cxx-python-skeleton`, the `swig_rules/`
directory (and its upstream repository `swig-rules`) to `cxx_python_rules/` (`cxx-python-rules`), and the python
package with the extension module from `skeleton_swig._swig_wrap` to `skeleton_cxx._bindings`. `cxx_python_rules/`
holds the nanobind rules and the C++ support code they use. Pickles written before the rename don't load (they name
the old module); boost archives and shelves do.

This is also a good skeleton for a new system, just remove the source code
we have here but otherwise use the system.

It is useful to build this with a pixi environment, particularly since that gives you
a simple way to test out newer compiler and nanobind versions. We have a pixi.toml that can
be used as a starting point in env/linux. Note we often have Makefile in env in our other
repository for building the environment. We can add that, but at this point everything
is so simple that we don't currently have a Makefile there, although we can add one if it
ends up being useful.

autotools is the main build (`./bootstrap`, `configure`, `make`, `make check`). There is also a
small CMake reference build (`CMakeLists.txt`) of the library, the python module and the tests,
as an example for projects that use CMake.
