// Python bindings for CameraLike, SimpleCameraLike and MultiBaseExample
// (multi_base_example.h): a class with several bases (Printable,
// Observable<CameraLike>, WithParameterLike), returned as and passed as
// each of them.
#include "observer_nb.h"
#include "multi_base_example.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

void init_multi_base_example(nb::module_& m)
{
  nb_observer::bind_observable<CameraLike>(
      m, "ObservableCameraLike", "Skeleton::Observable<Skeleton::CameraLike>");
  nb_observer::bind_observer<CameraLike>(
      m, "ObserverCameraLike", "Skeleton::Observer<Skeleton::CameraLike>");
  {
    // The nanobind base is Observable<CameraLike>; WithParameterLike's
    // methods are bound here directly.
    const auto doc = cxx_python_rules_nb::docs("Skeleton::CameraLike");
    nb::class_<CameraLike, Observable<CameraLike>> c(m, "CameraLike", doc());
    c.def("add_observer", &CameraLike::add_observer, "Obs"_a,
          doc("add_observer"));
    c.def("remove_observer", &CameraLike::remove_observer, "Obs"_a,
          doc("remove_observer"));
    c.def("parameter_sum", &CameraLike::parameter_sum, doc("parameter_sum"));
    c.def_prop_ro("number_band", &CameraLike::number_band, doc("number_band"));
    c.def_prop_ro("focal_length", &CameraLike::focal_length,
                  doc("focal_length"));
    // The parameter property through the _v_parameter methods, which a
    // python subclass can override.
    c.def("_v_parameter",
          nb::overload_cast<>(&CameraLike::parameter, nb::const_));
    c.def("_v_parameter",
          nb::overload_cast<const blitz::Array<double, 1>&>(
              &CameraLike::parameter),
          "Parm"_a);
    c.def_prop_rw(
        "parameter",
        [](nb::handle self) { return self.attr("_v_parameter")(); },
        [](nb::handle self, nb::handle v) { self.attr("_v_parameter")(v); },
        doc("parameter"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::register_type<CameraLike>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::SimpleCameraLike");
    nb::class_<SimpleCameraLike, CameraLike> c(m, "SimpleCameraLike", doc());
    c.def(nb::new_([](double Focal_length, int Number_band) {
            return std::make_shared<SimpleCameraLike>(Focal_length,
                                                      Number_band);
          }),
          "Focal_length"_a, "Number_band"_a);
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::pickle_serialization(c);
    cxx_python_rules_nb::register_type<SimpleCameraLike>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::MultiBaseExample");
    nb::class_<MultiBaseExample> c(m, "MultiBaseExample", doc());
    c.def(nb::new_([](const std::shared_ptr<CameraLike>& Cam) {
            return std::make_shared<MultiBaseExample>(Cam);
          }),
          "Cam"_a.none());
    c.def("camera_as_with_parameter",
          &MultiBaseExample::camera_as_with_parameter,
          doc("camera_as_with_parameter"));
    c.def("camera_as_generic", &MultiBaseExample::camera_as_generic,
          doc("camera_as_generic"));
    c.def("camera_as_observable", &MultiBaseExample::camera_as_observable,
          doc("camera_as_observable"));
    c.def_static("number_band", &MultiBaseExample::number_band, "Cam"_a,
                 doc("number_band"));
    c.def_static("parameter_sum", &MultiBaseExample::parameter_sum, "P"_a,
                 doc("parameter_sum"));
    // A reference to a secondary base: taken as a shared_ptr, which the
    // type registry converts for any class derived from it.
    c.def_static(
        "parameter_sum_ref",
        [](const std::shared_ptr<WithParameterLike>& P) {
          return MultiBaseExample::parameter_sum_ref(*P);
        },
        "P"_a, doc("parameter_sum_ref"));
    c.def_static("type_name", &MultiBaseExample::type_name, "G"_a.none(),
                 doc("type_name"));
    c.def_prop_ro("camera", &MultiBaseExample::camera, doc("camera"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::register_type<MultiBaseExample>();
  }
}
