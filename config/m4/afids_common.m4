#=================================================================
# A few things that are common to all our configure files.

AC_DEFUN([AFIDS_COMMON],[
AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT])
AC_REQUIRE([AC_PROG_CC])
# For some bizarre reason, this doesn't fail if there isn't a C++ compiler.
# This seems to be a bug, which had some discussion on the forums a while back
# (see http://lists.gnu.org/archive/html/bug-autoconf/2010-05/msg00001.html),
# but apparently hasn't been fixed. We work around this by checking if
# the CXX program is actually on the system.
AC_REQUIRE([AC_PROG_CXX])
# First check for CXX directly, in case the file path was given
if test -f "$CXX" && test -x "$CXX"; then
    HAVE_CXX=yes
else
   # Then check on path
   AC_CHECK_PROG(HAVE_CXX, $CXX, yes, no)
fi
if test "$HAVE_CXX" = "no"; then
   AC_MSG_ERROR([Could not find a c++ compiler]);
fi

# We check for the very basic programs m4, perl and patch. Surprising if not
# found, but possible, so we check.
AC_CHECK_PROG(HAVE_PERL, perl, yes, no)
if test "$HAVE_PERL" = "no"; then
   AC_MSG_ERROR([Could not find perl, which is required for install]);
fi
AC_CHECK_PROG(HAVE_M4, m4, yes, no)
if test "$HAVE_M4" = "no"; then
   AC_MSG_ERROR([Could not find m4, which is required for install]);
fi
AC_CHECK_PROG(HAVE_PATCH, patch, yes, no)
if test "$HAVE_PATCH" = "no"; then
   AC_MSG_ERROR([Could not find patch, which is required for install]);
fi
# Make sure we have perl module Data::Dumper which is used by some
# installed programs. This is actually a separate install on centos 7,
# so make sure it is there.
AX_PROG_PERL_MODULES([Data::Dumper], ,
   [AC_MSG_ERROR([Could not find perl module Data::Dumper, which is required for install])])


# We need to have csh to run things like vicarb
AC_CHECK_PROG(HAVE_CSH, csh, yes, no)
if test "$HAVE_CSH" = "no"; then
   AC_MSG_ERROR([Could not find csh, which is required for programs such as vicar]);
fi

# We use a few GNU make specific things, so make sure we have gnu make
AX_CHECK_GNU_MAKE()
if test "$_cv_gnu_make_command" = "" ; then
   AC_MSG_ERROR([Could not find a gnu version of make]);
fi

AC_COPYRIGHT(
[Copyright 2017, California Institute of Technology. 
ALL RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.])
# The obscure looking tar-pax here sets automake to allow file names longer
# than 99 characters to be included in the dist tar. See
# http://noisebleed.blogetery.com/2010/02/27/tar-file-name-is-too-long-max-99/#howtofixit
AM_INIT_AUTOMAKE([tar-pax])
AM_MAINTAINER_MODE
LT_INIT
AC_PROG_LN_S
AC_COPY_DIR

#AC_PREFIX_DEFAULT([`pwd`/install])

AX_CODE_COVERAGE()
AC_ENABLE_DEBUG

#=================================================================
# We are far enough along in time that we should be able to just
# require a C++17 compiler. We can perhaps relax that is needed to
# support older versions, but as of now (2024) this standard is already
# 7 years old. It is probably too soon to require C++20 right now,
# we can revisit that as needed (and perhaps just have this code
# conditional for now).
#=================================================================

AX_CXX_COMPILE_STDCXX([17], [ext], [mandatory])

#=================================================================
# C++ Threading requires pthread sometimes
#=================================================================

AX_PTHREAD()
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"

#=================================================================
# Test if we are using GCC compiler. Some flags get set in the 
# Makefile that should only be set for GCC.
#=================================================================

AM_CONDITIONAL([HAVE_GCC], [test "$GCC" = yes])

#=================================================================
# Add prefix, THIRDPARTY, and /opt/afids_support for pkgconfig file
#=================================================================

PKG_PROG_PKG_CONFIG

if test "x$THIRDPARTY" = x; then
  pkg_extra_path=${prefix}/lib/pkgconfig:/opt/afids_support/lib/pkgconfig
  geocal_support_path=${prefix}
else
  pkg_extra_path=${prefix}/lib/pkgconfig:$THIRDPARTY/lib/pkgconfig:/opt/afids_support/lib/pkgconfig
  geocal_support_path=$THIRDPARTY
fi
if test "x$CONDA_PREFIX" != x; then
   pkg_extra_path=$CONDA_PREFIX/lib/pkgconfig:${pkg_extra_path}
fi
if test "x$PKG_CONFIG_PATH" = x; then
  PKG_CONFIG_PATH=$pkg_extra_path
else
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$pkg_extra_path
fi
export PKG_CONFIG_PATH

AC_SUBST([pkgconfigdir], [${libdir}/pkgconfig])
AC_SUBST([geocalsupportdir], [$geocal_support_path])

])

