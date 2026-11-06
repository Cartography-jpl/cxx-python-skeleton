%include "skeleton_common.i"

%{
#include "skeleton_exception.h"
%}

%base_import(generic_object)

%skeleton_shared_ptr(Skeleton::Exception)

namespace Skeleton {
  class Exception : public GenericObject {
  public:
    Exception(const std::string& W);
    const char* what();
    std::string print_to_string() const;
    std::string stacktrace() const;
    static void capture_stacktrace(bool V);
    static bool capture_stacktrace();
  };
}

// std::source_location isn't wrapped, give python its parts.
%extend Skeleton::Exception {
  std::string source_file() const { return $self->source_location().file_name(); }
  int source_line() const { return static_cast<int>($self->source_location().line()); }
  std::string source_function() const { return $self->source_location().function_name(); }
}

// List of things "import *" will include
%python_export("Exception")
