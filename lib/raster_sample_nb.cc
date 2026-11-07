// Python bindings for raster_sample.h, the mdspan sample modeled on
// geocal's RasterImage and CalcRaster (phase S7).
#include "skeleton_nb_opaque.h"
#include "raster_sample.h"
#include "skeleton_exception.h"
#include <nanobind/ndarray.h>

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

namespace {
// CalcRasterSample: python subclasses implement calc() and fill
// self.data, as geocal's python CalcRaster subclasses do.
class PyCalcRasterSample : public CalcRasterSample {
public:
  NB_TRAMPOLINE(CalcRasterSample);
  PyCalcRasterSample(int Number_line, int Number_sample)
    : CalcRasterSample(Number_line, Number_sample)
  {
  }
  void calc(int Lstart, int Sstart) const override
  {
    NB_OVERRIDE_PURE_NAME("calc", calc, Lstart, Sstart);
  }
};

// Reach the protected data_ptr (the usual "publicist": a member pointer
// taken through a derived class has the base class type).
struct CalcPublicist : CalcRasterSample {
  using CalcRasterSample::data_ptr;
};

// A writable numpy array, used as the output of read_into without a
// copy (any non-negative strides). noconvert: a copy would be filled
// and thrown away.
using out_array = nb::ndarray<int, nb::ndim<2>, nb::device::cpu>;

RasterView to_view(const out_array& A)
{
  std::array<std::ptrdiff_t, 2> ext{}, str{};
  for(std::size_t i = 0; i < 2; ++i) {
    ext[i] = static_cast<std::ptrdiff_t>(A.shape(i));
    str[i] = static_cast<std::ptrdiff_t>(A.stride(i));
    if(str[i] < 0)
      throw TypeError("read_into: negative strides not supported");
  }
  return RasterView(A.data(), RasterView::mapping_type(
                                  md::dextents<std::ptrdiff_t, 2>(ext), str));
}
}

void init_raster_sample(nb::module_& m)
{
  {
    // Abstract, no constructor.
    const auto doc = cxx_python_rules_nb::docs("Skeleton::RasterSample");
    nb::class_<RasterSample> c(m, "RasterSample", doc());
    c.def_prop_ro("number_line", &RasterSample::number_line);
    c.def_prop_ro("number_sample", &RasterSample::number_sample);
    c.def("read", &RasterSample::read, "Lstart"_a, "Sstart"_a, "Number_line"_a,
          "Number_sample"_a, doc("read"));
    c.def("read_with_pad", &RasterSample::read_with_pad, "Lstart"_a, "Sstart"_a,
          "Number_line"_a, "Number_sample"_a, "Fill_value"_a = 0,
          doc("read_with_pad"));
    // Reads into the caller's numpy array without a copy, so the argument
    // must be a writable int32 array (noconvert).
    c.def(
        "read_into",
        [](const RasterSample& self, int Lstart, int Sstart,
           const out_array& Res) {
          self.read_into_checked(Lstart, Sstart, to_view(Res));
        },
        "Lstart"_a, "Sstart"_a, "Res"_a.noconvert(),
        "Fill the writable int32 numpy array Res (any strides) with the\n"
        "data starting at Lstart, Sstart; the size read is Res.shape.");
    c.def("write",
          nb::overload_cast<int, int, const blitz::Array<int, 2>&>(
              &RasterSample::write),
          "Lstart"_a, "Sstart"_a, "V"_a, doc("write"));
    cxx_python_rules_nb::printable(c);
    cxx_python_rules_nb::register_type<RasterSample>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::MemoryRasterSample");
    nb::class_<MemoryRasterSample, RasterSample> c(m, "MemoryRasterSample",
                                                   doc());
    c.def(nb::new_([](int Number_line, int Number_sample, int Fill_value) {
            return std::make_shared<MemoryRasterSample>(
                Number_line, Number_sample, Fill_value);
          }),
          "Number_line"_a, "Number_sample"_a, "Fill_value"_a = 0);
    c.def(nb::new_([](const blitz::Array<int, 2>& Data) {
            return std::make_shared<MemoryRasterSample>(Data);
          }),
          "Data"_a);
    c.def_prop_ro("data", &MemoryRasterSample::data, doc("data"));
    cxx_python_rules_nb::register_type<MemoryRasterSample>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::BufferRasterSample");
    nb::class_<BufferRasterSample, RasterSample> c(m, "BufferRasterSample",
                                                   doc());
    c.def(nb::new_([](int Number_line, int Number_sample) {
            return std::make_shared<BufferRasterSample>(Number_line,
                                                        Number_sample);
          }),
          "Number_line"_a, "Number_sample"_a);
    c.def_prop_ro("number_copy", &BufferRasterSample::number_copy);
    cxx_python_rules_nb::register_type<BufferRasterSample>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::SubRasterSample");
    nb::class_<SubRasterSample, RasterSample> c(m, "SubRasterSample", doc());
    c.def(nb::new_([](const std::shared_ptr<RasterSample>& Full, int Lstart,
                      int Sstart, int Number_line, int Number_sample) {
            return std::make_shared<SubRasterSample>(
                Full, Lstart, Sstart, Number_line, Number_sample);
          }),
          "Full"_a, "Lstart"_a, "Sstart"_a, "Number_line"_a, "Number_sample"_a);
    c.def_prop_ro("full", &SubRasterSample::full, doc("full"));
    cxx_python_rules_nb::register_type<SubRasterSample>();
  }
  {
    const auto doc = cxx_python_rules_nb::docs("Skeleton::CalcRasterSample");
    nb::class_<CalcRasterSample, RasterSample, PyCalcRasterSample> c(
        m, "CalcRasterSample", doc());
    // Abstract: only a python subclass can be constructed.
    c.def(nb::init<int, int>(), "Number_line"_a, "Number_sample"_a);
    // The protected scratch array, as a writable numpy view (geocal's
    // %python_attribute2_nonconst(data, data_ptr, ...)). A view of the
    // current array: get it again after each resize (each calc).
    c.def_prop_ro(
        "data",
        [](CalcRasterSample& self) -> blitz::Array<double, 2>& {
          return (self.*(&CalcPublicist::data_ptr))();
        },
        doc("data_ptr"));
    cxx_python_rules_nb::register_type<CalcRasterSample>();
    cxx_python_rules_nb::register_alias<PyCalcRasterSample, CalcRasterSample>();
  }
  m.def("copy_raster", &copy_raster, "In"_a, "Out"_a, "Tile_line"_a = 100,
        "Tile_sample"_a = 100,
        cxx_python_rules_nb::doc("Skeleton::copy_raster"));
}
