# SYNOPSIS
#
#   AC_CARTO([required])
#
# DESCRIPTION
#
# This looks for the Carto library. If we find it, then we set the
# Makefile conditional HAVE_CARTO. We also set CARTO_CFLAGS and
# CARTO_LIBS
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.

AC_DEFUN([AC_CARTO],
[
# Guard against running twice
if test "x$done_carto" = "x"; then
AC_HANDLE_WITH_ARG([carto], [carto], [Carto], $1)

if test "x$want_carto" = "xyes"; then
        AC_MSG_CHECKING([for Carto library])
        succeeded=no
        if test "$ac_carto_path" != ""; then
            CARTO_LIBS="-R$ac_carto_path/lib -L$ac_carto_path/lib -lcarto \$(VICAR_RTL_LIBS) \$(GSL_LIBS)"
            CARTO_CFLAGS="-I$ac_carto_path/include/carto -I$ac_carto_path/include \$(VICAR_RTL_CFLAGS) \$(GSL_CFLAGS)"
            succeeded=yes
        else
	    AC_SEARCH_LIB([CARTO], [carto], [carto/], [burl.h], ,
                          [libcarto], [-lcarto])
	    CARTO_LIBS="$CARTO_LIBS \$(VICAR_RTL_LIBS) \$(GSL_LIBS)"
        fi

# We can only use the carto library if we also have vicar and gsl
        if test "$have_gsl" != "yes"; then
	   succeeded=no
	fi
        if test "$have_vicar_rtl" != "yes"; then
	   succeeded=no
	fi

        if test "$succeeded" != "yes" ; then
                CARTO_CFLAGS=""
		CARTO_LIBS=""
                AC_MSG_RESULT([no])
		have_carto="no"
        else
                AC_MSG_RESULT([yes])
                AC_SUBST(CARTO_CFLAGS)
                AC_SUBST(CARTO_LIBS)
                AC_DEFINE(HAVE_CARTO,,[Defined if we have Carto library])
                have_carto="yes"
        fi
fi
AM_CONDITIONAL([HAVE_CARTO], [test "$have_carto" = "yes"])

AC_CHECK_FOUND([carto], [carto],[Carto],$1)
done_carto="yes"
fi
])
