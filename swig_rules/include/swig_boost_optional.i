#ifndef SWIG_MODULE_ALREADY_DONE
#if SWIG_VERSION < 0x040000  
%module(directors="1", allprotected="1") foo
#else
%module(moduleimport="from ._swig_wrap import $module", directors="1", allprotected="1") foo
#endif
#endif

%{
#include <boost/optional.hpp>
%}

// Don't think it is a problem assuming we also have blitz. If it is,
// we can separate this .i file in the future.
%import "swig_array_inc.i"

// Note that the by value "in" typemaps set $1 directly, the const
// reference ones point $1 at a local. A python value that can't be
// converted (e.g., a string) is a TypeError, rather than silently
// becoming -1 (what PyLong_AsLong returns on error).

%typemap(in) boost::optional<int> %{
    if($input == Py_None)
        $1 = boost::optional<int>();
    else {
        long t = PyLong_AsLong($input);
        if(t == -1 && PyErr_Occurred())
            SWIG_exception_fail(SWIG_TypeError, "in method '$symname', expecting int or None");
        $1 = boost::optional<int>((int) t);
    }
%}

%typemap(in) const boost::optional<int>& (boost::optional<int> v) %{
    if($input == Py_None)
        v = boost::optional<int>();
    else {
        long t = PyLong_AsLong($input);
        if(t == -1 && PyErr_Occurred())
            SWIG_exception_fail(SWIG_TypeError, "in method '$symname', expecting int or None");
        v = boost::optional<int>((int) t);
    }
    $1 = &v;
%}
   
%typemap(out) boost::optional<int> %{
    if($1)
        $result = PyLong_FromLong(*$1);
    else
    {
        $result = Py_None;
        Py_INCREF(Py_None);
    }
%}

%typemap(out) const boost::optional<int>& %{
    if(*$1)
        $result = PyLong_FromLong(**$1);
    else
    {
        $result = Py_None;
        Py_INCREF(Py_None);
    }
%}

%typemap(in) boost::optional<double> %{
    if($input == Py_None)
        $1 = boost::optional<double>();
    else {
        double t = PyFloat_AsDouble($input);
        if(t == -1.0 && PyErr_Occurred())
            SWIG_exception_fail(SWIG_TypeError, "in method '$symname', expecting float or None");
        $1 = boost::optional<double>(t);
    }
%}

%typemap(in) const boost::optional<double>& (boost::optional<double> v) %{
    if($input == Py_None)
        v = boost::optional<double>();
    else {
        double t = PyFloat_AsDouble($input);
        if(t == -1.0 && PyErr_Occurred())
            SWIG_exception_fail(SWIG_TypeError, "in method '$symname', expecting float or None");
        v = boost::optional<double>(t);
    }
    $1 = &v;
%}
   
%typemap(out) boost::optional<double> %{
    if($1)
        $result = PyFloat_FromDouble(*$1);
    else
    {
        $result = Py_None;
        Py_INCREF(Py_None);
    }
%}

%typemap(out) const boost::optional<double>& %{
    if(*$1)
        $result = PyFloat_FromDouble(**$1);
    else
    {
        $result = Py_None;
        Py_INCREF(Py_None);
    }
%}

%typemap(in) const boost::optional<blitz::Range>& (boost::optional<blitz::Range> v) %{
    if($input == Py_None)
        v = boost::optional<blitz::Range>();
    else {
        blitz::Range* r = 0;
        int res = SWIG_ConvertPtr($input, (void**) &r, $descriptor(blitz::Range*), 0);
        if(!SWIG_IsOK(res) || !r)
            SWIG_exception_fail(SWIG_TypeError, "in method '$symname', expecting blitz::Range or None");
        v = boost::optional<blitz::Range>(*r);
    }
    $1 = &v;
%}

%typemap(out) boost::optional<blitz::Range> %{
    // $1 may be a SwigValueWrapper<boost::optional<blitz::Range> > here,
    // which only provides one implicit conversion (to
    // boost::optional<blitz::Range>&) -- chaining that with optional's own
    // operator bool() isn't allowed implicitly, so force the first
    // conversion explicitly via a named local. Scoped in its own block
    // so it doesn't cross the "fail:" label SWIG's argument-checking
    // macros jump to earlier in the wrapper function.
    {
      boost::optional<blitz::Range> swig_opt_range = $1;
      if (swig_opt_range) {
        $result = SWIG_NewPointerObj(new blitz::Range(swig_opt_range.get()), $descriptor(blitz::Range*), SWIG_POINTER_OWN | 0);
      } else {
        $result = Py_None;
        Py_INCREF(Py_None);
      }
    }
%}

%typemap(out) const boost::optional<blitz::Range>& %{
    if (*$1) {
        $result = SWIG_NewPointerObj(new blitz::Range($1->get()), $descriptor(blitz::Range*), SWIG_POINTER_OWN | 0);
    } else {
        $result = Py_None;
        Py_INCREF(Py_None);
    }
%}
