#include "director_example.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

template<class Archive>
void DirectorExample::serialize(Archive & ar, const unsigned int )
{
  SKELETON_GENERIC_BASE(DirectorExample);
  ar & SKELETON_NVP_(value_to_add) & SKELETON_NVP_(name);
  // Note, this is just here so we can have a weak_ptr to a
  // DirectorExample handled correctly. If you are looking at this as
  // an example, you probably want to do something a little higher
  // order like the Observer/Observable pattern in ReFRACtor rather
  // than directly calling add_ptr_serialized_reference. But for this
  // skeleton of swig, we don't want to put all that code in place so
  // we just do a low level example.
  add_ptr_serialized_reference(this);
}

template<class Archive>
void DirectorExampleUser::serialize(Archive & ar, const unsigned int )
{
  SKELETON_GENERIC_BASE(DirectorExampleUser);
  ar & SKELETON_NVP_(value_vec) & SKELETON_NVP_(value_vec2)
    & SKELETON_NVP_(value) & SKELETON_NVP_(t) & SKELETON_NVP_(t2);
}

template<class Archive>
void DirectorExampleWeakPtr::serialize(Archive & ar, const unsigned int )
{
  SKELETON_GENERIC_BASE(DirectorExampleWeakPtr);
  ar & SKELETON_NVP_(a);
}

SKELETON_IMPLEMENT(DirectorExample);
SKELETON_IMPLEMENT(DirectorExampleUser);
SKELETON_IMPLEMENT(DirectorExampleWeakPtr);

void DirectorExample::print(std::ostream& Os) const
{
  Os << "DirectorExample:\n"
     << "  name:         " << name_
     << "  value_to_add: " << value_to_add_;
}
