#ifndef GENERIC_OBJECT_SAMPLE_H
#define GENERIC_OBJECT_SAMPLE_H
#include "printable.h"
#include <memory>
#include <utility>

namespace Skeleton {
/****************************************************************/ /**
  Simple set of classes for testing serialization support in python
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

}
SKELETON_EXPORT_KEY(GenericObjectA);
SKELETON_EXPORT_KEY(GenericObjectB);
SKELETON_EXPORT_KEY(GenericObjectB2);
SKELETON_EXPORT_KEY(GenericObjectC);
#endif
