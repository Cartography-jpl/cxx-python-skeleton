# Source directory for skeleton

AC_DEFUN([SKELETON_SOURCE_DIRECTORY], [
AC_SUBST([cxxpythonrules], [cxx_python_rules])
AC_SUBST([srcscript], [script])
AC_SUBST([srcpython], [python])
AC_SUBST([srclib], [lib])
AC_SUBST([srcpythonbin], [${srcpython}/bin])
AC_SUBST([srccython], [bindings/python/cython])
AC_SUBST([installskeletondir], [\${prefix}])
AC_SUBST([skeletonpkgpythondir],[\${prefix}/${pythondir}/skeleton])
AC_SUBST([unittestdata], [unit_test_data])
])

