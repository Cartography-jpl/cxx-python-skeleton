// -*- mode: c++; -*-
// (Not really c++, but closest emacs mode)

%include "skeleton_common.i"
%{
#include "director_example.h"
%}

%base_import(generic_object)
%skeleton_shared_ptr(Skeleton::DirectorExample);
%skeleton_shared_ptr(Skeleton::DirectorExampleUser);

// Note we need the template before its first use, so we have all
// the typemaps in place.
%template(Vector_DirectorExample) std::vector<boost::shared_ptr<Skeleton::DirectorExample> >;
%template(Vector_Vector_DirectorExample) std::vector<std::vector<boost::shared_ptr<Skeleton::DirectorExample> > >;
%template(Vector_int) std::vector<int>;
%template(Vector_Vector_int) std::vector<std::vector<int> >;

namespace Skeleton {
%feature("director") DirectorExample;

class DirectorExample : public GenericObject {
public:
  DirectorExample(int V, const std::string& Name);
  std::string print_to_string() const;
  virtual std::string desc() const;
  virtual int func(int v) const;
  int value_to_add() const;
  const std::string& name() const;
  %pickle_serialization();
protected:  
  // Only meant for swig serialization
  DirectorExample();
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
%skeleton_director_serialization(DirectorExample)
// TODO Make this into a macro, when we have everything ironed out
%shared_ptr(SwigDirector_DirectorExample)
%init {
  Skeleton::SwigTypeMapperBase::add(typeid(SwigDirector_DirectorExample), boost::make_shared<Skeleton::SwigTypeMapper< SwigDirector_DirectorExample > > ("boost::shared_ptr< SwigDirector_DirectorExample > *"));
}

%{
 #include "skeleton_serialize_support.h"
 #include "director_example_wrap.h"
 namespace boost {
   namespace serialization {
     template<class Archive>
     void serialize(Archive& ar, SwigDirector_DirectorExample& D, const unsigned int version) {
       ar & boost::serialization::make_nvp(BOOST_PP_STRINGIZE(DirectorExample),
 					  boost::serialization::base_object<Skeleton::DirectorExample>(D));
     }
     template<class Archive> 
     void save_construct_data(Archive & ar,
			      const SwigDirector_DirectorExample* d, 
			      const unsigned int version)
     {
       PyObject* pobj = d->swig_get_self();
       PyObject* this_save = PyObject_GetAttr(pobj, PyString_FromString("this"));
       PyObject_SetAttr(pobj, PyString_FromString("this"), Py_None);
       std::string python_object = cpickle_dumps(pobj);
       ar & SKELETON_NVP(python_object);
       PyObject_SetAttr(pobj, PyString_FromString("this"), this_save);
       Py_DECREF(this_save);
     }
     template<class Archive>
     void load_construct_data(Archive & ar, SwigDirector_DirectorExample* d,
 			     const unsigned int version)
     {
       std::string python_object;
       ar & SKELETON_NVP(python_object);
       PyObject* pobj = cpickle_loads(python_object);
       ::new(d)SwigDirector_DirectorExample(pobj);
       boost::shared_ptr<Skeleton::DirectorExample> *smartresult = d ? new boost::shared_ptr<Skeleton::DirectorExample>(d) : 0;
       PyObject* thisobj = SWIG_NewPointerObj(SWIG_as_voidptr(smartresult), SWIG_TypeQuery("boost::shared_ptr<Skeleton::DirectorExample>*"), SWIG_POINTER_NEW | SWIG_POINTER_OWN);
       // Transfer ownership of SwigDirector_DirectorExample to pobj.
       SWIG_Python_SetSwigThis(pobj, thisobj);
       Py_DECREF(thisobj);
     }
   }
}
BOOST_CLASS_EXPORT_KEY(SwigDirector_DirectorExample);
BOOST_CLASS_EXPORT_IMPLEMENT(SwigDirector_DirectorExample);
template void boost::serialization::serialize(boost::archive::polymorphic_oarchive& ar, SwigDirector_DirectorExample& D, const unsigned int version);
template void boost::serialization::serialize(boost::archive::polymorphic_iarchive& ar, SwigDirector_DirectorExample& D, const unsigned int version);
template void boost::serialization::load_construct_data
  (polymorphic_iarchive & ar, SwigDirector_DirectorExample* d, const unsigned int version);
template void boost::serialization::save_construct_data
  (polymorphic_oarchive & ar, const SwigDirector_DirectorExample* d, const unsigned int version);
%}
  
// List of things "import *" will include
%python_export("DirectorExample", "DirectorExampleUser",
	       "Vector_DirectorExample", "Vector_Vector_DirectorExample",
	       "Vector_int", "Vector_Vector_int");

  
