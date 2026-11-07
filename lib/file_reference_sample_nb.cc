// Python bindings for FileReferenceSample (file_reference_sample.h).
#include "skeleton_nb_opaque.h"
#include "file_reference_sample.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_file_reference_sample(nb::module_& m)
{
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::FileReferenceSample");
    nb::class_<FileReferenceSample> c(m, "FileReferenceSample", doc());
    c.def(nb::new_([](const std::string& Fname) {
            return std::make_shared<FileReferenceSample>(Fname);
          }),
          "Fname"_a);
    c.def_prop_ro("file_name", &FileReferenceSample::file_name,
                  doc("file_name"));
    c.def_prop_ro("contents", &FileReferenceSample::contents, doc("contents"));
    cxx_python_rules_nb::printable(c);
    // Pickles record the directory they were made in, for the relative
    // file name.
    cxx_python_rules_nb::pickle_serialization_dir(c);
    cxx_python_rules_nb::register_type<FileReferenceSample>();
  }
}
