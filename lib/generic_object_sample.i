// -*- mode: c++; -*-
// (Not really c++, but closest emacs mode)

%include "skeleton_common.i"
%{
#include "generic_object_sample.h"
%}

%base_import(generic_object)
%skeleton_shared_ptr(Skeleton::GenericObjectA);
%skeleton_shared_ptr(Skeleton::GenericObjectB);
namespace Skeleton {
class GenericObjectA : public GenericObject {
public:
  GenericObjectA(int V);
  std::string print_to_string() const;
  %python_attribute_with_set(v, int);
  %pickle_serialization();
};

class GenericObjectB : public GenericObject {
public:
  GenericObjectB(const boost::shared_ptr<GenericObjectA>& V) : a_(V) {}
  std::string print_to_string() const;
  %python_attribute_with_set(a, boost::shared_ptr<GenericObjectA>);
  %pickle_serialization();
};
}
// List of things "import *" will include
%python_export("GenericObjectA", "GenericObjectB");

  
