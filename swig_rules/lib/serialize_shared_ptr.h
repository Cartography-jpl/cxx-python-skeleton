#include <iostream>
#include <memory>
#include "director_hook.h"
// Workaround for a Boost.Serialization bug (boostorg/serialization issue
// #319, still in Boost 1.90): shared_ptr_helper.hpp declares, at namespace
// scope,
//   template<class Archive, template<class U> class SPT>
//   void load(Archive&, SPT< class U >&, const unsigned int);
// where "SPT< class U >" declares an incomplete class
// boost::serialization::U. Loading a std::optional<T> then considers this
// load() too, which instantiates std::optional<U>, a hard error with gcc
// 15's libstdc++. With BOOST_NO_MEMBER_TEMPLATE_FRIENDS Boost leaves that
// declaration out (and makes shared_ptr_helper's members public instead of
// befriending that load()). So define it just for this include. This header
// must be the first to include shared_ptr_helper.hpp in a translation unit
// (don't include boost/serialization/shared_ptr.hpp; this file replaces it).
#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
#define BOOST_NO_MEMBER_TEMPLATE_FRIENDS
#define SWIG_RULES_UNDEF_NO_MEMBER_TEMPLATE_FRIENDS
#endif
#include <boost/serialization/shared_ptr_helper.hpp>
#ifdef SWIG_RULES_UNDEF_NO_MEMBER_TEMPLATE_FRIENDS
#undef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
#undef SWIG_RULES_UNDEF_NO_MEMBER_TEMPLATE_FRIENDS
#endif
#ifdef SWIG_HAVE_BOOST_SERIALIZATION
#include <boost/archive/polymorphic_iarchive.hpp>
#include <boost/archive/polymorphic_oarchive.hpp>
#endif

#ifdef SWIG_HAVE_BOOST_SERIALIZATION
//-----------------------------------------------------------------------
/// This is largely a duplicate of boost/serialization/shared_ptr.hpp
/// (since phase S4 for std::shared_ptr; the archive format is the same
/// as for boost::shared_ptr: version 1, not tracked, the pointer as
/// "px", so archives written with boost::shared_ptr still load and
/// writing again gives the same bytes).
/// We need to have our own copy to add support for python subclasses
/// of C++ classes (director_hook.h).
//-----------------------------------------------------------------------

namespace boost {
  namespace serialization{
template<class T>
struct version< ::std::shared_ptr< T > > {
  typedef mpl::integral_c_tag tag;
  typedef mpl::int_<1> type;
  static const int value = type::value;
};
// don't track shared pointers
template<class T>
struct tracking_level< ::std::shared_ptr< T > > {
  typedef mpl::integral_c_tag tag;
  typedef mpl::int_< ::boost::serialization::track_never> type;
  static const int value = type::value;
};

void * const shared_ptr_helper_id = nullptr;

struct null_deleter {
    void operator()(void const *) const {}
};

//-----------------------------------------------------------------------
/// MMS    
/// We actually want to modify shared_ptr_helper, but this is a bit
/// involved code and copying it seemed like it would be fragile. So
/// instead we add a special version of the shared_ptr, but just adds
/// its own version of reset and other functions used by
/// shared_ptr_helper. This pretty much is just a normal shared_ptr,
/// but adds special handling for the pointer passed to reset.
//-----------------------------------------------------------------------

template<class T> class our_shared_ptr : public std::shared_ptr<T>
{
public:
  typedef typename std::shared_ptr< T >::element_type element_type;
  template< class Y >
  our_shared_ptr(our_shared_ptr<Y> const & r, element_type * p )
    : std::shared_ptr<T>(r, p)
  {
  }
  template< class Y >
  our_shared_ptr(our_shared_ptr<Y> const & r)
    : std::shared_ptr<T>(r)
  {
  }
  our_shared_ptr() = default;
  void reset() noexcept
  {
    std::shared_ptr<T>::reset();
  }
  void reset(T* r)
  {
    // We'll put in a proper deleter shortly. Made with the aliasing
    // constructor from a shared_ptr<void>, so a class with
    // enable_shared_from_this doesn't take this non-owning pointer as its
    // weak_this (see to_ptr in null_deleter.h).
    std::shared_ptr<T> t2(
      std::shared_ptr<void>(const_cast<void*>(static_cast<const void*>(r)),
                            null_deleter()), r);
    std::shared_ptr<SWIG_MAPPER_NAMESPACE::GenericObject> v2 =
      std::dynamic_pointer_cast<SWIG_MAPPER_NAMESPACE::GenericObject>(t2);
    std::shared_ptr<SWIG_MAPPER_NAMESPACE::GenericObject> v3;
    if(v2 && SWIG_MAPPER_NAMESPACE::director_hook())
      v3 = SWIG_MAPPER_NAMESPACE::director_hook()(v2);
    if(v3) {
      std::shared_ptr<T>::operator=(std::dynamic_pointer_cast<T>(v3));
    } else {
      std::shared_ptr<T>::reset(r);
    }
  }
};
  
template<class Archive, class T>
inline void save(
		 Archive & ar,
		 const std::shared_ptr< T > &t,
		 const unsigned int /* file_version */
		 ){
  // The most common cause of trapping here would be serializing
  // something like shared_ptr<int>.  This occurs because int
  // is never tracked by default.  Wrap int in a trackable type
  BOOST_STATIC_ASSERT((tracking_level< T >::value != track_never));
  const T * t_ptr = t.get();
  ar << boost::serialization::make_nvp("px", t_ptr);
}

template<class Archive, class T>
inline void load(
    Archive & ar,
    std::shared_ptr< T > &t,
    const unsigned int /*file_version*/
){
  // The most common cause of trapping here would be serializing
  // something like shared_ptr<int>.  This occurs because int
  // is never tracked by default.  Wrap int in a trackable type
  BOOST_STATIC_ASSERT((tracking_level< T >::value != track_never));
  T* r;
  ar >> boost::serialization::make_nvp("px", r);
  /// MMS, use our_shared_ptr to get the proper handling of
  /// PythonRefPtrCleanup in place.
  boost::serialization::shared_ptr_helper<our_shared_ptr> & h =
    ar.template get_helper<shared_ptr_helper<our_shared_ptr> >(
            shared_ptr_helper_id);
  our_shared_ptr<T> t2;
  h.reset(t2,r);
  t = t2;
}
    
template<class Archive, class T>
inline void serialize(
    Archive & ar,
    std::shared_ptr< T > &t,
    const unsigned int file_version
){
    // correct shared_ptr serialization depends upon object tracking
    // being used.
    BOOST_STATIC_ASSERT(
        boost::serialization::tracking_level< T >::value
        != boost::serialization::track_never
    );
    boost::serialization::split_free(ar, t, file_version);
}
    
  }
}
#endif
