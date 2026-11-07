#ifndef CXX_PYTHON_RULES_MDSPAN_H
#define CXX_PYTHON_RULES_MDSPAN_H
// mdspan, from the conda-forge "mdspan" package (the Kokkos reference
// implementation, pinned to the 0.6.1.dev20260223 snapshot; 0.6.0 only
// has std::experimental). It is in namespace Kokkos; use it as
// CXX_PYTHON_RULES_NAMESPACE::md (e.g. GeoCal::md), so switching
// to std::mdspan (gcc 16; gcc 15 doesn't have <mdspan> yet) later is a
// change to this one file.
// Index with m[i, j], as std::mdspan does (C++23 multidimensional
// subscript; Kokkos turns it on by itself in C++23 mode, and turns
// m(i, j) off).
//
// generic_object.h is the project's header that brings in its
// configuration, which defines CXX_PYTHON_RULES_NAMESPACE (see
// cxx_python_rules/README.md).
#include "generic_object.h"
#include <mdspan/mdspan.hpp>

#ifndef CXX_PYTHON_RULES_NAMESPACE
#error                                                                         \
    "CXX_PYTHON_RULES_NAMESPACE must be defined by the project's configuration header"
#endif

namespace CXX_PYTHON_RULES_NAMESPACE {
namespace md = Kokkos;
}
#endif
