// The std::vector types bound with nb::bind_vector, declared opaque (a
// python object that refers to the C++ vector, rather than a converted
// list). Every binding file includes this, so all agree.
#pragma once
#include "cxx_python_rules_nb.h"
#include "director_example.h"
#include "file_reference_sample.h"
#include "cxx_python_rules_sample.h"
NB_MAKE_OPAQUE(std::vector<std::shared_ptr<Skeleton::DirectorExample>>)
NB_MAKE_OPAQUE(
    std::vector<std::vector<std::shared_ptr<Skeleton::DirectorExample>>>)
NB_MAKE_OPAQUE(std::vector<std::vector<int>>)
NB_MAKE_OPAQUE(std::vector<std::shared_ptr<Skeleton::GenericObject>>)
NB_MAKE_OPAQUE(std::vector<Skeleton::ValueSample>)
