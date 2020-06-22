#include "generic_object_sample.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

template<class Archive>
void GenericObjectA::serialize(Archive & ar, const unsigned int )
{
    SKELETON_GENERIC_BASE(GenericObjectA);
    ar & SKELETON_NVP_(v);
}

template<class Archive>
void GenericObjectB::serialize(Archive & ar, const unsigned int )
{
    SKELETON_GENERIC_BASE(GenericObjectB);
    ar & SKELETON_NVP_(a);
}

SKELETON_IMPLEMENT(GenericObjectA);
SKELETON_IMPLEMENT(GenericObjectB);
