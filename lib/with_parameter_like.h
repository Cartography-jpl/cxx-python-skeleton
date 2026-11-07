#pragma once
#include "generic_object.h"
#include <blitz/array.h>

namespace Skeleton {
/****************************************************************/ /**
  Like geocal's WithParameter: a second (non-primary) base class with
  pure virtual functions for getting and setting parameters, plus a
  non-virtual function using them. Used by MultiBaseExample and
  VirtualDiamondExample.
*******************************************************************/
class WithParameterLike : public virtual GenericObject {
public:
  virtual blitz::Array<double, 1> parameter() const = 0;
  virtual void parameter(const blitz::Array<double, 1>& Parm) = 0;

  //-----------------------------------------------------------------------
  /// Sum of the parameters (a non-virtual function of this base).
  //-----------------------------------------------------------------------

  double parameter_sum() const { return blitz::sum(parameter()); }
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};
}

SKELETON_EXPORT_KEY(WithParameterLike);
