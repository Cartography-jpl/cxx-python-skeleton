# SYNOPSIS
#
#   AC_BOOST([required], [MINIMUM-VERSION])
#
# DESCRIPTION
#
# This looks for the BOOST libraries. If we find them, we set the Makefile
# conditional HAVE_BOOST. We as set BOOST_CPPFLAGS, and BOOST_LIBDIR.
#
# Because the libraries can have different names on different systems,
# we also set BOOST_REGEX_LIB, BOOST_FILESYSTEM_LIB, BOOST_DATETIME_LIB,
# BOOST_IOSTREAMS_LIB, and BOOST_SERIALIZATION_LIB that can be used on
# a link line (e.g., it might be "-lboost_regex" on a number of linux
# systems). We can extend this with other boost libraries if needed, but
# right now this is all we use.
#
# This is a thin wrapper around the Autoconf Archive macros AX_BOOST_BASE,
# AX_BOOST_REGEX etc., which do the actual search. The --with-boost=DIR
# and --with-boost-libdir=DIR options come from AX_BOOST_BASE. If the user
# doesn't give --with-boost, we first look for boost in $prefix,
# $CONDA_PREFIX, $THIRDPARTY and /opt/afids_support, before falling back to
# the normal AX_BOOST_BASE search.
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.

AC_DEFUN([AC_BOOST],
[
# Guard against running twice
if test "x$done_boost" = "x"; then
have_boost="no"
boost_missing=no

# AX_BOOST_BASE doesn't look in our usual places, so if the user didn't give
# --with-boost, point it at the first place we find boost.
if test "x${with_boost+set}" != "xset"; then
    for ac_boost_path_tmp in $prefix $CONDA_PREFIX $THIRDPARTY /opt/afids_support ; do
        if test -r "$ac_boost_path_tmp/include/boost/version.hpp"; then
            with_boost="$ac_boost_path_tmp"
            break
        fi
    done
fi

AX_BOOST_BASE([ifelse([$2], ,1.20.0,$2)], [have_boost=yes], [have_boost=no])
if test "$have_boost" = "yes"; then
    # Boost system is header only starting with 1.69, so there is no
    # library to find (and AX_BOOST_SYSTEM fails if there isn't one).
    boost_system_header_only=no
    AC_LANG_PUSH([C++])
    CPPFLAGS_SAVED="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <boost/version.hpp>]], [[
#if BOOST_VERSION < 106900
#error Boost system is not header only
#endif
]])], [boost_system_header_only=yes], [])
    CPPFLAGS="$CPPFLAGS_SAVED"
    AC_LANG_POP([C++])
    if test "$boost_system_header_only" = "no"; then
        AX_BOOST_SYSTEM
    fi
    AX_BOOST_FILESYSTEM
    AX_BOOST_REGEX
    AX_BOOST_DATE_TIME
    AX_BOOST_IOSTREAMS
    AX_BOOST_SERIALIZATION
    if test "x$BOOST_SYSTEM_LIB" = "x" -a "$boost_system_header_only" = "no"; then
        boost_missing=yes
    elif test "x$BOOST_FILESYSTEM_LIB" = "x" \
       -o "x$BOOST_REGEX_LIB" = "x" -o "x$BOOST_DATE_TIME_LIB" = "x" \
       -o "x$BOOST_IOSTREAMS_LIB" = "x" -o "x$BOOST_SERIALIZATION_LIB" = "x"; then
        boost_missing=yes
    fi
    if test "$boost_missing" = "yes"; then
        AC_MSG_WARN([We found boost, but not all the libraries we need])
        have_boost="no"
    fi
fi

