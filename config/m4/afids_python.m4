# SYNOPSIS
#
#   AFIDS_PYTHON([required])
#
# DESCRIPTION
#
# This finds the system python, and checks that the modules we need are also
# installed.
#
# Not finding python might or might not indicate an error. If you
# want an error to occur if we don't find it, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find it.


AC_DEFUN([AFIDS_PYTHON],
[
# Guard against running twice
if test "x$done_python" = "x"; then
AC_HANDLE_WITH_ARG([python], [python], [Python], $1)
if test "x$want_python" = "xyes"; then
   # Where to look for python and python related programs (e.g., pytest,
   # sphinx-build).
   if test "x$THIRDPARTY" = x ; then
      python_search_path=$PATH
   else
      python_search_path=$THIRDPARTY/bin:$PATH
   fi
   if test "x$CONDA_PREFIX" != x ; then
      python_search_path=$CONDA_PREFIX/bin:$python_search_path
   fi
   AC_MSG_CHECKING([for python])
   succeeded=no
   if test "$1" == "required"; then
        if test "x$PYTHON" = "x" -a "x$ac_python_path" != "x"; then
            PYTHON=$ac_python_path
	fi
	AC_PATH_PROG([PYTHON],[python[$PYTHON_VERSION]], [], [$python_search_path])
	AX_PYTHON_DEVEL([>= '3.4.0'])
	PYTHON_PREFIX=`$PYTHON -c "import sys; print(sys.prefix)"`
	old_ld_library_path="$LD_LIBRARY_PATH"
	PYTHON_LDPATH=$PYTHON_PREFIX/lib:$PYTHON_PREFIX/lib64:
	export LD_LIBRARY_PATH=$PYTHON_LDPATH$LD_LIBRARY_PATH
        AC_PYTHON_MODULE_WITH_VERSION(numpy, [1.7.0], [numpy.version.version], [required])
        AC_PYTHON_MODULE_WITH_VERSION(scipy, [0.10.1], [scipy.version.version], [required])
        AC_PYTHON_MODULE_WITH_VERSION(matplotlib, [1.0.1], [matplotlib.__version__], [required])
	
        AC_PYTHON_MODULE_WITH_VERSION(h5py, , , [not_required])
        AC_PYTHON_MODULE_WITH_VERSION(sphinx, , , [required])
        AC_PYTHON_MODULE_WITH_VERSION(sqlite3, , , [required])
	if test "x$PYTHON" != "x"; then
          pythondir=`$PYTHON -c "import sysconfig; print(sysconfig.get_path('purelib', vars={'base': '', 'platbase': ''}).lstrip('/'))"`
          platpythondir=`$PYTHON -c "import sysconfig; print(sysconfig.get_path('platlib', vars={'base': '', 'platbase': ''}).lstrip('/'))"`
          PYTHON_NUMPY_CPPFLAGS=`$PYTHON -c "import numpy; print('-I' + numpy.get_include())"`
	fi
	LD_LIBRARY_PATH=$old_ld_library_path
        AC_SUBST([PYTHON_NUMPY_CPPFLAGS])
        AC_SUBST([pythondir])
        AC_SUBST([platpythondir])
        AC_PROG_SPHINX
        AC_PROG_PYTEST
        if test "x$have_pytest" != "xyes" ; then
           AC_MSG_WARN(required program py.test not found)
           PYTHON=""
        fi
        if test "x$PYTHON" != "x"; then
          AC_SUBST([pkgpythondir], [\${prefix}/\${pythondir}/$PACKAGE])
          AC_SUBST(PYTHON_LDPATH)
          succeeded=yes
          have_python=yes
	fi
   else
        if test "x$PYTHON" = "x" -a "x$ac_python_path" != "x"; then
            PYTHON=$ac_python_path
	fi
	AC_PATH_PROG([PYTHON],[python[$PYTHON_VERSION]], [], [$python_search_path])
        AX_PYTHON_DEVEL([>= '3.4.0'])
        if test "x$PYTHON" != "x"; then
           pythondir=`$PYTHON -c "import sysconfig; print(sysconfig.get_path('purelib', vars={'base': '', 'platbase': ''}).lstrip('/'))"`
           platpythondir=`$PYTHON -c "import sysconfig; print(sysconfig.get_path('platlib', vars={'base': '', 'platbase': ''}).lstrip('/'))"`
           AC_SUBST([pythondir])
           AC_SUBST([platpythondir])
   	   AM_CONDITIONAL([HAVE_SPHINX], [false])
           AM_CONDITIONAL([HAVE_PYTEST], [false])
           succeeded=yes
           have_python=no
	fi
   fi
fi
AC_CHECK_FOUND([python], [python],[Python],$1)

done_python="yes"
fi
])
