#include "director_example.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

template<class Archive>
void DirectorExample::serialize(Archive & ar, const unsigned int )
{
  SKELETON_GENERIC_BASE(DirectorExample);
  ar & SKELETON_NVP_(value_to_add) & SKELETON_NVP_(name);
  add_ptr_serialized_reference(this);
}

template<class Archive>
void DirectorExampleUser::serialize(Archive & ar, const unsigned int )
{
  SKELETON_GENERIC_BASE(DirectorExampleUser);
  ar & SKELETON_NVP_(value_vec) & SKELETON_NVP_(value);
}

SKELETON_IMPLEMENT(DirectorExample);
SKELETON_IMPLEMENT(DirectorExampleUser);

void DirectorExample::print(std::ostream& Os) const
{
  Os << "DirectorExample:\n"
     << "  name:         " << name_
     << "  value_to_add: " << value_to_add_;
}
