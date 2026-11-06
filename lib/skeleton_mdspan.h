#ifndef SKELETON_MDSPAN_H
#define SKELETON_MDSPAN_H
// mdspan, from the conda-forge "mdspan" package (the Kokkos reference
// implementation, pinned to the 0.6.1.dev20260223 snapshot; 0.6.0 only
// has std::experimental). It is in namespace Kokkos; use it as
// Skeleton::md, so switching to std::mdspan (C++23, gcc 16) later is a
// change to this one file. Mirrors the geocal_mdspan.h the geocal plan
// describes (claude_notes/implementation_plan.md S6.1 item 7).
// m(i, j) indexing: C++20 has no multi-argument operator[].
#ifndef MDSPAN_USE_PAREN_OPERATOR
#define MDSPAN_USE_PAREN_OPERATOR 1
#endif
#include <mdspan/mdspan.hpp>

namespace Skeleton {
namespace md = Kokkos;
}
#endif
