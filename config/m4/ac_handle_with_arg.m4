# SYNOPSIS
#
#   AC_HANDLE_WITH_ARG([variable name], ["with" name], [Description name],
#                      [required])
#
# DESCRIPTION
#
# This processes the "--with-blah-blah" argument. This sets up the help
# message, and determines if we want to look for a particular item. For a
# variable name "blah_blah", this sets the variables want_blah_blah and
# ac_blah_blah_path (the directory given with --with-blah-blah=DIR, empty
# if the user didn't give one).
#
# Note that the "variable name" and "with name" are generally pretty similar.
# The difference is that "with" names typically contain "-" in the (e.g.,
# "with-vicar-rtl"), while the variable names can't have this in it (so
# the variable name would be "vicar_rtl").
#
# We no longer support building our own copy of a library, so we only
# search for it on the system (or in the directory given by the user).
# The fourth argument can be "do_not_use" to skip looking for the item
# entirely.

AC_DEFUN([AC_HANDLE_WITH_ARG],[
[have_][$1]="no"
[want_][$1]="no"
if test "$4" != "do_not_use"; then
  AC_ARG_WITH([$2],
        AS_HELP_STRING([--with-][$2][@<:@=DIR@:>@], [use ][$3][ (default is yes if found) - it is possible to specify the root directory for ][$3][ (optional).]),
        [
    if test "$withval" = "no"; then
        [want_][$1]="no"
    elif test "$withval" = "yes"; then
        [want_][$1]="yes"
        [ac_][$1][_path]=""
    else
        [want_][$1]="yes"
        [ac_][$1][_path]="$withval"
    fi
    ],
    [
    [want_][$1]="yes"
    ])
fi
])

# Variation where if the user doesn't give anything we default to no
AC_DEFUN([AC_HANDLE_WITH_ARG_DEFAULT_NO],[
[have_][$1]="no"
[want_][$1]="no"
if test "$4" != "do_not_use"; then
  AC_ARG_WITH([$2],
        AS_HELP_STRING([--with-][$2][@<:@=DIR@:>@], [use ][$3][ (default is no) - it is possible to specify the root directory for ][$3][ (optional).]),
        [
    if test "$withval" = "no"; then
        [want_][$1]="no"
    elif test "$withval" = "yes"; then
        [want_][$1]="yes"
        [ac_][$1][_path]=""
    else
        [want_][$1]="yes"
        [ac_][$1][_path]="$withval"
    fi
    ],
    [
    [want_][$1]="no"])
fi
])
