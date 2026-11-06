#ifndef SWIG_MODULE_ALREADY_DONE
#if SWIG_VERSION < 0x040000
%module(directors="1", allprotected="1") swig_boost_array
#else
%module(moduleimport="from ._swig_wrap import $module", directors="1", allprotected="1") swig_boost_array
#endif
#endif

%include "swig_boost_array_inc.i"
%include "swig_array_inc.i"
// For print_to_string below. Without it python got an opaque, leaked
// std::string* instead of a str.
%include "std_string.i"

namespace boost {
template<class T, int D> class array {
public:
#ifdef SWIGPYTHON
  %rename(_size) size();
  int size();
  %pythoncode %{
    @property
    def size(self):
       return self._size()

    def set(self, v):
       '''Set the value to the given value. The value should be a container
       that we can enumerate over to fill in the value of the array'''
       for i, t in enumerate(v):
          if(i >= self.size):
              raise RuntimeError("Value must be exactly %d in size" % self.size)
          self[i] = t
       if(i != self.size - 1):
           raise RuntimeError("Value must be exactly %d in size" % self.size)

    def __iter__(self):
       for i in range(self.size):
           yield self[i]

    def __reduce__(self):
       return _new_from_set, (self.__class__, list(self))
  %}
  %extend {
    T __getitem__(int i) {return (*$self)[i];}
    void __setitem__(int i, const T& V) {(*$self)[i] = V;}
    std::string print_to_string() const
      {
        std::ostringstream os;
        for(const T& i : *$self)
          os << i << " ";
        return os.str();
      }
   }
#else
   int size();
   %extend {
     T read(int i) {return (*$self)[i];}
     void write(int i, const T& V) {(*$self)[i] = V;}
   }
#endif
};

#ifdef SWIGPYTHON

//--------------------------------------------------------------
// Swig doesn't have typemap templates, so we define a macro to
// do this for each type and length, and then call the macro
// below to set this up for a range of types and sizes.
// The PRECEDENCE is the order that we check for a match with
// overloaded functions. The actual value doesn't matter too much,
// just make sure it is different from any of the type check
// in swig, and that it is different for different array types
//--------------------------------------------------------------

%define %boost_array_template(NAME,TYPE, LEN, PRECEDENCE)
%template(NAME) boost::array<TYPE, LEN>;

%typemap(in) const boost::array<TYPE, LEN>& (boost::array<TYPE, LEN> a, PythonObject numpy)
{
  numpy.obj = to_numpy<TYPE >($input);
  if(!numpy.obj)
    return NULL;
  try {
    a = to_boost_array<TYPE, LEN>(numpy);
  } catch(const std::exception& exc) {
    SWIG_exception_fail(SWIG_ValueError, exc.what());
  }
  $1 = &a;
}

%typemap(in) boost::array<TYPE, LEN> (PythonObject numpy)
{
  numpy.obj = to_numpy<TYPE >($input);
  if(!numpy.obj)
    return NULL;
  try {
    $1 = to_boost_array<TYPE, LEN>(numpy);
  } catch(const std::exception& exc) {
    SWIG_exception_fail(SWIG_ValueError, exc.what());
  }
}

%typecheck(PRECEDENCE) boost::array<TYPE, LEN>, const boost::array<TYPE, LEN>& {
  PythonObject t(to_numpy<TYPE >($input));
  if(!t.obj || PyArray_NDIM((PyArrayObject*) t.obj) != 1)
    return 0;
  blitz::Array<TYPE, 1> b = to_blitz_array<TYPE, 1>(t.obj);
  return (b.rows() == LEN ? 1 : 0);
}

%enddef

%define %python_attribute_boost_array(NAME, TYPE, LEN)
   %extend {
    blitz::Array<TYPE, 1> _v_ ## NAME() const
    {
      blitz::Array<TYPE, 1> res(LEN);
      std::copy($self->NAME.begin(), $self->NAME.end(), res.begin());
      return res;
    }
    void _v_ ## NAME(const blitz::Array<TYPE, 1>& V)
    {
      if(V.rows() != LEN)
        throw std::runtime_error("Array not expeced size");
      std::copy(V.begin(), V.end(), $self->NAME.begin());
    }
    }
%pythoncode {
@property
def NAME(self):
    return self._v_ ## NAME()

@NAME.setter
def NAME(self, value):
  self._v_ ## NAME(value)
}
%enddef

#endif
}
%boost_array_template(Array_double_20, double, 20, 1241)
%boost_array_template(Array_double_12, double, 12, 1242)
%boost_array_template(Array_double_14, double, 14, 1243)
%boost_array_template(Array_double_3, double, 3, 1244)
%boost_array_template(Array_bool_20, bool, 20, 1245)

#ifdef SWIGPYTHON
//--------------------------------------------------------------
// std::array. Unlike boost::array this isn't wrapped as a class:
// arguments take anything numpy can convert (numpy array, list,
// tuple) of the right length, and return values are a new 1-d numpy
// array (a copy). Members are exposed with %python_attribute_std_array.
//--------------------------------------------------------------

%define %std_array_template(TYPE, LEN, PRECEDENCE)
%typemap(in) const std::array<TYPE, LEN>& (std::array<TYPE, LEN> a, PythonObject numpy)
{
  numpy.obj = to_numpy<TYPE >($input);
  if(!numpy.obj)
    return NULL;
  try {
    a = to_std_array<TYPE, LEN>(numpy);
  } catch(const std::exception& exc) {
    SWIG_exception_fail(SWIG_ValueError, exc.what());
  }
  $1 = &a;
}

%typemap(in) std::array<TYPE, LEN> (PythonObject numpy)
{
  numpy.obj = to_numpy<TYPE >($input);
  if(!numpy.obj)
    return NULL;
  try {
    $1 = to_std_array<TYPE, LEN>(numpy);
  } catch(const std::exception& exc) {
    SWIG_exception_fail(SWIG_ValueError, exc.what());
  }
}

%typecheck(PRECEDENCE) std::array<TYPE, LEN>, const std::array<TYPE, LEN>& {
  PythonObject t(to_numpy<TYPE >($input));
  if(!t.obj || PyArray_NDIM((PyArrayObject*) t.obj) != 1)
    return 0;
  blitz::Array<TYPE, 1> b = to_blitz_array<TYPE, 1>(t.obj);
  return (b.rows() == LEN ? 1 : 0);
}

%typemap(out) std::array<TYPE, LEN> {
  $result = std_array_to_numpy<TYPE, LEN>($1);
  if(!$result)
    SWIG_fail;
}

%typemap(out) const std::array<TYPE, LEN>& {
  $result = std_array_to_numpy<TYPE, LEN>(*$1);
  if(!$result)
    SWIG_fail;
}
%enddef

// The member property is the same as for boost::array.
%define %python_attribute_std_array(NAME, TYPE, LEN)
%python_attribute_boost_array(NAME, TYPE, LEN)
%enddef

%std_array_template(double, 20, 1251)
%std_array_template(double, 12, 1252)
%std_array_template(double, 14, 1253)
%std_array_template(double, 3, 1254)
%std_array_template(bool, 20, 1255)
#endif
