#pragma once
#include "observer.h"
#include "skeleton_serialize_support.h"

//-----------------------------------------------------------------------
/// Serialization of Observer<NAME> and Observable<NAME>, for the .cc
/// file of NAME (framework's FP_OBSERVER_SERIALIZE). An Observer writes
/// only a placeholder, but marks itself as serialized, so the weak_ptr
/// to it in an Observable's olist is kept (two pass rule, see
/// weak_ptr_serialize_support.h). The Observable writes its lists. On
// load it merges them with observers that registered themselves while
// the lists were being read (Observable::merge_loaded); framework's
// FP_OBSERVER_SERIALIZE loads straight into olist, and gets such an
// observer twice.
//-----------------------------------------------------------------------

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
#define SKELETON_OBSERVER_SERIALIZE(NAME)                                      \
  template<>                                                                   \
  template<class Archive>                                                      \
  void Observer##NAME::serialize(Archive& ar, const unsigned int version)      \
  {                                                                            \
    SKELETON_GENERIC_BASE(Observer##NAME);                                     \
    std::string p = "empty";                                                   \
    ar& SKELETON_NVP2("placeholder", p);                                       \
    boost::serialization::split_member(ar, *this, version);                    \
  }                                                                            \
  template<>                                                                   \
  template<class Archive>                                                      \
  void Observer##NAME::save(Archive&, const unsigned int) const                \
  {                                                                            \
    add_ptr_serialized_reference(this);                                        \
  }                                                                            \
  template<>                                                                   \
  template<class Archive>                                                      \
  void Observer##NAME::load(Archive&, const unsigned int)                      \
  {                                                                            \
  }                                                                            \
  template<>                                                                   \
  template<class Archive>                                                      \
  void Observable##NAME::serialize(Archive& ar, const unsigned int version)    \
  {                                                                            \
    SKELETON_GENERIC_BASE(Observable##NAME);                                   \
    boost::serialization::split_member(ar, *this, version);                    \
  }                                                                            \
  template<>                                                                   \
  template<class Archive>                                                      \
  void Observable##NAME::save(Archive& ar, const unsigned int) const           \
  {                                                                            \
    ar << SKELETON_NVP(olist) << SKELETON_NVP(colist)                          \
       << SKELETON_NVP(ref_list);                                              \
  }                                                                            \
  template<>                                                                   \
  template<class Archive>                                                      \
  void Observable##NAME::load(Archive& ar, const unsigned int)                 \
  {                                                                            \
    decltype(olist) loaded_olist;                                              \
    decltype(colist) loaded_colist;                                            \
    ar >> SKELETON_NVP2("olist", loaded_olist) >>                              \
        SKELETON_NVP2("colist", loaded_colist) >> SKELETON_NVP(ref_list);      \
    merge_loaded(loaded_olist, loaded_colist);                                 \
  }                                                                            \
  SKELETON_IMPLEMENT(Observer##NAME);                                          \
  SKELETON_IMPLEMENT(Observable##NAME);
#else
#define SKELETON_OBSERVER_SERIALIZE(NAME) /* Noop */
#endif
