%include "skeleton_common.i"

%{
#include "multi_base_example.h"
%}
%base_import(generic_object)
%base_import(observer)
%base_import(with_parameter_like)

%skeleton_shared_ptr(Skeleton::CameraLike);
%skeleton_shared_ptr(Skeleton::SimpleCameraLike);
%skeleton_shared_ptr(Skeleton::MultiBaseExample);
namespace Skeleton {
  class CameraLike;
}
%skeleton_shared_ptr(Skeleton::Observable<Skeleton::CameraLike>);
%skeleton_shared_ptr(Skeleton::Observer<Skeleton::CameraLike>);

namespace Skeleton {
%feature("director") Observer<CameraLike>;
%template(ObservableCameraLike) Skeleton::Observable<Skeleton::CameraLike>;
%template(ObserverCameraLike) Skeleton::Observer<Skeleton::CameraLike>;

// Abstract (the pure virtual functions aren't all declared here)
%nodefaultctor CameraLike;

class CameraLike : public Observable<CameraLike>, public WithParameterLike {
public:
  virtual void add_observer(Observer<CameraLike>& Obs);
  virtual void remove_observer(Observer<CameraLike>& Obs);
  %python_attribute(number_band, virtual int);
  %python_attribute(focal_length, virtual double);
  std::string print_to_string() const;
};

class SimpleCameraLike : public CameraLike {
public:
  SimpleCameraLike(double Focal_length, int Number_band);
  %pickle_serialization();
};

class MultiBaseExample : public GenericObject {
public:
  MultiBaseExample(const boost::shared_ptr<CameraLike>& Cam);
  %python_attribute(camera, boost::shared_ptr<CameraLike>);
  boost::shared_ptr<WithParameterLike> camera_as_with_parameter() const;
  boost::shared_ptr<GenericObject> camera_as_generic() const;
  boost::shared_ptr<Observable<CameraLike> > camera_as_observable() const;
  static int number_band(const boost::shared_ptr<CameraLike>& Cam);
  static double parameter_sum(const boost::shared_ptr<WithParameterLike>& P);
  static double parameter_sum_ref(const WithParameterLike& P);
  static std::string type_name(const boost::shared_ptr<GenericObject>& G);
  std::string print_to_string() const;
};
}

// List of things "import *" will include
%python_export("CameraLike", "SimpleCameraLike", "MultiBaseExample",
               "ObservableCameraLike", "ObserverCameraLike")
