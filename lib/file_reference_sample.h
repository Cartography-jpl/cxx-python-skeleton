#ifndef FILE_REFERENCE_SAMPLE_H
#define FILE_REFERENCE_SAMPLE_H
#include "printable.h"
#include "generic_object_sample.h"
#include <memory>
#include <fstream>
#include <vector>

namespace Skeleton {
/****************************************************************/ /**
  A class that refers to a file by a (possibly relative) name, and
  reads it when created or loaded. This is like geocal's AirMspiIgc,
  which uses %pickle_serialization_dir so a pickle can be loaded from
  a different directory than the one it was made in (see
  cxx_python_rules_sample_test.py).
*******************************************************************/
class FileReferenceSample : public Printable<FileReferenceSample> {
public:
  FileReferenceSample(const std::string& Fname) : fname_(Fname) { read(); }
  ~FileReferenceSample() override = default;
  const std::string& file_name() const { return fname_; }
  const std::string& contents() const { return contents_; }
  virtual void print(std::ostream& Os) const
  {
    Os << "FileReferenceSample(" << fname_ << ")";
  }
private:
  void read()
  {
    std::ifstream in(fname_);
    if(!in)
      throw std::runtime_error("Can't open " + fname_);
    std::getline(in, contents_);
  }
  std::string fname_;
  std::string contents_;
  FileReferenceSample() = default;
  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive& ar, const unsigned int version) const;
  template<class Archive> void load(Archive& ar, const unsigned int version);
  SKELETON_SPLIT_MEMBER();
};

//-----------------------------------------------------------------------
/// Return each sample object as a shared_ptr<GenericObject>, so python
/// can check it gets the most derived type back (the downcast done by
/// cxx_python_rules' type mapper).
//-----------------------------------------------------------------------

inline std::vector<std::shared_ptr<GenericObject>> generic_object_list()
{
  std::vector<std::shared_ptr<GenericObject>> res;
  auto a = std::make_shared<GenericObjectA>(1);
  res.push_back(a);
  res.push_back(std::make_shared<GenericObjectB>(a));
  res.push_back(std::make_shared<GenericObjectB2>(*a));
  res.push_back(std::make_shared<GenericObjectC>(a));
  return res;
}

inline std::shared_ptr<GenericObject> generic_object_index(int I)
{
  return generic_object_list()[I];
}
}

SKELETON_EXPORT_KEY(FileReferenceSample);
#endif
