%fragment("StdTraits");

%header %{

//--------------------------------------------------------------
/// The default conversion of a python sequence to a std::vector<T>
/// doesn't work correctly for boost::shared_ptr types. This
/// expression here does this correctly.
//--------------------------------------------------------------

// Code changed a little between SWIG 3 and SWIG 4, so have different
// versions depending on swig version.
#if SWIG_VERSION < 0x040000
namespace swig {
  template <class T>
  struct traits_asptr<std::vector<boost::shared_ptr<T> > >  {
    static int asptr(PyObject *obj,
                     std::vector<boost::shared_ptr<T> > **vec) {
      // Long name, so shorten
      typedef std::vector<boost::shared_ptr<T> > vtype;
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
            typename std::vector< boost::shared_ptr<T > >::value_type temp2shared2;
            while((item = PyIter_Next(iterator))) {
              boost::shared_ptr<T> *itemp;
              int newmem = 0;
              int res = SWIG_ConvertPtrAndOwn(item, (void**) &itemp,
                        swig::type_info<boost::shared_ptr<T> >(),  0, &newmem);
              if(!SWIG_IsOK(res)) {
                Py_DECREF(item);
                Py_DECREF(iterator);
                return SWIG_ERROR;
              }
              // Added mms
              // Special handling if this is a director class. In that case, we
              // don't own the underlying python object. See
              // DirectorNotes.md for details.
              Swig::Director* dp = dynamic_cast<Swig::Director*>(itemp->get());
              if(dp) {
                temp2shared2.reset(itemp->get(), PythonRefPtrCleanup(dp->swig_get_self()));
                itemp = &temp2shared2;
              }
              pseq->push_back(*itemp);
              Py_DECREF(item);
            }
            Py_DECREF(iterator);
            *vec = pseq;
            return SWIG_NEWOBJ;
          } else {
            SwigPySequence_Cont<boost::shared_ptr<T> > swigpyseq(obj);
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
#else
namespace swig {
  template <class SwigPySeq, class T>
  inline void
  assign(const SwigPySeq& swigpyseq, std::vector<boost::shared_ptr<T> >* seq) {
    // seq->assign(swigpyseq.begin(), swigpyseq.end()); // not used as not always implemented
    typedef typename SwigPySeq::value_type value_type;
    typename SwigPySeq::const_iterator it = swigpyseq.begin();
    for (;it != swigpyseq.end(); ++it) {
      value_type itemp = (value_type)(*it);
      // Added mms
      // Special handling if this is a director class. In that case, we
      // don't own the underlying python object. See
      // DirectorNotes.md for details.
      Swig::Director* dp = dynamic_cast<Swig::Director*>(itemp.get());
      if(dp) {
        // Diagnostic to make sure we end up here when we should
        //std::cerr << "Setting up PythonRefPtrCleanup\n";
        itemp.reset(itemp.get(), PythonRefPtrCleanup(dp->swig_get_self()));
      }
      seq->insert(seq->end(),itemp);
    }
  }

  template <class T>
  struct traits_asptr<boost::shared_ptr<T> > {
    static int asptr(PyObject *obj, boost::shared_ptr<T> **val) {
      int res = SWIG_ERROR;
      swig_type_info *descriptor = swig::type_info<boost::shared_ptr<T> >();
      if (val) {
        boost::shared_ptr<T> *p = 0;
        int newmem = 0;
        res = descriptor ? SWIG_ConvertPtrAndOwn(obj, (void **)&p, descriptor, 0, &newmem) : SWIG_ERROR;
        if (SWIG_IsOK(res)) {
          if (newmem & SWIG_CAST_NEW_MEMORY) {
            res |= SWIG_NEWOBJMASK;
          }
          // Added mms
          // Special handling if this is a director class. In that case, we
          // don't own the underlying python object. See
          // DirectorNotes.md for details.
          Swig::Director* dp = dynamic_cast<Swig::Director*>(p->get());
          if(dp) {
            boost::shared_ptr<T> *p2 = new boost::shared_ptr<T>(p->get(), PythonRefPtrCleanup(dp->swig_get_self()));
            // If p was itself a heap allocation from the cast above (newmem
            // & SWIG_CAST_NEW_MEMORY), we're about to replace it with p2 --
            // free it first rather than leaking it. If newmem wasn't set, p
            // points at a shared_ptr swig still owns elsewhere, so leave it
            // alone.
            if (newmem & SWIG_CAST_NEW_MEMORY) {
              delete p;
            }
            p = p2;
            res |= SWIG_NEWOBJMASK;
          }
          *val = p;
        }
      } else {
        res = descriptor ? SWIG_ConvertPtr(obj, 0, descriptor, 0) : SWIG_ERROR;
      }
      return res;
    }
  };
}

#endif
%}


//--------------------------------------------------------------
/// Types that are wrapped with %shared_ptr, but that are also used
/// by value in a std::vector (e.g., std::vector<Time>). The default
/// SWIG conversion of the elements looks for the python type
/// "TYPE *", but the python objects are actually
/// "boost::shared_ptr< TYPE > *". So converting a python list to the
/// std::vector fails, and iterating over the std::vector returns
/// untyped (and leaked) "TYPE *" objects. This replaces the SWIG
/// traits for TYPE so the elements are converted through the
/// shared_ptr type.
///
/// This needs to be used before the %template for the std::vector, and
/// TYPE needs to be written the same way SWIG writes it (e.g.,
/// "GeoCal::AutoDerivative< double >").
//--------------------------------------------------------------

%define %shared_ptr_value_traits(TYPE...)
%fragment(SWIG_Traits_frag(TYPE), "header", fragment="StdTraits") {
namespace swig {
  template <>  struct traits< TYPE > {
    typedef pointer_category category;
    static const char* type_name() { return #TYPE; }
  };

  template <> struct traits_asptr< TYPE > {
    static int asptr(PyObject *obj, TYPE **val) {
      static swig_type_info *descriptor =
        SWIG_TypeQuery("boost::shared_ptr< " #TYPE " > *");
      if(!descriptor)
        return SWIG_ERROR;
      boost::shared_ptr< TYPE > *p = 0;
      int newmem = 0;
      int res = SWIG_ConvertPtrAndOwn(obj, (void **)&p, descriptor, 0,
                                      &newmem);
      if(!SWIG_IsOK(res))
        return res;
      if(!p || !*p)
        res = SWIG_ERROR;
      else if(val) {
        *val = new TYPE(**p);
        res = SWIG_NEWOBJ;
      } else
        res = SWIG_OK;
      if(newmem & SWIG_CAST_NEW_MEMORY)
        delete p;
      return res;
    }
  };

  template <> struct traits_from< TYPE > {
    static PyObject *from(const TYPE& v) {
      return SWIG_MAPPER_NAMESPACE::swig_to_python(boost::shared_ptr< TYPE >(new TYPE(v)));
    }
  };
}
}
%enddef
