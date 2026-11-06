%include "skeleton_common.i"

%{
#include "thread_example.h"
%}
%base_import(generic_object)
%import "director_example.i"

%skeleton_shared_ptr(Skeleton::ThreadExample);

namespace Skeleton {
class ThreadExample : public GenericObject {
public:
  ThreadExample();
  static int run(const boost::shared_ptr<DirectorExample>& Obj, int V,
                 int Number_thread);
  std::string print_to_string() const;
};
}

// List of things "import *" will include
%python_export("ThreadExample")
