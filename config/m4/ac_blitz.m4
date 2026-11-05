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
                # Keep only the include directories, as -isystem so
                # warnings from the Blitz headers don't show in our
                # builds. The conda-forge blitz.pc Cflags also carry the
                # flags blitz itself was built with (-DNDEBUG -O2
                # -D_FORTIFY_SOURCE=2 ...), which would otherwise end up
                # in our builds too, including debug ones.
                ac_blitz_cflags=""
                ac_blitz_next=""
                for ac_blitz_flag in $BLITZ_CFLAGS; do
                    ac_blitz_dir=""
                    if test "x$ac_blitz_next" = "xyes"; then
                        ac_blitz_dir="$ac_blitz_flag"
                        ac_blitz_next=""
                    else
                        case "$ac_blitz_flag" in
                          -isystem) ac_blitz_next=yes ;;
                          -isystem*) ac_blitz_dir=`echo "$ac_blitz_flag" | sed 's/^-isystem//'` ;;
                          -I*) ac_blitz_dir=`echo "$ac_blitz_flag" | sed 's/^-I//'` ;;
                        esac
                    fi
                    # /usr/include is already a system directory, and
                    # -isystem on it breaks #include_next.
                    if test "x$ac_blitz_dir" != "x" && test "x$ac_blitz_dir" != "x/usr/include"; then
                        case " $ac_blitz_cflags " in
                          *" -isystem $ac_blitz_dir "*) ;;
                          *) ac_blitz_cflags="$ac_blitz_cflags -isystem $ac_blitz_dir" ;;
                        esac
                    fi
                done
                BLITZ_CFLAGS="$ac_blitz_cflags"
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
