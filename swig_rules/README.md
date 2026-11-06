This contains generic rules for wrapping C++ code for python.

**The name is historical.** These were SWIG rules until phase S6 of the modernization
(`claude_notes/implementation_plan.md` in swig-rules-skeleton); the directory now holds the
nanobind rules (`nanobind/`, see `nanobind/README.md`) and the C++ support code they use
(`lib/`). The names `swig_rules`, `swig-rules-repo` and the `<package>_swig` python package are
kept so paths and remotes keep working.

This is not standalone code, it is meant to be included in a larger piece
of code.

Look for example at the [swig-rules-skeleton](https://github.jpl.nasa.gov/Cartography/swig-rules-skeleton) repository for a skeleton of using
this. A more full example can be found at 
[geocal](https://github.jpl.nasa.gov/Cartography/geocal).

Getting the SWIG rules into a repository
----------------------------------------

Normally you use git subtree to put this swig_rules. So assuming this has
been added as a remote swig-rules-repo you would do something like:

    git subtree pull --prefix swig_rules swig-rules-repo master
	
If you make local changes that should be shared with other repository, you
can go the opposite direction with:

    git subtree push --prefix swig_rules swig-rules-repo master
	
We can look at differences with upstream:

    git fetch swig-rules-repo
	git diff HEAD:swig_rules swig-rules-repo/master

The initial creation of the subtree in a new repository is by:

    git subtree add --prefix swig_rules swig-rules-repo master

Sometimes you will get errors saying "Updates were rejected because the
tip of current branch is behind." First do a subtree pull if you haven't,
this may be real. But you might get this even if a pull says everything
is up to date. This appears to be a bug in subtree, see description at
http://stackoverflow.com/questions/13756055/git-subtree-subtree-up-to-date-but-cant-push

Can try 

    git push swig-rules-repo `git subtree split --prefix swig_rules master`:master --force


Classes needed in code that uses this
-------------------------------------
All C++ classes exposed to python should derive (even indirectly, and usually
virtually) from a class `GenericObject` with `std::enable_shared_from_this`, in
the namespace given as `SWIG_MAPPER_NAMESPACE`. `lib/generic_object.h.sample`
and `lib/printable.h.sample` are examples (the specific code isn't required).
`Printable` maps the C++ `operator<<` to `print_to_string`, which the bindings
use for `__str__`.

`lib/serialize_function.cc` and `lib/weak_ptr_serialize_support.cc` are
compiled into the C++ library, with `-DSWIG_MAPPER_NAMESPACE=<namespace>`.
Serialization (boost) gives pickling; `lib/serialize_support.h.sample` is
the pattern for the library's own support header. `lib/DirectorNotes.md`
describes python subclasses of C++ classes (ownership, `weak_ptr`,
serialization, threads).

The python package files (`__init__.py`, `all.py`, `_nanobind_support.py`,
`py.typed`) are installed next to the extension module.

Build
-----
Autotools: `swig_rules.am` (C++ support code) and `nanobind/nanobind.am`
(the extension module), with `config/m4/ac_nanobind.m4` in configure. See
the comment at the top of `nanobind.am` for the variables to set.
