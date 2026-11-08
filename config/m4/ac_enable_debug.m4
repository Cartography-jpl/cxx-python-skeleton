AC_DEFUN([AC_ENABLE_DEBUG],[

AC_MSG_CHECKING([whether to enable debug flags when compiling])
AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],[Enable compiler debugging flags]),
[if test "$enableval" = yes; then
  AC_MSG_RESULT([yes])
  # Use conda's debug flags when its compiler activation set them. A
  # conda env without the activation (e.g. plain gcc/gxx packages) has
  # CONDA_PREFIX but no DEBUG_CXXFLAGS, and would otherwise get no
  # debug flags at all, so fall back to the gcc flags below.
  #
  # conda's activation also puts "-DNDEBUG -D_FORTIFY_SOURCE=2 -O2" in
  # CPPFLAGS, and -DNDEBUG would turn off assert() in library headers
  # (BOOST_ASSERT and so on). So use its DEBUG_CPPFLAGS instead (the same
  # without -DNDEBUG, and -Og for -O2), or drop -DNDEBUG if those aren't
  # set. -DBZ_DEBUG turns on Blitz's bounds checks, as in the gcc branch.
  if test "x$CONDA_PREFIX" != x && test "x$DEBUG_CXXFLAGS" != x; then
    CXXFLAGS="$DEBUG_CXXFLAGS -DBZ_DEBUG"
    CFLAGS="$DEBUG_CFLAGS"
    if test "x$DEBUG_CPPFLAGS" != x; then
      CPPFLAGS="$DEBUG_CPPFLAGS"
    else
      CPPFLAGS=`echo " $CPPFLAGS " | sed -e 's/ -DNDEBUG / /g' -e 's/^ //' -e 's/ $//'`
    fi
# There are some legitimate uses of mve in p1 that appear to violate the
# bounds-check. This is because arrays are passed into fortran that have
# negative indicies, which is allowed in fortran. So turn off the bound-check
# for fortran.
    FFLAGS="$DEBUG_FFLAGS -fno-bounds-check"
  elif test "$GCC" = yes; then
    CXXFLAGS="-ggdb -DBZ_DEBUG -fbounds-check"
    CFLAGS="-ggdb -fbounds-check"
# There are some legitimate uses of mve in p1 that appear to violate the
# bounds-check. This is because arrays are passed into fortran that have
# negative indicies, which is allowed in fortran. So turn off the bound-check
# for fortran.
    FFLAGS="-ggdb"
  fi
fi],
[AC_MSG_RESULT([no])
 enable_debug=no
 AC_DEFINE([BOOST_DISABLE_ASSERTS],[],[Disable assertions in boost])
 if test "$enable_code_coverage" = yes; then
   CXXFLAGS="-g -Wall"
   CFLAGS="-g -Wall"
   FFLAGS="-g"
 else
   CXXFLAGS="$CXXFLAGS -Wall"
   CFLAGS="$CFLAGS -Wall"
 fi
]
)
AC_ARG_VAR([DEBUG_FLAGS],[Compiler debugging flags])
])

