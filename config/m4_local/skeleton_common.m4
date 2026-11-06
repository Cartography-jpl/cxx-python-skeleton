#=================================================================
# A few things that are common to all our configure files.

AC_DEFUN([SKELETON_COMMON],[
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

# We need to have csh to run things like vicarb
#AC_CHECK_PROG(HAVE_CSH, csh, yes, no)
#if test "$HAVE_CSH" = "no"; then
#   AC_MSG_ERROR([Could not find csh, which is required for programs such as vicar]);
#fi

# We use a few GNU make specific things, so make sure we have gnu make
AX_CHECK_GNU_MAKE()
if test "$_cv_gnu_make_command" = "" ; then
   AC_MSG_ERROR([Could not find a gnu version of make]);
fi

AC_COPYRIGHT(
[Copyright 2019, California Institute of Technology. 
ALL RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.])
# The obscure looking tar-pax here sets automake to allow file names longer
# than 99 characters to be included in the dist tar. See
# http://noisebleed.blogetery.com/2010/02/27/tar-file-name-is-too-long-max-99/#howtofixit
AM_INIT_AUTOMAKE([tar-pax])
AM_MAINTAINER_MODE
LT_INIT
# Don't need these yet
AC_PROG_CXX
# AC_PROG_LN_S
# AC_COPY_DIR

AC_PREFIX_DEFAULT([`pwd`/install])
AC_PROG_CC

AM_PROG_CC_C_O
AC_ENABLE_DEBUG
# Warnings for debug builds (the development builds). conda's
# DEBUG_CXXFLAGS already has -Wall -Wextra; this covers the other cases
# and adds -Wdeprecated. Third party headers are -isystem, so their
# warnings don't show.
if test "x$enable_debug" = "xyes" && test "x$GXX" = "xyes"; then
  CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wdeprecated"
fi

#=================================================================
# addr2line (binutils), used by Exception::stacktrace() to give
# function names and file:line (Boost.Stacktrace's addr2line backend).
# Boost runs it by absolute path, given as a macro that it turns back
# into a string, so only accept a plain path. Without it the stack
# trace has only addresses, plus names of exported symbols.
#=================================================================

AC_PATH_PROG([ADDR2LINE], [addr2line])
case "$ADDR2LINE" in
  "") ;;
  *[[!A-Za-z0-9/._-]]*)
    AC_MSG_WARN([addr2line path $ADDR2LINE has unusual characters, not using it for stack traces]) ;;
  *)
    AC_DEFINE_UNQUOTED([SKELETON_ADDR2LINE], [$ADDR2LINE],
                       [Absolute path of addr2line, used for stack traces]) ;;
esac

#=================================================================
# Require C++20 (gcc >= 14, the minimum compiler tested; see
# claude_notes/implementation_plan.md phase S1). Use -std=c++20, not
# gnu++20: nothing here needs the GNU extensions, and the strict mode
# catches nonportable code. nanobind needs at least C++17.
#=================================================================

AX_CXX_COMPILE_STDCXX([20], [noext], [mandatory])

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

PKG_PROG_PKG_CONFIG

if test "x$THIRDPARTY" = x ; then
  pkg_extra_path=${prefix}/lib/pkgconfig:/opt/afids_support/lib/pkgconfig
else
  pkg_extra_path=${prefix}/lib/pkgconfig:$THIRDPARTY/lib/pkgconfig:/opt/afids_support/lib/pkgconfig
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

])
