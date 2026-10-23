# SYNOPSIS
#
#   AC_GDAL([required])
#
# DESCRIPTION
#
# This looks for the GDAL libraries. If we find them, we set the Makefile
# conditional HAVE_GDAL. We as set GDAL_CFLAGS and GDAL_LIBS.
#
# If the variable $V2OLB is defined (so we are in a MIPL environment) we
# initially look for GDAL at $GDALLIB directory

AC_DEFUN([AC_GDAL],
[
# Guard against running twice
if test "x$done_gdal" = "x"; then
AC_HANDLE_WITH_ARG([gdal], [gdal], [GDAL], $1)
if test "x$want_gdal" = "xyes"; then
        AC_MSG_CHECKING([for GDAL library])
        succeeded=no
        if test "$ac_gdal_path" != ""; then
            GDAL_PREFIX="$ac_gdal_path"
            GDAL_LIBS="-L$ac_gdal_path/lib -lgdal"
            GDAL_CFLAGS="-I$ac_gdal_path/include"
            succeeded=yes
        else
	    if test "$V2OLB" != "" && test "$GDALLIB" != ""; then
		GDAL_PREFIX="$GDALLIB"
		GDAL_LIBS="-L$GDAL_PREFIX/lib -lgdal"
		GDAL_CFLAGS="-I$GDAL_PREFIX/include"
		succeeded=yes
	    else
	      AC_SEARCH_LIB([GDAL], [gdal], , [gdal.h], , [libgdal], [-lgdal])
	    fi
        fi
        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
        else
                AC_MSG_RESULT([yes])
	fi
        if test "$succeeded" = "yes"; then
                GDAL_LIBS="`$GDAL_PREFIX/bin/gdal-config --libs`"
                GDAL_CFLAGS=`$GDAL_PREFIX/bin/gdal-config --cflags`
	        AC_MSG_CHECKING([version GDAL library is new enough])
		AC_REQUIRE([AC_PROG_CC])
		CPPFLAGS_SAVED="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $GDAL_CFLAGS"
		export CPPFLAGS
		AC_LANG_PUSH([C++])
		AC_RUN_IFELSE([AC_LANG_PROGRAM([[@%:@include <gdal.h>
@%:@include <fstream>]],
		  [[std::ofstream t("conftest.out");
		   t << GDAL_RELEASE_NAME << "\n";]])],
		   [ gdal_version=`cat conftest.out` ],
		   [AC_MSG_FAILURE([gdal version program execution failed])])
		AC_LANG_POP([C++])
		CPPFLAGS="$CPPFLAGS_SAVED"
		AX_COMPARE_VERSION([${gdal_version}], [ge], [1.9.2],
		   [AC_MSG_RESULT([yes])],
		   [AC_MSG_RESULT([no])
		    AC_MSG_WARN([We require GDAL version to be >= 1.9.2. There are bugs in earlier versions that we have encountered, so we don't use earlier version of GDAL])
		    succeeded=no])
	fi
        if test "$succeeded" = "yes" ; then
                AC_SUBST(GDAL_PREFIX)
                AC_SUBST(GDAL_CFLAGS)
                AC_SUBST(GDAL_LIBS)
                AC_DEFINE(HAVE_GDAL,,[Defined if we have GDAL])
                have_gdal="yes"
        fi
fi
AM_CONDITIONAL([HAVE_GDAL], [test "$have_gdal" = "yes"])

AC_CHECK_FOUND([gdal], [gdal],[Gdal],$1)

done_gdal="yes"
fi
])
