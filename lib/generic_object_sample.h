#ifndef GENERIC_OBJECT_SAMPLE_H
#define GENERIC_OBJECT_SAMPLE_H
#include "printable.h"
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace Skeleton {
/****************************************************************//**
  Simple set of classes for testing serialization support in python
*******************************************************************/
class GenericObjectA : public Printable<GenericObjectA> {
public:
  GenericObjectA(int V) : v_(V) {}
  GenericObjectA() {}
  void print(std::ostream& Os) const {Os << "A";}
  int v() const { return v_;}
  void v(int V) { v_ = V;}
private:
  int v_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version);
};

class GenericObjectB : public Printable<GenericObjectB> {
public:
  GenericObjectB(const boost::shared_ptr<GenericObjectA>& V) : a_(V) {}
  void print(std::ostream& Os) const {Os << "B";}
  const boost::shared_ptr<GenericObjectA>& a() const { return a_;}
  void a(const boost::shared_ptr<GenericObjectA>& V) { a_ = V;}
private:
  boost::shared_ptr<GenericObjectA> a_;
  GenericObjectB() {}
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version);
};

class GenericObjectB2 : public Printable<GenericObjectB2> {
public:
  GenericObjectB2(const GenericObjectA& V) : a_(V) {}
  void print(std::ostream& Os) const {Os << "B2";}
  const GenericObjectA& a() const { return a_;}
  void a(const GenericObjectA& V) { a_ = V;}
private:
  GenericObjectA a_;
  GenericObjectB2() {}
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version);
};
  
class GenericObjectC : public Printable<GenericObjectC> {
public:
  GenericObjectC(const boost::shared_ptr<GenericObjectA>& V) : a_(V) {}
  void print(std::ostream& Os) const {Os << "C";}
  boost::shared_ptr<GenericObjectA> a() const { return a_.lock();}
  void a(const boost::shared_ptr<GenericObjectA>& V) { a_ = V;}
private:
  boost::weak_ptr<GenericObjectA> a_;
  GenericObjectC() {}
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version);
};
  
}
SKELETON_EXPORT_KEY(GenericObjectA);
SKELETON_EXPORT_KEY(GenericObjectB);
SKELETON_EXPORT_KEY(GenericObjectB2);
SKELETON_EXPORT_KEY(GenericObjectC);
#endif
