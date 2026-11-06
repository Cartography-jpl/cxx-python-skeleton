#ifndef SKELETON_MDSPAN_H
#define SKELETON_MDSPAN_H
// mdspan, from the conda-forge "mdspan" package (the Kokkos reference
// implementation, pinned to the 0.6.1.dev20260223 snapshot; 0.6.0 only
// has std::experimental). It is in namespace Kokkos; use it as
// Skeleton::md, so switching to std::mdspan (gcc 16; gcc 15 doesn't have
// <mdspan> yet) later is a change to this one file. Mirrors the
// geocal_mdspan.h the geocal plan describes
// (claude_notes/implementation_plan.md S6.1 item 7).
// Index with m[i, j], as std::mdspan does (C++23 multidimensional
// subscript; Kokkos turns it on by itself in C++23 mode, and turns
// m(i, j) off).
#include <mdspan/mdspan.hpp>

namespace Skeleton {
namespace md = Kokkos;
}
#endif
