#include "file_reference_sample.h"
#include "skeleton_serialize_support.h"

using namespace Skeleton;

#ifdef SKELETON_HAVE_BOOST_SERIALIZATION
template<class Archive>
void FileReferenceSample::save(Archive& ar, const unsigned int) const
{
  SKELETON_GENERIC_BASE(FileReferenceSample);
  ar & SKELETON_NVP_(fname);
}

// Like geocal's AirMspiIgc, we read the file when loading, so a relative
// file name has to be found relative to the right directory.
template<class Archive>
void FileReferenceSample::load(Archive& ar, const unsigned int)
{
  SKELETON_GENERIC_BASE(FileReferenceSample);
  ar & SKELETON_NVP_(fname);
  read();
}

SKELETON_SPLIT_IMPLEMENT(FileReferenceSample);
#endif
