// -*- mode: c++; -*-
// (Not really c++, but closest emacs mode)

%include "skeleton_common.i"
%{
#include "director_example.h"
%}

%base_import(generic_object)
%skeleton_shared_ptr(Skeleton::DirectorExample);
%skeleton_shared_ptr(Skeleton::DirectorExampleUser);
namespace Skeleton {
%feature("director") DirectorExample;

class DirectorExample : public GenericObject {
public:
  DirectorExample(int V);
  std::string print_to_string() const;
  virtual std::string desc() const;
  virtual int func(int v) const;
  int value_to_add() const;
  %pickle_serialization();
};

class DirectorExampleUser : public GenericObject {
public:
  DirectorExampleUser();
  std::string print_to_string() const;
  const std::vector<boost::shared_ptr<DirectorExample> >&
    value_vec() const;
  void set_vec(const std::vector<boost::shared_ptr<DirectorExample> >& V);
  const boost::shared_ptr<DirectorExample>&
    value() const;
  void set_value(const boost::shared_ptr<DirectorExample>& V);
  int apply_func(int v) const;
  int apply_func(int v, int ind) const;
  %pickle_serialization();
};

}

%template(Vector_DirectorExample) std::vector<boost::shared_ptr<Skeleton::DirectorExample> >;

// List of things "import *" will include
%python_export("DirectorExample", "DirectorExampleUser", "Vector_DirectorExample");

  
