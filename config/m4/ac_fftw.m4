# SYNOPSIS
#
#   AC_FFTW([required])
#
# DESCRIPTION
#
# This looks for the fftw library. If we find them, we set the Makefile
# conditional HAVE_FFTW. We as set FFTW_CFLAGS and FFTW_LIBS
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.

AC_DEFUN([AC_FFTW],
[
# Guard against running twice
if test "x$done_fftw" = "x"; then
AC_HANDLE_WITH_ARG([fftw], [fftw], [fftw], $1)
if test "x$want_fftw" = "xyes"; then
        AC_MSG_CHECKING([for Fftw library])
        succeeded=no
        if test "$ac_fftw_path" != ""; then
            FFTW_LIBS="-L$ac_fftw_path/lib -lfftw3"
            FFTW_CFLAGS="-I$ac_fftw_path/include"
            succeeded=yes
        else
	    AC_SEARCH_LIB([FFTW], [fftw3], , [fftw3.h], , [libfftw3], [-lfftw3])
        fi

        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
        else
                AC_MSG_RESULT([yes])
                AC_SUBST(FFTW_CFLAGS)
                AC_SUBST(FFTW_LIBS)
                AC_DEFINE(HAVE_FFTW,,[Defined if we have FFTW])
                have_fftw="yes"
        fi
fi
AM_CONDITIONAL([HAVE_FFTW], [test "$have_fftw" = "yes"])

AC_CHECK_FOUND([fftw], [fftw],[fftw],$1)

done_fftw="yes"
fi
])
