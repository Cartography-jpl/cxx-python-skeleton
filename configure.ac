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
# Option to bypass running SWIG to update SWIG wrappers. The SWIG
# wrappers can take a bit of time to create and compile, so if you
# aren't working directly with them it can be useful to turn off during
# development.
AC_ARG_WITH([swig],
           [AS_HELP_STRING([--without-swig],
             [Bypass running SWIG to update SWIG wrappers. The SWIG wrappers can take a bit of time to create and compile, so if you aren't working directly with them it can be useful to turn off during development.])],
           [SWIG=/fake_so_fail],
           [])

have_swig=no
AX_PKG_SWIG([], [have_swig=yes], [AC_MSG_WARN([cannot find 'swig' program. You should look at http://www.swig.org])])
AM_CONDITIONAL([HAVE_SWIG], [test "$have_swig" = "yes"])
AX_SWIG_ENABLE_CXX
AX_SWIG_PYTHON
# swig_rules.am uses the older name for this variable
AC_SUBST([SWIG_PYTHON_OPT], [$AX_SWIG_PYTHON_OPT])

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

if test "$have_swig" = "yes" -a "$USE_MAINTAINER_MODE" != "no"; then
  generate_swig="yes"
else
  generate_swig="no" 
fi

if test "$generate_swig" = "yes" -a "$have_doxygen" != "yes"; then
     AC_MSG_ERROR([
Skeleton requires doxygen if you are also generating SWIG python bindings.
You can try specifying the location using --with-doxygen if configure
couldn't find this. Or you can turn off the use of swig using 
--without-swig.])
fi

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
LOC_MSG([  Generate SWIG wrappers:        ${generate_swig}])
LOC_MSG([  Install documentation:         ${with_documentation}])


