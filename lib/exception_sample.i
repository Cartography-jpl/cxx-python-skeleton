%include "skeleton_common.i"
%{
#include "exception_sample.h"
%}

%base_import(generic_object)
%import "skeleton_exception.i"
%skeleton_shared_ptr(Skeleton::ExceptionSample);
namespace Skeleton {
%feature("director") ExceptionSample;
class ExceptionSample : public GenericObject {
public:
  ExceptionSample();
  void throw_skeleton_exception(const std::string& Msg) const;
  void throw_std_exception(const std::string& Msg) const;
  void throw_unknown() const;
  int checked_value(int V) const;
  virtual ~ExceptionSample();
  virtual int value() const;
  std::string value_error_message() const;
  int value_rethrow_copy() const;
  static Exception caught_exception(const std::string& Msg);
  std::string print_to_string() const;
};
}
// List of things "import *" will include
%python_export("ExceptionSample")
