#ifndef MULTI_BASE_EXAMPLE_H
#define MULTI_BASE_EXAMPLE_H
#include "observer.h"
#include "printable.h"
#include "skeleton_exception.h"
#include "with_parameter_like.h"
#include <memory>
#include <string>

namespace Skeleton {
/****************************************************************/ /**
  Like geocal's Camera (8 such classes there, 52 in framework): a class
  with several bases, a Printable, an Observable and a second base
  class with its own interface (WithParameterLike).
*******************************************************************/
class CameraLike : public Printable<CameraLike>,
                   public Observable<CameraLike>,
                   public WithParameterLike {
public:
  ~CameraLike() override = default;
  void add_observer(Observer<CameraLike>& Obs) override
  {
    add_observer_do(Obs, *this);
  }
  void remove_observer(Observer<CameraLike>& Obs) override
  {
    remove_observer_do(Obs, *this);
  }
  virtual int number_band() const = 0;
  virtual double focal_length() const = 0;
  virtual void print(std::ostream& Os) const = 0;
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  A concrete CameraLike. The parameter is the focal length; setting it
  notifies the observers.
*******************************************************************/
class SimpleCameraLike : public CameraLike {
public:
  SimpleCameraLike(double Focal_length, int Number_band)
    : focal_length_(Focal_length), number_band_(Number_band)
  {
  }
  ~SimpleCameraLike() override = default;
  int number_band() const override { return number_band_; }
  double focal_length() const override { return focal_length_; }
  blitz::Array<double, 1> parameter() const override
  {
    blitz::Array<double, 1> res(1);
    res(0) = focal_length_;
    return res;
  }
  void parameter(const blitz::Array<double, 1>& Parm) override
  {
    if(Parm.rows() != 1)
      throw std::runtime_error("parameter must have size 1");
    focal_length_ = Parm(0);
    notify_update_do(*this);
  }
  void print(std::ostream& Os) const override
  {
    Os << "SimpleCameraLike " << focal_length_ << " " << number_band_;
  }
private:
  SimpleCameraLike() = default;
  double focal_length_;
  int number_band_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

/****************************************************************/ /**
  Functions that return a SimpleCameraLike as each of its bases, and
  take each of the bases as an argument. Python should get the most
  derived type back in every case, and be able to pass the derived
  object for any of the base types.
*******************************************************************/
class MultiBaseExample : public Printable<MultiBaseExample> {
public:
  MultiBaseExample(const std::shared_ptr<CameraLike>& Cam) : cam_(Cam) {}
  ~MultiBaseExample() override = default;
  const std::shared_ptr<CameraLike>& camera() const { return cam_; }
  std::shared_ptr<WithParameterLike> camera_as_with_parameter() const
  {
    return cam_;
  }
  std::shared_ptr<GenericObject> camera_as_generic() const { return cam_; }
  std::shared_ptr<Observable<CameraLike>> camera_as_observable() const
  {
    return cam_;
  }
  static int number_band(const std::shared_ptr<CameraLike>& Cam)
  {
    if(!Cam)
      throw Exception("MultiBaseExample::number_band: Cam is null");
    return Cam->number_band();
  }
  static double parameter_sum(const std::shared_ptr<WithParameterLike>& P)
  {
    if(!P)
      throw Exception("MultiBaseExample::parameter_sum: P is null");
    return P->parameter_sum();
  }
  static double parameter_sum_ref(const WithParameterLike& P)
  {
    return P.parameter_sum();
  }
  static std::string type_name(const std::shared_ptr<GenericObject>& G);
  virtual void print(std::ostream& Os) const { Os << "MultiBaseExample"; }
private:
  std::shared_ptr<CameraLike> cam_;
};
}

SKELETON_EXPORT_OBSERVER_KEY(CameraLike);
SKELETON_EXPORT_KEY(CameraLike);
SKELETON_EXPORT_KEY(SimpleCameraLike);
#endif
