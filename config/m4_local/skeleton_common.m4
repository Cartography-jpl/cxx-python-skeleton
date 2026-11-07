#=================================================================
# A few things that are common to all our configure files.

AC_DEFUN([SKELETON_COMMON],[
AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT])
# Hoisted: AM_INIT_AUTOMAKE below expands it, and LT_INIT requires it
# (else autoconf warns "AC_PROG_AWK was expanded before it was required").
AC_REQUIRE([AC_PROG_AWK])
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
# Require C++23 (gcc >= 14, the minimum compiler tested). Use -std=c++23,
# not gnu++23: nothing here needs the GNU extensions, and the strict mode
# catches nonportable code. nanobind needs at least C++17. Library
# features newer than gcc 14 (std::mdspan, gcc 16; range formatting,
# gcc 15) aren't used.
#=================================================================

AX_CXX_COMPILE_STDCXX([23], [noext], [mandatory])

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
# Warnings for the project's own code (AM_CXXFLAGS in common.am: the
# library, its tests and the python bindings; nanobind's own source has
# its own flags in nanobind.am, and third-party headers are -isystem).
# Each flag is only used if the compiler takes it. The code is clean with
# all of them (gcc 14, gcc 15, clang 22).
# --enable-werror makes warnings errors; it is off by default, so a new
# compiler's new warnings don't break builds for users.
#=================================================================

AC_LANG_PUSH([C++])
SKELETON_WARN_CXXFLAGS=""
# -Wduplicated-cond, -Wduplicated-branches and -Wlogical-op are gcc only
# (the check below drops them for clang).
# -Wextra-semi is left out: the X(...); style of using the Boost
# serialization macros (SKELETON_EXPORT_KEY(Foo);) gives a warning per use.
# -Wnull-dereference is left out: in optimized builds gcc reports false
# positives inside libstdc++'s shared_ptr (weak_ptr::lock).
for skeleton_flag in -Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast \
    -Wsuggest-override -Wnon-virtual-dtor -Wconversion -Wsign-conversion \
    -Wduplicated-cond -Wduplicated-branches -Wlogical-op \
    -Wcast-qual -Wformat=2 -Wdeprecated-copy-dtor; do
  AX_CHECK_COMPILE_FLAG([$skeleton_flag],
    [SKELETON_WARN_CXXFLAGS="$SKELETON_WARN_CXXFLAGS $skeleton_flag"], [], [-Werror])
done
# For the test programs only: Boost.Test's macros use __COUNTER__, which
# clang (>= 22) reports under -Wpedantic as a C2y extension, once per test
# case. Only for compilers that have the warning.
SKELETON_TEST_CXXFLAGS=""
AX_CHECK_COMPILE_FLAG([-Wc2y-extensions],
  [SKELETON_TEST_CXXFLAGS="-Wno-c2y-extensions"], [], [-Werror])
AC_SUBST([SKELETON_TEST_CXXFLAGS])
AC_LANG_POP([C++])
AC_ARG_ENABLE([werror],
  [AS_HELP_STRING([--enable-werror], [treat compiler warnings as errors])],
  [], [enable_werror=no])
if test "x$enable_werror" = "xyes"; then
  SKELETON_WARN_CXXFLAGS="$SKELETON_WARN_CXXFLAGS -Werror"
fi
AC_SUBST([SKELETON_WARN_CXXFLAGS])

#=================================================================
# Sanitizers: --enable-sanitizers builds everything with AddressSanitizer
# and UndefinedBehaviorSanitizer, --enable-sanitizers=LIST with the ones
# in LIST (e.g. address, or undefined). The flags are added here, after
# AC_ENABLE_DEBUG has set CXXFLAGS, so a debug build keeps them. "make
# check" then sets the sanitizer options (common.am) and, for the python
# tests, preloads the sanitizer runtimes: libtool links shared libraries
# with -nostdlib, so libskeleton and the extension modules don't pull them
# in, and python itself isn't built with them.
# In a conda environment, configure in one with conda's compiler
# activation (pixi run, or the build-rpath recipe in CLAUDE.md): without
# it the sanitizer runtime can link against the host's libraries.
#=================================================================

AC_ARG_ENABLE([sanitizers],
  [AS_HELP_STRING([--enable-sanitizers@<:@=LIST@:>@],
    [build with the sanitizers in LIST, comma separated (default: address,undefined)])],
  [], [enable_sanitizers=no])
case "$enable_sanitizers" in
  no) skeleton_sanitizers= ;;
  yes) skeleton_sanitizers=address,undefined ;;
  *) skeleton_sanitizers=$enable_sanitizers ;;
esac
SKELETON_SANITIZER_PRELOAD=
if test -n "$skeleton_sanitizers"; then
  skeleton_san_flags="-fsanitize=$skeleton_sanitizers -fno-omit-frame-pointer"
  CXXFLAGS="$CXXFLAGS $skeleton_san_flags"
  CFLAGS="$CFLAGS $skeleton_san_flags"
  LDFLAGS="$LDFLAGS $skeleton_san_flags"
  AC_LANG_PUSH([C++])
  AC_MSG_CHECKING([whether $CXX builds with $skeleton_san_flags])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([], [])], [AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])
     AC_MSG_ERROR([$CXX can't build with $skeleton_san_flags])])
  AC_LANG_POP([C++])
  for skeleton_san in address:asan undefined:ubsan; do
    case ",$skeleton_sanitizers," in
      *,${skeleton_san%%:*},*)
        SKELETON_SANITIZER_PRELOAD="$SKELETON_SANITIZER_PRELOAD `$CXX -print-file-name=lib${skeleton_san##*:}.so`" ;;
    esac
  done
fi
AC_SUBST([SKELETON_SANITIZER_PRELOAD])
AM_CONDITIONAL([SKELETON_SANITIZERS], [test -n "$skeleton_sanitizers"])

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

#=================================================================
# The python bindings need python >= 3.12 (PyErr_GetRaisedException in
# cxx_python_rules/nanobind/cxx_python_rules_nb.h). The shared
# config/m4/afids_python.m4 only asks for 3.4, so check here; call after
# AFIDS_PYTHON.
#=================================================================

AC_DEFUN([SKELETON_PYTHON_MINIMUM],[
AC_MSG_CHECKING([whether $PYTHON is at least python 3.12])
if $PYTHON -c "import sys; sys.exit(0 if sys.version_info >= (3, 12) else 1)" 2>/dev/null; then
   AC_MSG_RESULT([yes])
else
   AC_MSG_RESULT([no])
   AC_MSG_ERROR([python 3.12 or later is required, $PYTHON is $($PYTHON --version 2>&1)])
fi
])