if test "$have_boost" = "yes"; then
    # Our link lines expect these names. We also need boost system with
    # filesystem
    BOOST_FILESYSTEM_LIB="$BOOST_FILESYSTEM_LIB $BOOST_SYSTEM_LIB"

    # Boost 1.88 and later has process v2 as a compiled library, older
    # versions are header only. If we have the library we need to link to
    # it (we use process v2 in boost_pcall.h). The library has the same name
    # with the -mt variant that AX_BOOST_* handles. We tack this on to
    # BOOST_FILESYSTEM_LIB, so it ends up on the link line and in geocal.pc
    # without needing a separate variable.
    LDFLAGS_SAVED="$LDFLAGS"
    LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
    for boost_process_lib in boost_process boost_process-mt; do
        AC_CHECK_LIB([$boost_process_lib], [main],
                     [BOOST_FILESYSTEM_LIB="$BOOST_FILESYSTEM_LIB -l$boost_process_lib"; break],
                     [], [])
    done
    LDFLAGS="$LDFLAGS_SAVED"
    BOOST_DATETIME_LIB="$BOOST_DATE_TIME_LIB"

    # The library directory, used in the pkg-config file and for -R
    BOOST_LIBDIR=`echo "$BOOST_LDFLAGS" | sed -n 's/^-L\([[^ ]]*\).*/\1/p'`
    if test "x$BOOST_LIBDIR" = "x"; then
        # Boost is in a standard location, so ask the compiler where
        BOOST_LIBDIR=$(dirname "$($CXX -print-file-name=libboost_regex.so)")
    fi

    # GCC has -isystem like -I. The advantage is is knows not to warn about
    # things in the header files we can't change. Boost has a number of
    # things like this
    if test "x$GCC" = "xyes"; then
        BOOST_CPPFLAGS=`echo "$BOOST_CPPFLAGS" | sed 's/^-I/-isystem/'`
        boost_include="-isystem"
    else
        boost_include="-I"
    fi
# We have a fix needed for specific versions of boost. Go ahead and
# check for these.
    if test "$have_boost" = "yes" ; then
	   CPPFLAGS_SAVED="$CPPFLAGS"
	   CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
	   export CPPFLAGS

 	   LDFLAGS_SAVED="$LDFLAGS"
 	   LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
 	   export LDFLAGS

 	   AC_REQUIRE([AC_PROG_CXX])
 	   AC_LANG_PUSH(C++)

 	   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
 	   @%:@include <boost/version.hpp>
 	   ]], [[
 	   #if BOOST_VERSION / 100 == 1056
 	   // At 1.56
 	   #else
 	   #  error Boost version is not 1.56
 	   #endif
 	   ]])],[
	   BOOST_CPPFLAGS="$boost_include$srcdir/boost_fix/1.56 $BOOST_CPPFLAGS"
 	   ],[
 	   ])

 	   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
 	   @%:@include <boost/version.hpp>
 	   ]], [[
 	   #if BOOST_VERSION / 100 == 1057
 	   // At 1.57
 	   #else
 	   #  error Boost version is not 1.57
 	   #endif
 	   ]])],[
	   BOOST_CPPFLAGS="$boost_include$srcdir/boost_fix/1.57 $BOOST_CPPFLAGS"
 	   ],[
 	   ])

 	   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
 	   @%:@include <boost/version.hpp>
 	   ]], [[
 	   #if BOOST_VERSION / 100 == 1058
 	   // At 1.58
 	   #else
 	   #  error Boost version is not 1.58
 	   #endif
 	   ]])],[
	   BOOST_CPPFLAGS="$boost_include$srcdir/boost_fix/1.58 $BOOST_CPPFLAGS"
 	   ],[
 	   ])

	   # Also, check for the header file 
           AC_CHECK_HEADERS([boost/serialization/boost_array.hpp])
 	   AC_LANG_POP([C++])
        fi

    BOOST_CPPFLAGS="$BOOST_CPPFLAGS -DBOOST_TEST_DYN_LINK"
    AC_SUBST(BOOST_CPPFLAGS)
    AC_SUBST(BOOST_LIBDIR)
    AC_SUBST(BOOST_REGEX_LIB)
    AC_SUBST(BOOST_FILESYSTEM_LIB)
    AC_SUBST(BOOST_DATETIME_LIB)
    AC_SUBST(BOOST_IOSTREAMS_LIB)
    AC_SUBST(BOOST_SERIALIZATION_LIB)
fi
AM_CONDITIONAL([HAVE_BOOST], [test "$have_boost" = "yes"])
AC_CHECK_FOUND([boost], [boost],[BOOST],$1)
done_boost="yes"
fi
])
