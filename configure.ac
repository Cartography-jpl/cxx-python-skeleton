dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([swig-rules-skeleton],[0.10])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIRS([config/m4 config/m4_local])
AC_CONFIG_FILES([script/setup_skeleton.csh script/setup_skeleton.sh])
AC_CONFIG_HEADERS([lib/skeleton_config.h])
AC_CONFIG_FILES([lib/global_fixture_default.cc])

SKELETON_COMMON

#=================================================================
# Find location of afids and geocal
# (note for skeleton we don't depend on AFIDS, but leave this name in
# place in case we need to add this back in).

#AFIDS_LOCATION

#=================================================================
# Find python. Note that the virtual environment has modules that
# depend on the afids build, so set LD_LIBRARY_PATH here for AFIDS
# before checking all the modules.

original_ld_library_path="${LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH="${afidsdir}/lib64:${afidsdir}/lib:${LD_LIBRARY_PATH}"
AFIDS_PYTHON(required)
LD_LIBRARY_PATH="${original_ld_library_path}"

#=================================================================
# Various thirdparty libraries.
AC_DOXYGEN(optional)
AC_BOOST(required, 1.46.0)
AC_BLITZ(required)

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
# nanobind, for the python bindings (they replaced SWIG in phase S6 of
# claude_notes/implementation_plan.md).
AC_NANOBIND(optional)

#=================================================================
# Turn off use of boost serialization of desired. Don't have a use
# case for this yet, but it is easier to put in and not use than
# to not have it and find we need it.

AC_ARG_WITH([boost-serialization],
            [AS_HELP_STRING([--without-boost-serialization],
	      [Do not include code to support doing boost serialization with GeoCal objects])],
	      [with_boost_serialization=no],
              [with_boost_serialization=yes])
if test "$with_boost_serialization" = "yes"; then
    AC_DEFINE(SKELETON_HAVE_BOOST_SERIALIZATION,,[Defined if we have boost serialization])
fi
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
LOC_MSG([  CXX compiler:                  ${CXX} ${CXXFLAGS}])
LOC_MSG()
LOC_MSG([  Build python bindings:         ${build_python_bindings} (nanobind ${NANOBIND_VERSION})])
LOC_MSG([  Install documentation:         ${with_documentation}])


