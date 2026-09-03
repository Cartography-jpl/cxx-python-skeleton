#####
#
# SYNOPSIS
#
#   AC_PROG_PYTEST()
#
# DESCRIPTION
#
#   This macro searches for a Python Pytest installation on your system. If
#   found you should call pytest via $(PYTEST). 
#
#   In configure.in, use as:
#
#     AC_PROG_PYTEST
#

AC_DEFUN([AC_PROG_PYTEST],[
        have_pytest=no
        AC_ARG_VAR([PYTEST], [Override the pytest executable to use,
           the default is to search for 'pytest' then fall back to 'py.test'
           (modern pytest installs only provide the 'pytest' script)])
        if test "$PYTEST" == ""; then
           AC_PATH_PROGS([PYTEST],[pytest py.test], [], [$python_search_path])
        else
           AC_PATH_PROG([PYTEST],[$PYTEST], [], [$python_search_path])
        fi
        if test -z "$PYTEST" ; then
           AC_MSG_WARN([cannot find 'pytest' or 'py.test' program.])
           PYTEST='echo "Error: pytest is not installed. " ; false'
        else
          have_pytest=yes
        fi
AM_CONDITIONAL([HAVE_PYTEST], [test "$have_pytest" = "yes"])
])
