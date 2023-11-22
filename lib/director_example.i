// -*- mode: c++; -*-
// (Not really c++, but closest emacs mode)

%include "skeleton_common.i"
%include <std_vector.i>
%include "swig_vector_shared_ptr.i"
%{
#include "director_example.h"
%}

%{
namespace swig {
  template <>
  struct traits_asptr<std::vector<boost::shared_ptr<Skeleton::DirectorExample> > >  {
    static int asptr(PyObject *obj, 
		     std::vector<boost::shared_ptr<Skeleton::DirectorExample> > **vec) {
      // Long name, so shorten
      typedef std::vector<boost::shared_ptr<Skeleton::DirectorExample> > vtype;
      if (obj == Py_None || SWIG_Python_GetSwigThis(obj)) {
	vtype *p;
	if (::SWIG_ConvertPtr(obj,(void**)&p,
			      swig::type_info<vtype>(),0) == SWIG_OK) {
	  if (vec) *vec = p;
	  return SWIG_OLDOBJ;
	}
      } else if (PySequence_Check(obj)) {
	try {
	  if (vec) {
	    vtype *pseq = new vtype();
	    PyObject *iterator = PyObject_GetIter(obj);
	    PyObject *item;
	    while((item = PyIter_Next(iterator))) {
	      boost::shared_ptr<Skeleton::DirectorExample> *itemp;
	      int newmem = 0;
	      int res = SWIG_ConvertPtrAndOwn(item, (void**) &itemp, 
			swig::type_info<boost::shared_ptr<Skeleton::DirectorExample> >(),  0, &newmem);
	      if(!SWIG_IsOK(res)) {
		Py_DECREF(item);
		Py_DECREF(iterator);
		return SWIG_ERROR;
	      }
	      pseq->push_back(*itemp);
	      Py_DECREF(item);
	    }
	    Py_DECREF(iterator);
	    *vec = pseq;
	    return SWIG_NEWOBJ;
	  } else {
	    SwigPySequence_Cont<boost::shared_ptr<Skeleton::DirectorExample> > swigpyseq(obj);
	    return swigpyseq.check() ? SWIG_OK : SWIG_ERROR;
	  }
	} catch (std::exception& e) {
	  if (vec) {
	    if (!PyErr_Occurred()) {
	      PyErr_SetString(PyExc_TypeError, e.what());
	    }
	  }
	  return SWIG_ERROR;
	}
      }
      return SWIG_ERROR;
    }
  };
}
%}

%base_import(generic_object)
%skeleton_shared_ptr(Skeleton::DirectorExample);
%skeleton_shared_ptr(Skeleton::DirectorExampleUser);

// Note we need the template before its first use, so we have all
// the typemaps in place.
%template(Vector_DirectorExample) std::vector<boost::shared_ptr<Skeleton::DirectorExample> >;

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
  const std::vector<int>& value_vec_int() const;
  void set_vec_int(const std::vector<int>& V);
  void set_value(const boost::shared_ptr<DirectorExample>& V);
  int apply_func(int v) const;
  int apply_func(int v, int ind) const;
  %pickle_serialization();
};

}

%template(Vector_int) std::vector<int>;

// List of things "import *" will include
%python_export("DirectorExample", "DirectorExampleUser", "Vector_DirectorExample",
	       "Vector_int");

  
