#ifndef VIRTUAL_DIAMOND_EXAMPLE_H
#define VIRTUAL_DIAMOND_EXAMPLE_H
#include "printable.h"
#include "with_parameter_like.h"
#include <memory>
#include <string>

namespace Skeleton {
/****************************************************************/ /**
  A second level virtual base below WithParameterLike, like geocal's
  WithParameterNested.
*******************************************************************/
class NestedLike : public virtual WithParameterLike {
public:
  ~NestedLike() override = default;
  virtual std::string nested_name() const { return "NestedLike"; }
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  The other side of the diamond, like geocal's ImageGroundConnection.
*******************************************************************/
class RootLike : public virtual GenericObject {
public:
  ~RootLike() override = default;
  virtual std::string root_name() const = 0;
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  A virtual diamond, like geocal's IgcRollingShutter and IgcArray, or
  framework's classes with virtual public StateVectorObserver.
  GenericObject is reached through RootLike, NestedLike ->
  WithParameterLike and Printable, all virtual.
*******************************************************************/
class Diamond : public virtual RootLike,
                public virtual NestedLike,
                public Printable<Diamond> {
public:
  Diamond(const std::string& Name, double P0, double P1) : name_(Name), p_(2)
  {
    p_ = P0, P1;
  }
  ~Diamond() override = default;
  std::string root_name() const override { return "Diamond " + name_; }
  std::string nested_name() const override { return "Diamond nested " + name_; }
  blitz::Array<double, 1> parameter() const override { return p_.copy(); }
  void parameter(const blitz::Array<double, 1>& Parm) override
  {
    p_.reference(Parm.copy());
  }
  const std::string& name() const { return name_; }
  virtual void print(std::ostream& Os) const { Os << "Diamond " << name_; }
private:
  Diamond() = default;
  std::string name_;
  blitz::Array<double, 1> p_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  Return a Diamond as each of its bases, and take each base as an
  argument.
*******************************************************************/
class VirtualDiamondExample : public Printable<VirtualDiamondExample> {
public:
  VirtualDiamondExample(const std::shared_ptr<Diamond>& D) : d_(D) {}
  ~VirtualDiamondExample() override = default;
  const std::shared_ptr<Diamond>& diamond() const { return d_; }
  std::shared_ptr<RootLike> as_root() const { return d_; }
  std::shared_ptr<NestedLike> as_nested() const { return d_; }
  std::shared_ptr<WithParameterLike> as_with_parameter() const { return d_; }
  std::shared_ptr<GenericObject> as_generic() const { return d_; }
  static std::string root_name(const std::shared_ptr<RootLike>& R)
  {
    return R->root_name();
  }
  static std::string nested_name(const std::shared_ptr<NestedLike>& N)
  {
    return N->nested_name();
  }
  static double parameter_sum(const std::shared_ptr<WithParameterLike>& P)
  {
    return P->parameter_sum();
  }
  virtual void print(std::ostream& Os) const { Os << "VirtualDiamondExample"; }
private:
  std::shared_ptr<Diamond> d_;
};
}

SKELETON_EXPORT_KEY(NestedLike);
SKELETON_EXPORT_KEY(RootLike);
SKELETON_EXPORT_KEY(Diamond);
#endif
