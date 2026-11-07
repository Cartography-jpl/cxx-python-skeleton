#pragma once
#include "printable.h"
#include "skeleton_exception.h"
#include <memory>
#include <utility>
#include <vector>

namespace Skeleton {
/****************************************************************/ /**
  This is a simple class that we can use to test having a derived
  class in python to override the director functions.
*******************************************************************/

class DirectorExample : public Printable<DirectorExample> {
public:
  //-----------------------------------------------------------------------
  /// Constructor
  //-----------------------------------------------------------------------

  DirectorExample(int value_to_add, std::string name)
    : value_to_add_(value_to_add), name_(std::move(name))
  {
  }
  virtual int func(int v) const { return v + value_to_add_; }
  virtual void print(std::ostream& Os) const;
  int value_to_add() const { return value_to_add_; }
  void value_to_add(int v) { value_to_add_ = v; }
  const std::string& name() const { return name_; }
  bool same_object(const DirectorExample& obj2) const { return this == &obj2; }
protected:
  // For the python bindings: reading a python subclass from an archive
  // constructs the trampoline with this (CXX_PYTHON_RULES_NB_DIRECTOR_SERIALIZATION)
  DirectorExample() = default;
private:
  int value_to_add_ = 0;
  std::string name_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  A version used for nesting DirectorExample.
*******************************************************************/

class DirectorExampleNested : public DirectorExample {
public:
  //-----------------------------------------------------------------------
  /// Constructor
  //-----------------------------------------------------------------------

  DirectorExampleNested(int value_to_add, const std::string& name,
                        std::shared_ptr<DirectorExample> Obj)
    : DirectorExample(value_to_add, name), obj_(std::move(Obj))
  {
  }

  const std::shared_ptr<DirectorExample>& obj() const { return obj_; }
protected:
  DirectorExampleNested() = default;
private:
  std::shared_ptr<DirectorExample> obj_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  A weak pointer to a DirectorExample, used to test proper handling of
  weak pointers and also pointer tracking in the serialization code.
*******************************************************************/
class DirectorExampleWeakPtr : public Printable<DirectorExampleWeakPtr> {
public:
  explicit DirectorExampleWeakPtr(const std::shared_ptr<DirectorExample>& V)
    : a_(V)
  {
  }
  virtual void print(std::ostream& Os) const
  {
    Os << "----------------------\n";
    Os << "DirectorExampleWeakPtr\n";
    if(a())
      Os << *a();
    else
      Os << "None";
    Os << "\n----------------------\n";
  }
  std::shared_ptr<DirectorExample> a() const { return a_.lock(); }
  void a(const std::shared_ptr<DirectorExample>& V) { a_ = V; }
  bool expired() const { return a_.expired(); }
  long use_count() const { return a_.use_count(); }
private:
  std::weak_ptr<DirectorExample> a_;
  DirectorExampleWeakPtr() = default;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  Sample class to use DirectorExample, can use to make sure python
  can be passed correctly.
*******************************************************************/
class DirectorExampleUser : public Printable<DirectorExampleUser> {
public:
  DirectorExampleUser() = default;
  virtual void print(std::ostream& Os) const { Os << "DirectorExampleUser"; }
  const std::vector<std::shared_ptr<DirectorExample>>& value_vec() const
  {
    return value_vec_;
  }
  void set_vec(const std::vector<std::shared_ptr<DirectorExample>>& V)
  {
    value_vec_ = V;
  }
  const std::vector<std::vector<std::shared_ptr<DirectorExample>>>&
  value_vec_vec() const
  {
    return value_vec2_;
  }
  void set_vec_vec(
      const std::vector<std::vector<std::shared_ptr<DirectorExample>>>& V)
  {
    value_vec2_ = V;
  }
  const std::vector<int>& value_vec_int() const { return t_; }
  void set_vec_int(const std::vector<int>& V) { t_ = V; }
  const std::vector<std::vector<int>>& value_vec_vec_int() const { return t2_; }
  void set_vec_vec_int(const std::vector<std::vector<int>>& V) { t2_ = V; }
  const std::shared_ptr<DirectorExample>& value() const { return value_; }
  void set_value(const std::shared_ptr<DirectorExample>& V) { value_ = V; }
  //-----------------------------------------------------------------------
  /// Call func(v) on value(), value_vec()[ind] or value_vec_vec()[ind][ind2].
  /// The indices come from python, so they are checked (std::out_of_range),
  /// and so is a null pointer.
  //-----------------------------------------------------------------------

  int apply_func(int v) const { return checked(value_).func(v); }
  int apply_func(int v, int ind) const
  {
    return checked(value_vec_.at(static_cast<std::size_t>(ind))).func(v);
  }
  int apply_func(int v, int ind, int ind2) const
  {
    return checked(value_vec2_.at(static_cast<std::size_t>(ind))
                       .at(static_cast<std::size_t>(ind2)))
        .func(v);
  }
private:
  static const DirectorExample&
  checked(const std::shared_ptr<DirectorExample>& P)
  {
    if(!P)
      throw Exception("DirectorExampleUser: no DirectorExample set");
    return *P;
  }
  std::vector<std::shared_ptr<DirectorExample>> value_vec_;
  std::vector<std::vector<std::shared_ptr<DirectorExample>>> value_vec2_;
  std::shared_ptr<DirectorExample> value_;
  std::vector<int> t_;
  std::vector<std::vector<int>> t2_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

}
SKELETON_EXPORT_KEY(DirectorExample);
SKELETON_EXPORT_KEY(DirectorExampleNested);
SKELETON_EXPORT_KEY(DirectorExampleWeakPtr);
SKELETON_EXPORT_KEY(DirectorExampleUser);
