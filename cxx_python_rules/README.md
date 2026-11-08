This contains generic rules for wrapping C++ code for python.

These were SWIG rules (`swig_rules`, repository `swig-rules`) until the bindings moved to nanobind,
and were renamed in 2026-11. The directory holds the
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

`lib/serialize_function.cc`, `lib/weak_ptr_serialize_support.cc`,
`lib/director_hook.cc` and `lib/blitz_mdspan.cc` are compiled into the C++
library.
`lib/cxx_python_rules_mdspan.h` makes mdspan (the Kokkos implementation)
available as `CXX_PYTHON_RULES_NAMESPACE::md`, and `lib/blitz_mdspan.h` has
views between `blitz::Array` and mdspan, the C++ half of the numpy bridge
in `nanobind/blitz_array.h`.
Serialization (boost) gives pickling; `lib/serialize_support.h.sample` is
the pattern for the library's own support header.

The `.sample` files (here and in `python/`) are written from the skeleton's
own files by `script/samples.py --write`, with the names replaced (`Skeleton`
becomes `GeoCal` and so on). In the skeleton, edit the file in `lib/` or
`python/`, not the sample; `script/samples.py --check` (run by
`script/check_all.sh`) finds a sample that has drifted. Lines only a sample
has are kept between `// sample only:` and `// end sample only`.

Relative file names inside an archive (an object that opens a file in its
`load()`) are relative to the archive's directory: reading an archive from
a file sets a per-thread *archive base directory* (`serialize_function.h`),
and such an object opens `resolve_archive_path(name)`. Python pickles made
with `pickle_serialization_dir` load with the directory they were made in,
and `archive_base_directory_scope(dir)` (python) or `ArchiveBaseDirectory`
(C++) sets it for a block of code. This replaced changing the process's
current directory during a load in the C++ readers, which broke relative
file access on every other thread. `read_shelve` (`swig_shelve.py`) still
changes to the shelve's directory while it reads as well, so code that
opens files by relative name without the resolver (a python class's
`__setstate__`, a C++ class not converted yet) keeps working; don't read
shelves while other threads use relative file names. `lib/DirectorNotes.md`
describes python subclasses of C++ classes (ownership, `weak_ptr`,
serialization, threads).

The python package files (`__init__.py`, `all.py`, `_nanobind_support.py`,
`py.typed`) are installed next to the extension module.

Archives are trusted input
--------------------------
Archives and shelves can hold pickles (python subclass objects, python
objects in a shelve), and Boost.Serialization doesn't validate what it
reads, so loading one can run arbitrary code. Load only files you trust.

Threads
-------
What can be used from several threads at once:
- `serialize_write*` and `serialize_read*`: yes, on different threads. Each
  write has its own record of the objects in it for the `weak_ptr` rule
  (`weak_ptr_serialize_support.h`), per thread, and the archive base
  directory is per thread.
- The skeleton's `Exception`: copies can be used on any thread (copying
  can't throw; copies share the data, copy on write).
- Blitz arrays: copies share their memory with a reference count, which
  is atomic only if Blitz was built with `--enable-threadsafe`. Configure
  reports it ("Blitz thread safe" in its summary) and warns if not; then
  classes that copy Blitz arrays can't be used from several threads at
  once.
- The `Observer`/`Observable` classes (in the skeleton's `lib/`): no.
  Register, notify and destroy observers on one thread, and don't destroy
  an observer while it is being notified.
- The binding code in `nanobind/`: it relies on the GIL. The python
  modules it keeps (`<package>._nanobind_support` and numpy) are imported
  when the extension module is imported (`init_statics`), under the
  import lock, not at first use.

Free-threaded python (no GIL) is a goal for later, not supported now. What
is left for it:
- Build with nanobind's free-threaded support (CMake `FREE_THREADED`, and
  the matching setting in `nanobind.am`).
- Lock the type registry (`nanobind/type_registry.h`) while modules
  register their types at import; its one-time setup is already safe.
- `Observable::notify_update_do` is const but removes expired entries from
  its lists. Moving that to add/remove changes the bytes of archives
  rewritten after loading (they hold empty entries for observers outside
  the archive), so it was left; a mutex around the lists covers it.
- A mutex around `Observable`'s lists, and a decision on observers
  destroyed on another thread while being notified (the `weak_ptr` to an
  observer doesn't keep it alive).
- Classes with shared scratch space in the skeleton (`CalcRasterSample`,
  `BufferRasterSample`'s copy counter).
- Run the python tests on a free-threaded python (conda-forge
  `python-freethreading`).

Build
-----
Autotools: `cxx_python_rules.am` (C++ support code) and `nanobind/nanobind.am`
(the extension module), with `config/m4/ac_nanobind.m4` in configure. See
the comment at the top of `nanobind.am` for the variables to set.
