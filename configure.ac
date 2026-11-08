dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([cxx-python-skeleton],[0.10])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIRS([config/m4 config/m4_local])
AC_CONFIG_FILES([script/setup_skeleton.csh script/setup_skeleton.sh])
# skeleton_build_config.h (autoheader) is private to the build;
# skeleton_config.h (from a hand-written template) is installed.
AC_CONFIG_HEADERS([lib/skeleton_build_config.h])
AC_CONFIG_FILES([lib/skeleton_config.h])
AC_CONFIG_FILES([lib/global_fixture_default.cc])

SKELETON_COMMON

# AC_ENABLE_DEBUG (in SKELETON_COMMON) defines BOOST_DISABLE_ASSERTS for a
# non-debug build in skeleton_build_config.h, which only skeleton_exception.cc
# includes. Pass it on the command line too, so all of the library is
# compiled with it, as when it was in the installed skeleton_config.h (it
# no longer reaches code that just uses the installed headers).
if test "x$enable_debug" != "xyes"; then
   SKELETON_RELEASE_CPPFLAGS=-DBOOST_DISABLE_ASSERTS=
fi
AC_SUBST([SKELETON_RELEASE_CPPFLAGS])

# A debug build also turns on libstdc++'s assertions (bounds checks in
# std::vector, std::array, std::string and other precondition checks). It
# doesn't change the ABI, so code built with and without it can be mixed.
if test "x$enable_debug" = "xyes"; then
   SKELETON_DEBUG_CPPFLAGS=-D_GLIBCXX_ASSERTIONS
fi
AC_SUBST([SKELETON_DEBUG_CPPFLAGS])

#=================================================================
# Find python and the python modules the build and tests need.

AFIDS_PYTHON(required)
SKELETON_PYTHON_MINIMUM

#=================================================================
# Various thirdparty libraries.
AC_DOXYGEN(optional)
AC_BOOST(required, 1.84.0)
AC_BLITZ(required)
SKELETON_BLITZ_THREADSAFE

#=================================================================
# Location of all the source code directories
SKELETON_SOURCE_DIRECTORY

#=================================================================
# Install the python code in editable mode (pip install -e) when doing
# "make install". Can also be selected for a single run with
# "make install PYTHON_EDITABLE_MODE=t".

AC_ARG_ENABLE([python-editable-mode],
	      [AS_HELP_STRING([--enable-python-editable-mode],
	      [install the python code in editable mode (pip install -e) in make install])],
	      [enable_python_editable_mode=$enableval],
	      [enable_python_editable_mode=no])
if test "$enable_python_editable_mode" = "yes"; then
   python_editable_mode_default=t
else
   python_editable_mode_default=
fi
AC_SUBST([python_editable_mode_default])

#=================================================================
# nanobind, for the python bindings.
AC_NANOBIND(optional)

#=================================================================
# Turn off use of boost serialization of desired. Don't have a use
# case for this yet, but it is easier to put in and not use than
# to not have it and find we need it.

AC_ARG_WITH([boost-serialization],
            [AS_HELP_STRING([--without-boost-serialization],
	      [Do not include code to support doing boost serialization with Skeleton objects])],
	      [with_boost_serialization=no],
              [with_boost_serialization=yes])
if test "$with_boost_serialization" = "yes"; then
    skeleton_no_serialization=
else
    skeleton_no_serialization="// "
fi
# Comments out the serialization macros in lib/skeleton_config.h.
AC_SUBST([skeleton_no_serialization])
AM_CONDITIONAL([HAVE_BOOST_SERIALIZATION], [test "$with_boost_serialization" = "yes"])

#=================================================================
# Option to bypass running doxygen to update documentation. This is useful
# for a build environment that is used for development, but you'll want all
# of your "real" build environments to also update the documentation.
AC_ARG_WITH([documentation],
           [AS_HELP_STRING([--without-documentation],
             [Bypass running doxygen to update documentation. This is useful for a build environment that is used for development, but you'll want all of your "real" build environments to also update the documentation.])],
           [],
           [with_documentation=yes])
if test "$have_doxygen" != "yes"; then
   with_documentation=no
fi
AM_CONDITIONAL([WITH_DOCUMENTATION], [test x$with_documentation = xyes])

#=================================================================
# The python bindings need nanobind and boost serialization (pickling).
# Without either, only the C++ library and its tests are built.
AC_ARG_WITH([python-bindings],
           [AS_HELP_STRING([--without-python-bindings],
             [Build only the C++ library and its tests, not the nanobind python extension module])],
           [],
           [with_python_bindings=yes])
build_python_bindings=no
if test "$with_python_bindings" != "no" -a "$have_nanobind" = "yes" -a "$with_boost_serialization" = "yes"; then
  build_python_bindings=yes
fi
AM_CONDITIONAL([BUILD_PYTHON_BINDINGS], [test "$build_python_bindings" = "yes"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

#======================================================================
# Give a nice log message describing what has been configured
#======================================================================

LOC_MSG()
LOC_MSG([Skeleton is now configured])
LOC_MSG()
LOC_MSG([  Installation directory:        ${prefix}])
LOC_MSG([  Build debug version:           ${enable_debug}])
LOC_MSG([  Blitz thread safe:             ${skeleton_blitz_threadsafe}])
LOC_MSG([  CXX compiler:                  ${CXX} ${CXXFLAGS}])
LOC_MSG()
LOC_MSG([  Build python bindings:         ${build_python_bindings} (nanobind ${NANOBIND_VERSION})])
LOC_MSG([  Install documentation:         ${with_documentation}])


