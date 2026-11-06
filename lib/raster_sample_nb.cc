// Written by hand (phase S7; no .i file ever existed). Bindings for
// raster_sample.h: the mdspan sample, modeled on geocal's RasterImage and
// CalcRaster.
#include "skeleton_nb_opaque.h"
#include "raster_sample.h"
#include "skeleton_exception.h"
#include <nanobind/ndarray.h>
#include <memory>

namespace nb = nanobind;

namespace {
// CalcRasterSample: python subclasses implement calc() and fill
// self.data, as geocal's python CalcRaster subclasses do.
class PyCalcRasterSample : public Skeleton::CalcRasterSample {
public:
  NB_TRAMPOLINE(Skeleton::CalcRasterSample);
  PyCalcRasterSample(int Number_line, int Number_sample)
    : Skeleton::CalcRasterSample(Number_line, Number_sample)
  {
  }
  void calc(int Lstart, int Sstart) const override
  {
    NB_OVERRIDE_PURE_NAME("calc", calc, Lstart, Sstart);
  }
};

// Reach the protected data_ptr (the usual "publicist": a member pointer
// taken through a derived class has the base class type).
struct CalcPublicist : Skeleton::CalcRasterSample {
  using Skeleton::CalcRasterSample::data_ptr;
};

// A writable numpy array, used as the output of read_into without a
// copy (any non-negative strides). noconvert: a copy would be filled
// and thrown away.
using out_array = nb::ndarray<int, nb::ndim<2>, nb::device::cpu>;

Skeleton::RasterView to_view(const out_array& A)
{
  std::array<std::ptrdiff_t, 2> ext, str;
  for(int i = 0; i < 2; ++i) {
    ext[i] = static_cast<std::ptrdiff_t>(A.shape(i));
    str[i] = static_cast<std::ptrdiff_t>(A.stride(i));
    if(str[i] < 0)
      throw Skeleton::TypeError("read_into: negative strides not supported");
  }
  return Skeleton::RasterView(
      A.data(), Skeleton::RasterView::mapping_type(
                    Skeleton::md::dextents<std::ptrdiff_t, 2>(ext), str));
}
}

