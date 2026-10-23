# SYNOPSIS
#
#   AC_GSL([required])
#
# DESCRIPTION
#
# This looks for the GSL libraries. If we find them, we set the Makefile
# conditional HAVE_GSL. We as set GSL_CFLAGS and GSL_LIBS
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.

AC_DEFUN([AC_GSL],
[
# Guard against running twice
if test "x$done_gsl" = "x"; then
AC_HANDLE_WITH_ARG([gsl], [gsl], [GSL], $1)
if test "x$want_gsl" = "xyes"; then
        AC_MSG_CHECKING([for GSL library])
        succeeded=no
        if test "$ac_gsl_path" != ""; then
            GSL_LIBS="-L$ac_gsl_path/lib -lgsl -lgslcblas"
            GSL_CFLAGS="-I$ac_gsl_path/include"
            succeeded=yes
        else
	    AC_SEARCH_LIB([GSL], [gsl], , [gsl/gsl_blas.h], ,
                          [libgsl], [-lgsl -lgslcblas])
        fi

        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
        else
                AC_MSG_RESULT([yes])
                AC_SUBST(GSL_CFLAGS)
                AC_SUBST(GSL_LIBS)
                AC_DEFINE(HAVE_GSL,,[Defined if we have GSL])
                have_gsl="yes"
        fi
fi
AM_CONDITIONAL([HAVE_GSL], [test "$have_gsl" = "yes"])

AC_CHECK_FOUND([gsl], [gsl],[GSL],$1)

done_gsl="yes"
fi
])
