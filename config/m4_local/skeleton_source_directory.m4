# Source directory for skeleton

AC_DEFUN([SKELETON_SOURCE_DIRECTORY], [
AC_SUBST([swigrules], [swig_rules])
AC_SUBST([srcscript], [script])
AC_SUBST([srcpython], [python])
AC_SUBST([testsupportsrc], [python/test_support])
AC_SUBST([srclib], [lib])
AC_SUBST([srcpythonlib], [${srcpython}/lib])
AC_SUBST([srcpythonbin], [${srcpython}/bin])
AC_SUBST([srccython], [bindings/python/cython])
AC_SUBST([pythonswigsrc], [bindings/python])
AC_SUBST([swigsrc], [bindings/python/swig])
AC_SUBST([installskeletondir], [\${prefix}])
AC_SUBST([skeletonswigincdir], [\${prefix}/share/geocal/swig])
AC_SUBST([swigincdir], [\${prefix}/share/geocal/swig])
AC_SUBST([skeletonpkgpythondir],[\${prefix}/${pythondir}/skeleton])
AC_SUBST([unittestdata], [unit_test_data])
])

