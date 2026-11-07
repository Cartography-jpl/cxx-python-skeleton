#pragma once
#include "printable.h"
#include "skeleton_exception.h"
#include "serialize_function.h"
#include <memory>
#include <utility>
#include <fstream>

namespace Skeleton {
/****************************************************************/ /**
  A class that refers to a file by a (possibly relative) name, and
  reads it when created or loaded. This is like geocal's AirMspiIgc,
  which uses pickle_serialization_dir so a pickle can be loaded from
  a different directory than the one it was made in (see
  cxx_python_rules_sample_test.py).

  Thread safety: as for a standard library type: const member functions
  can be called from several threads at once; anything else needs
  exclusive access.
*******************************************************************/
class FileReferenceSample : public Printable<FileReferenceSample> {
public:
  explicit FileReferenceSample(std::string Fname) : fname_(std::move(Fname))
  {
    read();
  }
  const std::string& file_name() const { return fname_; }
  const std::string& contents() const { return contents_; }
  virtual void print(std::ostream& Os) const
  {
    Os << "FileReferenceSample(" << fname_ << ")";
  }
private:
  // A relative name is relative to the archive's directory while an
  // archive is being read (resolve_archive_path), and to the current
  // directory otherwise.
  void read()
  {
    std::filesystem::path p = resolve_archive_path(fname_);
    std::ifstream in(p);
    if(!in)
      throw Exception("Can't open {} (looked for {})", fname_, p.string());
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

}

SKELETON_EXPORT_KEY(FileReferenceSample);
