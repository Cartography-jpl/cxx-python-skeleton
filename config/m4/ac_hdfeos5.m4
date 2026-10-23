# SYNOPSIS
#
#   AC_HDFEOS5([required])
#
# DESCRIPTION
#
# This looks for the Hdfeos5 library. If we find it, then we set the
# Makefile conditional HAVE_HDFEOS5. We also set HDFEOS5_CFLAGS and
# HDFEOS5_LIBS
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.

AC_DEFUN([AC_HDFEOS5],
[
# Guard against running twice
if test "x$done_hdfeos5" = "x"; then
AC_HANDLE_WITH_ARG([hdfeos5], [hdfeos5], [Hdfeos5 library], $1)
if test "x$want_hdfeos5" = "xyes"; then
        AC_HDF5($1)
        AC_MSG_CHECKING([for Hdfeos5 library])
        succeeded=no
        if test "$ac_hdfeos5_path" != ""; then
            HDFEOS5_LIBS="-R$ac_hdfeos5_path/lib -L$ac_hdfeos5_path/lib -lhe5_hdfeos -lGctp"
            HDFEOS5_CFLAGS="-I$ac_hdfeos5_path/include/hdfeos5"
            succeeded=yes
        else
	    # Special case, conda doesn't have a pkgconfig for hdfeos5, but
	    # we want to pick this before anything else. So start by
	    # searching for that.
            if test "x$CONDA_PREFIX" != x; then
	       if test -e "$CONDA_PREFIX/include/HE5_HdfEosDef.h"; then
	           HDFEOS5_PREFIX="$CONDA_PREFIX"
		   HDFEOS5_CFLAGS="-I$CONDA_PREFIX/include"
		   HDFEOS5_LIBS="-R$CONDA_PREFIX/lib -L$CONDA_PREFIX/lib -lhe5_hdfeos -lGctp"
		   succeeded=yes
	       fi
	    fi
            if test "$succeeded" != "yes" ; then
	       AC_SEARCH_LIB([HDFEOS5], [hdfeos5], [hdfeos5/], [HE5_HdfEosDef.h], ,
                          [libhe5_hdfeos], [-lhe5_hdfeos -lGctp])
	    fi  
            if test "$succeeded" != "yes" ; then
 	       AC_SEARCH_LIB([HDFEOS5], [hdfeos5], , [HE5_HdfEosDef.h], ,
                             [libhe5_hdfeos], [-lhe5_hdfeos -lGctp])
            fi
        fi
	if test "$have_hdf5" == "no"; then
            succeeded=no
        fi

        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
                HDFEOS5_CFLAGS=""
		HDFEOS5_LIBS=""
		have_hdfeos5="no"
        else
                HDFEOS5_CFLAGS="$HDFEOS5_CFLAGS $HDF5_CFLAGS"
		HDFEOS5_LIBS="$HDFEOS5_LIBS $HDF5_LIBS"
                AC_MSG_RESULT([yes])
                AC_SUBST(HDFEOS5_CFLAGS)
                AC_SUBST(HDFEOS5_LIBS)
                have_hdfeos5="yes"
        fi
fi
AM_CONDITIONAL([HAVE_HDFEOS5], [test "$have_hdfeos5" = "yes"])

AC_CHECK_FOUND([hdfeos5], [hdfeos5],[Hdfeos5 library],$1)

done_hdfeos5="yes"
fi
])
