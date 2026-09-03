This is a skeleton showing the use of the [swig-rules repository](https://github.jpl.nasa.gov/Cartography/swig-rules). This includes some sample code and
tests of the SWIG bindings.

This is also a good skeleton for a new system, just remove the source code
we have here but otherwise use the system.

It is useful to build this with a pixi environment, particularly since that gives you
a simple way to test out newer compiler and swig versions. We have a pixi.toml that can
be used as a starting point in env/linux. Note we often have Makefile in env in our other
repository for building the environment. We can add that, but at this point everything
is so simple and the whole point of the repository is to tweak things like the swig version.
So we don't currently have a Makefile there, although we can add one if it ends up being
useful.

