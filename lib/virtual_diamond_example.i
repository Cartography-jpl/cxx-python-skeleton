%include "skeleton_common.i"

%{
#include "virtual_diamond_example.h"
%}
%base_import(generic_object)
%base_import(with_parameter_like)

%skeleton_shared_ptr(Skeleton::NestedLike);
%skeleton_shared_ptr(Skeleton::RootLike);
%skeleton_shared_ptr(Skeleton::Diamond);
%skeleton_shared_ptr(Skeleton::VirtualDiamondExample);

// Abstract (the pure virtual functions aren't all declared here)
%nodefaultctor Skeleton::NestedLike;
%nodefaultctor Skeleton::RootLike;

namespace Skeleton {
class NestedLike : public WithParameterLike {
public:
  virtual std::string nested_name() const;
};

class RootLike : public GenericObject {
public:
  virtual std::string root_name() const = 0;
};

class Diamond : public RootLike, public NestedLike {
public:
  Diamond(const std::string& Name, double P0, double P1);
  virtual std::string root_name() const;
  %python_attribute(name, std::string);
  std::string print_to_string() const;
  %pickle_serialization();
};

class VirtualDiamondExample : public GenericObject {
public:
  VirtualDiamondExample(const boost::shared_ptr<Diamond>& D);
  %python_attribute(diamond, boost::shared_ptr<Diamond>);
  boost::shared_ptr<RootLike> as_root() const;
  boost::shared_ptr<NestedLike> as_nested() const;
  boost::shared_ptr<WithParameterLike> as_with_parameter() const;
  boost::shared_ptr<GenericObject> as_generic() const;
  static std::string root_name(const boost::shared_ptr<RootLike>& R);
  static std::string nested_name(const boost::shared_ptr<NestedLike>& N);
  static double parameter_sum(const boost::shared_ptr<WithParameterLike>& P);
  std::string print_to_string() const;
};
}

// List of things "import *" will include
%python_export("NestedLike", "RootLike", "Diamond", "VirtualDiamondExample")
