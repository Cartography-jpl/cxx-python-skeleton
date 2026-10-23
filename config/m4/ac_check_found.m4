# SYNOPSIS
#
#   AC_CHECK_FOUND([variable name], ["with" name], [library name], [required])
#
# DESCRIPTION
#
# Check if the library was found, if if not then print out an
# error message.
#
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.

AC_DEFUN([AC_CHECK_FOUND],
[
if test "$4" = "required"; then
  if test "[$have_][$1]" = "no"; then
     AC_MSG_ERROR([
The ][$3][ library is required by GeoCal. Try specifying
location using --with-][$2][ if configure could't find the
library.])
  fi
fi
])
