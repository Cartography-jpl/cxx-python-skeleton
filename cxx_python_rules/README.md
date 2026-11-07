This contains generic rules for wrapping C++ code for python.

These were SWIG rules (`swig_rules`, repository `swig-rules`) until phase S6 of the modernization
(`claude_notes/implementation_plan.md` in cxx-python-skeleton), and were renamed in 2026-11. The directory holds the
nanobind rules (`nanobind/`, see `nanobind/README.md`) and the C++ support code they use (`lib/`). The python package
with the extension module is `<package>_cxx`, the module `<package>_cxx._bindings`.

This is not standalone code, it is meant to be included in a larger piece
of code.

Look for example at the [cxx-python-skeleton](https://github.jpl.nasa.gov/Cartography/cxx-python-skeleton) repository for a skeleton of using
this. A more full example can be found at 
[geocal](https://github.jpl.nasa.gov/Cartography/geocal).

Getting the rules into a repository
-----------------------------------

Normally you use git subtree to put this cxx_python_rules. So assuming this has
been added as a remote cxx-python-rules-repo you would do something like:

    git subtree pull --prefix cxx_python_rules cxx-python-rules-repo master
	
If you make local changes that should be shared with other repository, you
can go the opposite direction with:

    git subtree push --prefix cxx_python_rules cxx-python-rules-repo master
	
We can look at differences with upstream:

    git fetch cxx-python-rules-repo
	git diff HEAD:cxx_python_rules cxx-python-rules-repo/master

The initial creation of the subtree in a new repository is by:

    git subtree add --prefix cxx_python_rules cxx-python-rules-repo master

Sometimes you will get errors saying "Updates were rejected because the
tip of current branch is behind." First do a subtree pull if you haven't,
this may be real. But you might get this even if a pull says everything
is up to date. This appears to be a bug in subtree, see description at
http://stackoverflow.com/questions/13756055/git-subtree-subtree-up-to-date-but-cant-push

Can try 

    git push cxx-python-rules-repo `git subtree split --prefix cxx_python_rules master`:master --force


Classes needed in code that uses this
-------------------------------------
All C++ classes exposed to python should derive (even indirectly, and usually
virtually) from a class `GenericObject` with `std::enable_shared_from_this`, in
the namespace given as `CXX_PYTHON_RULES_NAMESPACE`. `lib/generic_object.h.sample`
and `lib/printable.h.sample` are examples (the specific code isn't required).
`Printable` maps the C++ `operator<<` to `print_to_string`, which the bindings
use for `__str__`.

The project's configuration header (the one `generic_object.h` includes; in
the skeleton `skeleton_config.h`, from `configure.ac`) defines
`CXX_PYTHON_RULES_NAMESPACE` (e.g. `#define CXX_PYTHON_RULES_NAMESPACE GeoCal`)
and, if Boost.Serialization is used, `CXX_PYTHON_RULES_HAVE_BOOST_SERIALIZATION`.
Every header here reaches it through `generic_object.h`, so code using the
installed headers needs no `-D` flags. (They used to be command-line flags;
a header used without them silently put its code in a namespace literally
called `CXX_PYTHON_RULES_NAMESPACE`. `cxx_python_rules_mdspan.h` now stops
with an `#error` instead.)

`lib/serialize_function.cc`, `lib/weak_ptr_serialize_support.cc` and
`lib/blitz_mdspan.cc` are compiled into the C++ library.
`lib/cxx_python_rules_mdspan.h` makes mdspan (the Kokkos implementation)
available as `CXX_PYTHON_RULES_NAMESPACE::md`, and `lib/blitz_mdspan.h` has
views between `blitz::Array` and mdspan, the C++ half of the numpy bridge
in `nanobind/blitz_array.h`.
Serialization (boost) gives pickling; `lib/serialize_support.h.sample` is
the pattern for the library's own support header.

Relative file names inside an archive (an object that opens a file in its
`load()`) are relative to the archive's directory: reading an archive from
a file sets a per-thread *archive base directory* (`serialize_function.h`),
and such an object opens `resolve_archive_path(name)`. Python pickles made
with `pickle_serialization_dir` load with the directory they were made in,
and `archive_base_directory_scope(dir)` (python) or `ArchiveBaseDirectory`
(C++) sets it for a block of code, e.g. reading a shelve. This replaced
changing the process's current directory during a load, which broke
relative file access on every other thread. `lib/DirectorNotes.md`
describes python subclasses of C++ classes (ownership, `weak_ptr`,
serialization, threads).

The python package files (`__init__.py`, `all.py`, `_nanobind_support.py`,
`py.typed`) are installed next to the extension module.

Build
-----
Autotools: `cxx_python_rules.am` (C++ support code) and `nanobind/nanobind.am`
(the extension module), with `config/m4/ac_nanobind.m4` in configure. See
the comment at the top of `nanobind.am` for the variables to set.
