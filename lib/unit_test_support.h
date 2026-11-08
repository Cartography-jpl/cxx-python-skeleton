#pragma once
#include "global_fixture.h"
#include "serialize_function.h"
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <memory>

//-----------------------------------------------------------------------
/// Check that two Blitz arrays are equal to within a tolerance. (geocal
/// has the same macros with a BOOST_ prefix, which is Boost's own.)
//-----------------------------------------------------------------------

#define SKELETON_CHECK_MATRIX_CLOSE_TOL(A, B, tol)                             \
  BOOST_CHECK_LT(max(abs((A) - (B))), (tol))
#define SKELETON_CHECK_MATRIX_CLOSE(A, B)                                      \
  SKELETON_CHECK_MATRIX_CLOSE_TOL(A, B, 1e-8)
