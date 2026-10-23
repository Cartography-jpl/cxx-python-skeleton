# SYNOPSIS
#
#   AC_VICAR_RTL([required])
#
# DESCRIPTION
#
# This looks for the VICAR RTL library. If we find it, then we set the
# Makefile conditional HAVE_VICAR_RTL. We also set VICAR_RTL_CPPFFLAGS,
# VICAR_RTL_LIBS and VICMAIN_FOR_LOCATION.
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.
#
# We initially look for librtl at $V2OLB, which is where MIPL vicar
# builds this. If we don't find this, we then look for libvicar_rtl.
# Note the difference in the library names, MIPL calls this librtl but
# AFIDS calls in libvicar_rtl

AC_DEFUN([AC_VICAR_RTL],
[
# Guard against running twice
if test "x$done_vicar_rtl" = "x"; then
AC_HANDLE_WITH_ARG([vicar_rtl], [vicar-rtl], [VICAR RTL], $1)

if test "x$want_vicar_rtl" = "xyes"; then
        AC_MSG_CHECKING([for VICAR RTL library])
        succeeded=no
        if test "$ac_vicar_rtl_path" != ""; then
            VICAR_RTL_LIBS="-R$ac_vicar_rtl_path/lib -L$ac_vicar_rtl_path/lib -lvicar_rtl \$(CURSES_LIB) \$(FLIBS)"
            VICAR_RTL_CFLAGS="-I$ac_vicar_rtl_path/include/vicar_rtl"
	    VICMAIN_FOR_LOCATION="$ac_vicar_rtl_path/include/vicar_rtl/VICMAIN_FOR"
            succeeded=yes
        else
	    if test "$V2OLB" != ""; then
	      VICAR_RTL_LIBS="-R$V2OLB -L$V2OLB -lrtl"
	      VICAR_RTL_CFLAGS="-I$V2INC"
	      VICAR_RTL_PREFIX="$V2TOP"
	      VICMAIN_FOR_LOCATION="$V2INC/vicmain_for.fin"
              succeeded=yes
	    else
  	      AC_SEARCH_LIB([VICAR_RTL], [vicar-rtl], [vicar_rtl/], 
                          [zvproto.h], , [libvicar_rtl], [-lvicar_rtl])
              VICMAIN_FOR_LOCATION="${VICAR_RTL_PREFIX}/include/vicar_rtl/VICMAIN_FOR"
	    fi
	    VICAR_RTL_LIBS="${VICAR_RTL_LIBS} \$(CURSES_LIB) \$(FLIBS)"
        fi

        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
        else
                AC_MSG_RESULT([yes])
                AC_SUBST(VICAR_RTL_CFLAGS)
                AC_SUBST(VICMAIN_FOR_LOCATION)
                AC_SUBST(VICAR_RTL_LIBS)
                AC_DEFINE(HAVE_VICAR_RTL,,[Defined if we have VICAR RTL])
                have_vicar_rtl="yes"
        fi
fi

AM_CONDITIONAL([HAVE_VICAR_RTL], [test "$have_vicar_rtl" = "yes"])

AC_CHECK_FOUND([vicar_rtl], [vicar-rtl],[VICAR RTL],$1)
done_vicar_rtl="yes"
fi
])