void init_raster_sample(nb::module_& m)
{
  {
    // Bases not bound as the nanobind base: GenericObject. Abstract, no
    // constructor.
    nb::class_<Skeleton::RasterSample> c(
        m, "RasterSample", cxx_python_rules_nb::doc("Skeleton::RasterSample"));
    c.def_prop_ro("number_line", [](const Skeleton::RasterSample& self) {
      return self.number_line();
    });
    c.def_prop_ro("number_sample", [](const Skeleton::RasterSample& self) {
      return self.number_sample();
    });
    c.def(
        "read",
        [](const Skeleton::RasterSample& self, int Lstart, int Sstart,
           int Number_line, int Number_sample) {
          return self.read(Lstart, Sstart, Number_line, Number_sample);
        },
        nb::arg("Lstart"), nb::arg("Sstart"), nb::arg("Number_line"),
        nb::arg("Number_sample"),
        cxx_python_rules_nb::doc("Skeleton::RasterSample::read"));
    c.def(
        "read_with_pad",
        [](const Skeleton::RasterSample& self, int Lstart, int Sstart,
           int Number_line, int Number_sample, int Fill_value) {
          return self.read_with_pad(Lstart, Sstart, Number_line, Number_sample,
                                    Fill_value);
        },
        nb::arg("Lstart"), nb::arg("Sstart"), nb::arg("Number_line"),
        nb::arg("Number_sample"), nb::arg("Fill_value") = 0,
        cxx_python_rules_nb::doc("Skeleton::RasterSample::read_with_pad"));
    c.def(
        "read_into",
        [](const Skeleton::RasterSample& self, int Lstart, int Sstart,
           const out_array& Res) {
          self.read_into_checked(Lstart, Sstart, to_view(Res));
        },
        nb::arg("Lstart"), nb::arg("Sstart"), nb::arg("Res").noconvert(),
        "Fill the writable int32 numpy array Res (any strides) with the\n"
        "data starting at Lstart, Sstart; the size read is Res.shape.");
    c.def(
        "write",
        [](Skeleton::RasterSample& self, int Lstart, int Sstart,
           const blitz::Array<int, 2>& V) { self.write(Lstart, Sstart, V); },
        nb::arg("Lstart"), nb::arg("Sstart"), nb::arg("V"),
        cxx_python_rules_nb::doc("Skeleton::RasterSample::write"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::register_type<Skeleton::RasterSample>();
  }
  {
    nb::class_<Skeleton::MemoryRasterSample, Skeleton::RasterSample> c(
        m, "MemoryRasterSample",
        cxx_python_rules_nb::doc("Skeleton::MemoryRasterSample"));
    c.def(nb::new_([](int Number_line, int Number_sample, int Fill_value) {
            return std::make_shared<Skeleton::MemoryRasterSample>(
                Number_line, Number_sample, Fill_value);
          }),
          nb::arg("Number_line"), nb::arg("Number_sample"),
          nb::arg("Fill_value") = 0);
    c.def(nb::new_([](const blitz::Array<int, 2>& Data) {
            return std::make_shared<Skeleton::MemoryRasterSample>(Data);
          }),
          nb::arg("Data"));
    c.def_prop_ro(
        "data",
        [](const Skeleton::MemoryRasterSample& self) -> decltype(auto) {
          return self.data();
        },
        cxx_python_rules_nb::doc("Skeleton::MemoryRasterSample::data"));
    cxx_python_rules_nb::register_type<Skeleton::MemoryRasterSample>();
  }
  {
    nb::class_<Skeleton::BufferRasterSample, Skeleton::RasterSample> c(
        m, "BufferRasterSample",
        cxx_python_rules_nb::doc("Skeleton::BufferRasterSample"));
    c.def(nb::new_([](int Number_line, int Number_sample) {
            return std::make_shared<Skeleton::BufferRasterSample>(
                Number_line, Number_sample);
          }),
          nb::arg("Number_line"), nb::arg("Number_sample"));
    c.def_prop_ro("number_copy", [](const Skeleton::BufferRasterSample& self) {
      return self.number_copy();
    });
    cxx_python_rules_nb::register_type<Skeleton::BufferRasterSample>();
  }
  {
    nb::class_<Skeleton::SubRasterSample, Skeleton::RasterSample> c(
        m, "SubRasterSample",
        cxx_python_rules_nb::doc("Skeleton::SubRasterSample"));
    c.def(nb::new_([](const std::shared_ptr<Skeleton::RasterSample>& Full,
                      int Lstart, int Sstart, int Number_line,
                      int Number_sample) {
            return std::make_shared<Skeleton::SubRasterSample>(
                Full, Lstart, Sstart, Number_line, Number_sample);
          }),
          nb::arg("Full").none(), nb::arg("Lstart"), nb::arg("Sstart"),
          nb::arg("Number_line"), nb::arg("Number_sample"));
    c.def_prop_ro(
        "full",
        [](const Skeleton::SubRasterSample& self) -> decltype(auto) {
          return self.full();
        },
        cxx_python_rules_nb::doc("Skeleton::SubRasterSample::full"));
    cxx_python_rules_nb::register_type<Skeleton::SubRasterSample>();
  }
  {
    nb::class_<Skeleton::CalcRasterSample, Skeleton::RasterSample,
               PyCalcRasterSample>
        c(m, "CalcRasterSample",
          cxx_python_rules_nb::doc("Skeleton::CalcRasterSample"));
    // Abstract: only a python subclass can be constructed.
    c.def(nb::init<int, int>(), nb::arg("Number_line"),
          nb::arg("Number_sample"));
    // The protected scratch array, as a writable numpy view (geocal's
    // %python_attribute2_nonconst(data, data_ptr, ...)). A view of the
    // current array: get it again after each resize (each calc).
    c.def_prop_ro(
        "data",
        [](Skeleton::CalcRasterSample& self) -> blitz::Array<double, 2>& {
          return (self.*(&CalcPublicist::data_ptr))();
        },
        cxx_python_rules_nb::doc("Skeleton::CalcRasterSample::data_ptr"));
    cxx_python_rules_nb::register_type<Skeleton::CalcRasterSample>();
    cxx_python_rules_nb::register_alias<PyCalcRasterSample,
                                        Skeleton::CalcRasterSample>();
  }
  m.def(
      "copy_raster",
      [](const Skeleton::RasterSample& In, Skeleton::RasterSample& Out,
         int Tile_line, int Tile_sample) {
        Skeleton::copy_raster(In, Out, Tile_line, Tile_sample);
      },
      nb::arg("In"), nb::arg("Out"), nb::arg("Tile_line") = 100,
      nb::arg("Tile_sample") = 100,
      cxx_python_rules_nb::doc("Skeleton::copy_raster"));
}
