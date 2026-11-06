This is a skeleton showing the use of the [swig-rules repository](https://github.jpl.nasa.gov/Cartography/swig-rules). This includes some sample code and
tests of the python bindings.

**Naming.** The python bindings used SWIG until the 2026 modernization (phase S6 of
`claude_notes/implementation_plan.md`); they now use [nanobind](https://nanobind.readthedocs.io).
The names `swig-rules-skeleton`, `swig_rules/`, `swig-rules-repo` and the `skeleton_swig` python
package are historical and kept so paths and remotes keep working; `swig_rules/` now holds the
nanobind rules.

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
