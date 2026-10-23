# SYNOPSIS
#
#   AC_HDF5([required])
#
# DESCRIPTION
#
# This looks for the HDF 5 libraries. If we find them, we set the Makefile
# conditional HAVE_HDF5. We as set HDF5_CPPFLAGS and HDF5_LDFLAGS
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.

AC_DEFUN([AC_HDF5],
[
# Guard against running twice
if test "x$done_hdf5" = "x"; then
AC_HANDLE_WITH_ARG([hdf5], [hdf5], [HDF 5], $1)

if test "x$want_hdf5" = "xyes"; then
        AC_MSG_CHECKING([for HDF5 library])
        succeeded=no
        if test "$ac_hdf5_path" != ""; then
            HDF5HOME="$ac_hdf5_path"
            HDF5_LIBS="$ac_hdf5_path/lib/libhdf5_cpp.la $ac_hdf5_path/lib/libhdf5_hl.la $ac_hdf5_path/lib/libhdf5.la -L$ac_hdf5_path/lib -lz"
            HDF5_CFLAGS="-I$ac_hdf5_path/include"
            succeeded=yes
        else
	    AC_SEARCH_LIB([HDF5], [hdf5], , [hdf5.h], ,
                          [libhdf5_cpp], [-lhdf5_cpp -lhdf5_hl -lhdf5 -lz])
            HDF5HOME="$HDF5_PREFIX"
        fi

        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
        else
                HDF5_BIN="$HDF5HOME/bin"
                AC_MSG_RESULT([yes])
                AC_SUBST(HDF5_CFLAGS)
                AC_SUBST(HDF5_LIBS)
                AC_SUBST(HDF5HOME)
                AC_SUBST(HDF5_BIN)
                AC_DEFINE(HAVE_HDF5,,[Defined if we have HDF5])
                have_hdf5="yes"
        fi
fi
AM_CONDITIONAL([HAVE_HDF5], [test "$have_hdf5" = "yes"])

# For right now, build netcdf if we are building hdf5. Might want
# more complicated logic at some point, but this is sufficient for
# now.


AC_CHECK_FOUND([hdf5], [hdf5],[HDF 5],$1)
done_hdf5="yes"
fi
])
