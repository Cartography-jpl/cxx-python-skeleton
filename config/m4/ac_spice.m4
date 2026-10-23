# SYNOPSIS
#
#   AC_SPICE([required])
#
# DESCRIPTION
#
# This looks for the SPICE library. If we find it, then we set the
# Makefile conditional HAVE_SPICE. We also set SPICE_CFLAGS and
# SPICE_LIBS
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.

AC_DEFUN([AC_SPICE],
[
# Guard against running twice
if test "x$done_spice" = "x"; then
AC_HANDLE_WITH_ARG([spice], [spice], [SPICE], $1)

if test "x$want_spice" = "xyes"; then
        AC_MSG_CHECKING([for SPICE library])
        succeeded=no
        if test "$ac_spice_path" != ""; then
            SPICE_LIBS="-R$ac_spice_path/lib -L$ac_spice_path/lib -lcspice"
            SPICE_CFLAGS="-I$ac_spice_path/include"
            succeeded=yes
        else
  	    if test "x$CONDA_PREFIX" != x; then
	       if test -e "$CONDA_PREFIX/include/SpiceCK.h"; then
                  SPICE_LIBS="-R$CONDA_PREFIX/lib -L$CONDA_PREFIX/lib -lcspice"
                  SPICE_CFLAGS="-I$CONDA_PREFIX/include"
                  succeeded=yes
	       elif test -e "$prefix/include/SpiceCK.h"; then
                  SPICE_LIBS="-R$prefix/lib -L$prefix/lib -lcspice"
                  SPICE_CFLAGS="-I$prefix/include"
                  succeeded=yes
	       fi
            fi
            if test "$succeeded" != "yes" ; then
	        AC_SEARCH_LIB([SPICE], [spice], , [SpiceCK.h], ,
                            [libcspice], [-lcspice])
            fi
        fi

        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
        else
                AC_MSG_RESULT([yes])
                AC_SUBST(SPICE_CFLAGS)
                AC_SUBST(SPICE_LIBS)
                have_spice="yes"
                AC_DEFINE(HAVE_SPICE,,[Defined if we have SPICE])
        fi
fi
AM_CONDITIONAL([HAVE_SPICE], [test "$have_spice" = "yes"])

AC_CHECK_FOUND([spice], [spice],[SPICE],$1)
done_spice="yes"
fi
])
