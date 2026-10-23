# SYNOPSIS
#
#   AC_VICAR_GDALPLUGIN([required])
#
# DESCRIPTION
#
# This looks for the VICAR GDAL plugin. If we find it, we set the Makefile
# conditional HAVE_VICAR_GDALPLUGIN and VICAR_GDALPLUGIN_HOME.
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.

AC_DEFUN([AC_VICAR_GDALPLUGIN],
[
# Guard against running twice
if test "x$done_vicar_gdalplugin" = "x"; then
AC_HANDLE_WITH_ARG([vicar_gdalplugin], [vicar-gdalplugin], [VICAR GDAL Plugin], $1)

if test "x$want_vicar_gdalplugin" = "xyes"; then
        AC_MSG_CHECKING([for GDAL VICAR Plugin])
        succeeded=no
        if test "$ac_vicar_gdalplugin_path" != ""; then
            VICAR_GDALPLUGIN_HOME="$ac_vicar_gdalplugin_path/lib/gdalplugins"
            succeeded=yes
        else
           for ac_path_tmp in $prefix $CONDA_PREFIX $THIRDPARTY /opt/afids_support /opt/afids /usr /usr/local /opt /opt/local /sw ; do
             if test -e "$ac_path_tmp/lib/gdalplugins/gdal_vicar.so"; then
               VICAR_GDALPLUGIN_HOME="$ac_path_tmp/lib/gdalplugins"
               succeeded=yes
               break;
             fi
           done
        fi

        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
        else
                AC_MSG_RESULT([yes])

		VICAR_GDALPLUGIN_HOME_ABS=`eval echo ${VICAR_GDALPLUGIN_HOME}`
                AC_SUBST(VICAR_GDALPLUGIN_HOME)
                AC_SUBST(VICAR_GDALPLUGIN_HOME_ABS)
		AC_DEFINE([HAVE_VICAR_GDALPLUGIN],,[Defined if we have VICAR GDAL plugin])
                have_vicar_gdalplugin="yes"
        fi
fi
AM_CONDITIONAL([HAVE_VICAR_GDALPLUGIN], [test "$have_vicar_gdalplugin" = "yes"])

AC_CHECK_FOUND([vicar_gdalplugin], [vicar-gdalplugin],[VICAR GDAL Plugin],$1)

done_vicar_gdalplugin="yes"
fi
])
