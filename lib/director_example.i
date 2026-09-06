// -*- mode: c++; -*-
// (Not really c++, but closest emacs mode)

%include "skeleton_common.i"
%{
#include "director_example.h"
%}

%base_import(generic_object)

// Note we need the template before its first use, so we have all
// the typemaps in place.
%template(Vector_DirectorExample) std::vector<boost::shared_ptr<Skeleton::DirectorExample> >;
%template(Vector_Vector_DirectorExample) std::vector<std::vector<boost::shared_ptr<Skeleton::DirectorExample> > >;
%template(Vector_Vector_int) std::vector<std::vector<int> >;

// We also need the shared ptr after the templates, so we override the
// output type maps for std::vector<boost::shared_ptr<T> >
%skeleton_shared_ptr(Skeleton::DirectorExample);
%skeleton_shared_ptr(Skeleton::DirectorExampleNested);
%skeleton_shared_ptr(Skeleton::DirectorExampleUser);
%skeleton_shared_ptr(Skeleton::DirectorExampleWeakPtr);

namespace Skeleton {
%feature("director") DirectorExample;

class DirectorExample : public GenericObject {
public:
  DirectorExample(int V, const std::string& Name);
  std::string print_to_string() const;
  virtual std::string desc() const;
  virtual int func(int v) const;
  int value_to_add() const;
  void value_to_add(int v);
  const std::string& name() const;
  bool same_object(const DirectorExample& obj2) const
  { return this == &obj2; }
  %pickle_serialization();
protected:  
  // Only meant for swig serialization
  DirectorExample();
};

class DirectorExampleNested : public DirectorExample {
public:
  DirectorExampleNested(int value_to_add, const std::string& name, const boost::shared_ptr<DirectorExample> Obj);
  %python_attribute(obj, boost::shared_ptr<DirectorExample>);
  %pickle_serialization();
};
  
class DirectorExampleWeakPtr : public GenericObject {
public:
  // See DirectorNotes.md on the used of SHARED_PTR_NO_OWN
  DirectorExampleWeakPtr(const boost::shared_ptr<DirectorExample>& SHARED_PTR_NO_OWN);
  std::string print_to_string() const;
  boost::shared_ptr<DirectorExample> a() const;
  void a(const boost::shared_ptr<DirectorExample>& SHARED_PTR_NO_OWN);
  bool expired() const;
  long use_count() const;
  %pickle_serialization();
};
  
class DirectorExampleUser : public GenericObject {
public:
  DirectorExampleUser();
  std::string print_to_string() const;
  const std::vector<boost::shared_ptr<DirectorExample> >&
    value_vec() const;
  void set_vec(const std::vector<boost::shared_ptr<DirectorExample> >& V);
  const std::vector<std::vector<boost::shared_ptr<DirectorExample> > >&
    value_vec_vec() const;
  void set_vec_vec(const std::vector<std::vector<boost::shared_ptr<DirectorExample> > >& V);
  const boost::shared_ptr<DirectorExample>&
    value() const;
  const std::vector<int>& value_vec_int() const;
  void set_vec_int(const std::vector<int>& V);
  const std::vector<std::vector<int> >& value_vec_vec_int() const;
  void set_vec_vec_int(const std::vector<std::vector<int> >& V);
  void set_value(const boost::shared_ptr<DirectorExample>& V);
  int apply_func(int v) const;
  int apply_func(int v, int ind) const;
  int apply_func(int v, int ind, int ind2) const;
  %pickle_serialization();
};

}

// Extra code for handling boost serialization/python pickle of
// director classes
%skeleton_director_serialization(director_example, DirectorExample)
  
// List of things "import *" will include
%python_export("DirectorExample", "DirectorExampleUser",
	       "DirectorExampleNested",
	       "DirectorExampleWeakPtr",
	       "Vector_DirectorExample", "Vector_Vector_DirectorExample",
	       "Vector_Vector_int");

  
