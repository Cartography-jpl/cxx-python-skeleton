#pragma once
#include "printable.h"
#include <memory>
#include <utility>
#include <vector>

namespace Skeleton {
/****************************************************************/ /**
  Simple set of classes for testing serialization support in python

  Thread safety (all four classes): as for a standard library type: const member functions
  can be called from several threads at once; anything else needs
  exclusive access.
*******************************************************************/
class GenericObjectA : public Printable<GenericObjectA> {
public:
  explicit GenericObjectA(int V) : v_(V) {}
  GenericObjectA() = default;
  virtual void print(std::ostream& Os) const { Os << "A"; }
  int v() const { return v_; }
  void v(int V) { v_ = V; }
private:
  int v_ = 0;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

class GenericObjectB : public Printable<GenericObjectB> {
public:
  explicit GenericObjectB(std::shared_ptr<GenericObjectA> V) : a_(std::move(V))
  {
  }
  virtual void print(std::ostream& Os) const { Os << "B"; }
  const std::shared_ptr<GenericObjectA>& a() const { return a_; }
  void a(const std::shared_ptr<GenericObjectA>& V) { a_ = V; }
private:
  std::shared_ptr<GenericObjectA> a_;
  GenericObjectB() = default;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

class GenericObjectB2 : public Printable<GenericObjectB2> {
public:
  explicit GenericObjectB2(GenericObjectA V) : a_(std::move(V)) {}
  virtual void print(std::ostream& Os) const { Os << "B2"; }
  const GenericObjectA& a() const { return a_; }
  void a(const GenericObjectA& V) { a_ = V; }
private:
  GenericObjectA a_;
  GenericObjectB2() = default;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

class GenericObjectC : public Printable<GenericObjectC> {
public:
  explicit GenericObjectC(const std::shared_ptr<GenericObjectA>& V) : a_(V) {}
  virtual void print(std::ostream& Os) const { Os << "C"; }
  std::shared_ptr<GenericObjectA> a() const { return a_.lock(); }
  void a(const std::shared_ptr<GenericObjectA>& V) { a_ = V; }
private:
  std::weak_ptr<GenericObjectA> a_;
  GenericObjectC() = default;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
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
  // I comes from python, so check it (std::out_of_range).
  return generic_object_list().at(static_cast<std::size_t>(I));
}
}

SKELETON_EXPORT_KEY(GenericObjectA);
SKELETON_EXPORT_KEY(GenericObjectB);
SKELETON_EXPORT_KEY(GenericObjectB2);
SKELETON_EXPORT_KEY(GenericObjectC);
