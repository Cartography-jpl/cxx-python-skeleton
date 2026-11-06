# SYNOPSIS
#
#   AC_NANOBIND([required])
#
# DESCRIPTION
#
#   Find nanobind (https://nanobind.readthedocs.io) through the configured
#   python (run AFIDS_PYTHON or AC_PYTHON_DEVEL first). nanobind has no
#   library to link to; an extension module compiles its source
#   (nb_combined.cpp) once, see cxx_python_rules/nanobind/nanobind.am. Sets
#
#     NANOBIND_CPPFLAGS  -isystem for nanobind's include directory and
#                        its bundled robin_map
#     NANOBIND_SRCDIR    directory with nb_combined.cpp
#     NANOBIND_VERSION
#
#   and the conditional HAVE_NANOBIND. With "required", it is an error if
#   nanobind can't be imported.

AC_DEFUN([AC_NANOBIND],
[
AC_MSG_CHECKING([for nanobind])
have_nanobind=no
NANOBIND_CPPFLAGS=""
NANOBIND_SRCDIR=""
NANOBIND_VERSION=""
if test -n "$PYTHON"; then
  nb_inc=`$PYTHON -c "import nanobind; print(nanobind.include_dir())" 2>/dev/null`
  nb_src=`$PYTHON -c "import nanobind; print(nanobind.source_dir())" 2>/dev/null`
  NANOBIND_VERSION=`$PYTHON -c "import nanobind; print(nanobind.__version__)" 2>/dev/null`
  if test -n "$nb_inc" -a -f "$nb_src/nb_combined.cpp"; then
    have_nanobind=yes
    NANOBIND_CPPFLAGS="-isystem $nb_inc -isystem $nb_src/../ext/robin_map/include"
    NANOBIND_SRCDIR="$nb_src"
  fi
fi
AC_MSG_RESULT([$have_nanobind $NANOBIND_VERSION])
if test "$have_nanobind" != "yes" -a "$1" = "required"; then
  AC_MSG_ERROR([nanobind is required (pip or conda install nanobind for $PYTHON)])
fi
AC_SUBST([NANOBIND_CPPFLAGS])
AC_SUBST([NANOBIND_SRCDIR])
AC_SUBST([NANOBIND_VERSION])
AM_CONDITIONAL([HAVE_NANOBIND], [test "$have_nanobind" = "yes"])
])
