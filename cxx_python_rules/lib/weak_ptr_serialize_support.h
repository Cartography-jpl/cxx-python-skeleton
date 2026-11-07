#pragma once
#include "generic_object.h"
#include <memory>
#ifdef CXX_PYTHON_RULES_HAVE_BOOST_SERIALIZATION
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_free.hpp>
#endif

/****************************************************************/ /**
  There is a complication in serializing a std::weak_ptr. Normally
  when you encounter a std::shared_ptr you want to serialize the
  object, e.g. object A contains a boost:shared_ptr pointing to object
  B.

  std::weak_ptr on the other hand is a looser connection. It means
  an object A has a reference to access B, but doesn't really "own"
  it (e.g., B is a Observer of A, and A notifies it about changes).

  There is no one "right" way to handle this, it really just depends
  on what the semantics are for you objects.

  There are two ways to handled this already in boost serialization:

  1. You don't want to keep any references. The weak_ptr is just not
     included in the serialization of class A.
  2. You always want the weak_ptr included just like we do with
     std::shared_ptr. boost has serialization code that does this
     in boost/serialization/weak_ptr.hpp.

  Often neither of these ways are what you want. For example, in
  ReFRACtor we have Pressure, which is Observerable. For case (1)
  we loose all internal connections in something like a ForwardModel,
  objects which need to know then Pressure changes loose that
  connection when we serialize the ForwardModel. On the other hand,
  we don't want to serialize just a Pressure object and also get the
  entire world through a series of pointers. Also there might be
  observers like a logger writing out when Pressure changes that we
  wouldn't want to serialize.

  We supply an alternative to the two where we include all weak_ptr
  to objects that are otherwise included in a serialization. So
  from our example, serializing a single Pressure object would have
  no weak_ptr serialized, while a ForwardModel would have all weak_ptr
  from Pressure to other objects in the ForwardModel, but not to
  object not in ForwardModel.

  This is accomplished by doing the serialization twice. The first
  time, we mark all the GenericObject  that get serialized. The second
  time, we keep all weak_ptr that point to one of these objects.

  The serialization code needs to "know" to call the
  add_serialized_reference function in this header. This isn't too
  hard in practice, usually only special objects can have weak_ptr to
  them, e.g Observers of a Observable. There is sample code for this
  in ReFRACtor if you want to look at an example of its use.

  Note that the GenericObjectMap can be used for collections of
  objects you want to have serialized together that don't already have
  a containing object, e.g. a ForwardModel plus a StateVector.

  Also note that you are not required to use this particular method if it
  doesn't fit your semantics - you can use one of the others supplied
  by boost or create your own way to handle this.
*******************************************************************/

namespace CXX_PYTHON_RULES_NAMESPACE {
void clear_ptr_serialized_reference();
void add_ptr_serialized_reference(const GenericObject* P);
bool is_ptr_serialized(const GenericObject* P);

//-----------------------------------------------------------------------
/// The marks are kept per write: each serialize_write* call makes a
/// PtrSerializeScope, which starts an empty set of marks for its mark
/// pass and its write, and drops it at the end. Scopes nest, so a write
/// started in the middle of another one (pickling a python object that
/// holds a C++ object, during the write of an archive that holds the
/// python object) has its own marks and leaves the outer write's marks
/// alone. The marks are per thread, so writes on different threads don't
/// interfere and need no lock. Outside any scope (code writing an archive
/// directly) the functions above use one set per thread.
//-----------------------------------------------------------------------

class PtrSerializeScope final {
public:
  PtrSerializeScope();
  ~PtrSerializeScope();
  PtrSerializeScope(const PtrSerializeScope&) = delete;
  PtrSerializeScope& operator=(const PtrSerializeScope&) = delete;
};
}

#ifdef CXX_PYTHON_RULES_HAVE_BOOST_SERIALIZATION

namespace boost::serialization {

template<class Archive, class T>
inline void save(Archive& ar, const std::weak_ptr<T>& t,
                 const unsigned int /* file_version */
)
{
  std::shared_ptr<T> sp = t.lock();
  const CXX_PYTHON_RULES_NAMESPACE::GenericObject* ptr = nullptr;
  if(sp) {
    ptr = dynamic_cast<const CXX_PYTHON_RULES_NAMESPACE::GenericObject*>(
        sp.get());
    if(ptr && !CXX_PYTHON_RULES_NAMESPACE::is_ptr_serialized(ptr))
      sp.reset();
  }
  ar << boost::serialization::make_nvp("weak_ptr", sp);
}

template<class Archive, class T>
inline void load(Archive& ar, std::weak_ptr<T>& t,
                 const unsigned int /* file_version */
)
{
  std::shared_ptr<T> sp;
  ar >> boost::serialization::make_nvp("weak_ptr", sp);
  t = sp;
}

template<class Archive, class T>
inline void serialize(Archive& ar, std::weak_ptr<T>& t,
                      const unsigned int file_version)
{
  boost::serialization::split_free(ar, t, file_version);
}
}

#endif
