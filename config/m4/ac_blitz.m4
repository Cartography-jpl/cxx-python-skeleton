# SYNOPSIS
#
#   AC_BLITZ([required])
#
# DESCRIPTION
#
# This looks for the Blitz++ library. If we find them, we set the Makefile
# conditional HAVE_BLITZ. We as set BLITZ_CFLAGS and BLITZ_LIBS
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.

AC_DEFUN([AC_BLITZ],
[
# Guard against running twice
if test "x$done_blitz" = "x"; then
AC_HANDLE_WITH_ARG([blitz], [blitz], [Blitz++], $1)

if test "x$want_blitz" = "xyes"; then
        AC_MSG_CHECKING([for Blitz library])
        succeeded=no
        if test "$ac_blitz_path" != ""; then
            BLITZ_LIBS="-L$ac_blitz_path/lib -lblitz"
            BLITZ_CFLAGS="-I$ac_blitz_path/include"
            succeeded=yes
        else
	    AC_SEARCH_LIB([BLITZ], [blitz], [blitz/], [blitz.h], ,
                          [libblitz], [-lblitz])
        fi
        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
        else
                AC_MSG_RESULT([yes])
                AC_SUBST(BLITZ_CFLAGS)
                AC_SUBST(BLITZ_LIBS)
                AC_DEFINE(HAVE_BLITZ,,[Defined if we have Blitz])
                have_blitz="yes"
        fi
fi
AM_CONDITIONAL([HAVE_BLITZ], [test "$have_blitz" = "yes"])

AC_CHECK_FOUND([blitz], [blitz],[Blitz++],$1)

done_blitz="yes"
fi
])
