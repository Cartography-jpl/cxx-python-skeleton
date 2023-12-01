#ifndef DIRECTOR_EXAMPLE_H
#define DIRECTOR_EXAMPLE_H
#include "printable.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <iostream>

namespace Skeleton {
/****************************************************************//**
  This is a simple class that we can use to test having a derived
  class in python to override the director functions.
*******************************************************************/

class DirectorExample : public Printable<DirectorExample> {
public:
//-----------------------------------------------------------------------
/// Constructor
//-----------------------------------------------------------------------

  DirectorExample(int value_to_add, const std::string& name)
    : value_to_add_(value_to_add),
      name_(name)
  {
    std::cerr << "Creating C++ " << name_ << "\n";
  }
  virtual ~DirectorExample()
  {
    std::cerr << "Destroying C++ " << name_ << "\n";
  }
  virtual int func(int v) const { return v + value_to_add_; }
  virtual void print(std::ostream& Os) const;
  int value_to_add() const { return value_to_add_;}
  const std::string& name() const { return name_;}
protected:
  // Directors with swig seems to need to have this available
  DirectorExample() {}
private:
  int value_to_add_;
  std::string name_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version);
};

/****************************************************************//**
  Sample class to use DirectorExample, can use to make sure python
  can be passed correctly.
*******************************************************************/
class DirectorExampleUser : public Printable<DirectorExampleUser> {
public:
  DirectorExampleUser()
  {
    std::cerr << "Creating DirectorExampleUser\n";
  }
  virtual ~DirectorExampleUser()
  {
    std::cerr << "Destroying DirectorExampleUser\n";
  }
  virtual void print(std::ostream& Os) const { Os << "DirectorExampleUser"; }
  const std::vector<boost::shared_ptr<DirectorExample> >&
  value_vec() const { return value_vec_; }
  void set_vec(const std::vector<boost::shared_ptr<DirectorExample> >& V)
  {value_vec_ = V; }
  const std::vector<int>&
  value_vec_int() const { return t_; }
  void set_vec_int(const std::vector<int>& V)
  {t_ = V; }
  const boost::shared_ptr<DirectorExample>&
  value() const { return value_; }
  void set_value(const boost::shared_ptr<DirectorExample>& V)
  {value_ = V; }
  int apply_func(int v) const { return value_->func(v);}
  int apply_func(int v, int ind) const { return value_vec_[ind]->func(v);}
private:
  std::vector<boost::shared_ptr<DirectorExample> >  value_vec_;
  boost::shared_ptr<DirectorExample> value_;
  std::vector<int> t_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version);
};
  
}
SKELETON_EXPORT_KEY(DirectorExample);
SKELETON_EXPORT_KEY(DirectorExampleUser);
#endif

