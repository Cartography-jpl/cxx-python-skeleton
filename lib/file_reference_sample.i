%include "skeleton_common.i"
%{
#include "file_reference_sample.h"
%}

%base_import(generic_object)
%import "generic_object_sample.i"
%skeleton_shared_ptr(Skeleton::FileReferenceSample);
namespace Skeleton {
class FileReferenceSample : public GenericObject {
public:
  FileReferenceSample(const std::string& Fname);
  %python_attribute(file_name, std::string);
  %python_attribute(contents, std::string);
  std::string print_to_string() const;
  %pickle_serialization_dir();
};

std::vector<boost::shared_ptr<GenericObject> > generic_object_list();
boost::shared_ptr<GenericObject> generic_object_index(int I);
}

%template(Vector_GenericObject) std::vector<boost::shared_ptr<Skeleton::GenericObject> >;

// List of things "import *" will include
%python_export("FileReferenceSample", "generic_object_list",
               "generic_object_index", "Vector_GenericObject")
