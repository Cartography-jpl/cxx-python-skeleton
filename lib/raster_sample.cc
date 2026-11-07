#include "raster_sample.h"
#include "skeleton_exception.h"
#include <algorithm>
#include <cmath>
#include <ranges>

using namespace Skeleton;

namespace {
// Element by element copy between two mdspans of the same extents, any
// layouts (C++23 cartesian_product instead of two nested loops).
template<class From, class To> void copy_view(const From& F, const To& T)
{
  using std::views::iota;
  for(auto [i, j] :
      std::views::cartesian_product(iota(std::ptrdiff_t{0}, F.extent(0)),
                                    iota(std::ptrdiff_t{0}, F.extent(1))))
    T[i, j] = F[i, j];
}

// True if M is row major and contiguous, so its pointer can go to a C
// library expecting Number_line * Number_sample values in a row.
template<class M> bool is_row_major_contiguous(const M& V)
{
  if(V.extent(0) == 0 || V.extent(1) == 0)
    return true;
  return (V.extent(1) == 1 || V.stride(1) == 1) &&
         (V.extent(0) == 1 || V.stride(0) == V.extent(1));
}
}

//-----------------------------------------------------------------------
// RasterSample
//-----------------------------------------------------------------------

void RasterSample::write_from(int, int, const ConstRasterView&)
{
  throw Exception("{} is read only", desc());
}

void RasterSample::check_region(std::ptrdiff_t Lstart, std::ptrdiff_t Sstart,
                                std::ptrdiff_t Number_line,
                                std::ptrdiff_t Number_sample) const
{
  if(Lstart < 0 || Sstart < 0 || Number_line < 0 || Number_sample < 0 ||
     Lstart + Number_line > number_line() ||
     Sstart + Number_sample > number_sample())
    throw Exception("Region ({}, {}) size {} x {} is outside the image "
                    "of {} x {}",
                    Lstart, Sstart, Number_line, Number_sample, number_line(),
                    number_sample());
}

blitz::Array<int, 2> RasterSample::read(int Lstart, int Sstart, int Number_line,
                                        int Number_sample) const
{
  // Check before allocating: a negative size must not reach blitz.
  check_region(Lstart, Sstart, Number_line, Number_sample);
  blitz::Array<int, 2> res(Number_line, Number_sample);
  read_into(Lstart, Sstart, to_mdspan(res));
  return res;
}

blitz::Array<int, 2> RasterSample::read_with_pad(int Lstart, int Sstart,
                                                 int Number_line,
                                                 int Number_sample,
                                                 int Fill_value) const
{
  if(Number_line < 0 || Number_sample < 0)
    throw Exception("read_with_pad: size {} x {} is negative", Number_line,
                    Number_sample);
  blitz::Array<int, 2> res(Number_line, Number_sample);
  res = Fill_value;
  // The part of the region inside the image, in image coordinates.
  int l0 = std::max(Lstart, 0);
  int s0 = std::max(Sstart, 0);
  int l1 = std::min(Lstart + Number_line, number_line());
  int s1 = std::min(Sstart + Number_sample, number_sample());
  if(l0 >= l1 || s0 >= s1)
    return res;
  // Read it straight into that part of res: a strided view.
  RasterView inside = md::submdspan(
      to_mdspan(res),
      std::pair<std::ptrdiff_t, std::ptrdiff_t>(l0 - Lstart, l1 - Lstart),
      std::pair<std::ptrdiff_t, std::ptrdiff_t>(s0 - Sstart, s1 - Sstart));
  read_into_checked(l0, s0, inside);
  return res;
}

void RasterSample::print(std::ostream& Os) const
{
  Os << "RasterSample " << number_line() << " x " << number_sample();
}

void Skeleton::copy_raster(const RasterSample& In, RasterSample& Out,
                           int Tile_line, int Tile_sample)
{
  if(In.number_line() != Out.number_line() ||
     In.number_sample() != Out.number_sample())
    throw Exception("copy_raster: sizes differ, {} x {} and {} x {}",
                    In.number_line(), In.number_sample(), Out.number_line(),
                    Out.number_sample());
  if(Tile_line <= 0 || Tile_sample <= 0)
    throw Exception("copy_raster: tile size must be positive");
  std::vector<int> buf(static_cast<std::size_t>(Tile_line) *
                       static_cast<std::size_t>(Tile_sample));
  for(int l = 0; l < In.number_line(); l += Tile_line)
    for(int s = 0; s < In.number_sample(); s += Tile_sample) {
      int nl = std::min(Tile_line, In.number_line() - l);
      int ns = std::min(Tile_sample, In.number_sample() - s);
      // A row-major view of the start of buf, the size of this tile.
      md::mdspan<int, md::dextents<std::ptrdiff_t, 2>> tile(buf.data(), nl, ns);
      In.read_into_checked(l, s, tile);
      Out.write_ptr(l, s, nl, ns, buf.data());
    }
}

//-----------------------------------------------------------------------
// MemoryRasterSample
//-----------------------------------------------------------------------

void MemoryRasterSample::read_into(int Lstart, int Sstart,
                                   const RasterView& Res) const
{
  // A blitz array using Res's memory (no owner: Res's memory outlives
  // this call), assigned from the region of data_.
  blitz::Array<int, 2> r = to_blitz(Res);
  r = data_(blitz::Range(Lstart, Lstart + r.rows() - 1),
            blitz::Range(Sstart, Sstart + r.cols() - 1));
}

void MemoryRasterSample::write_from(int Lstart, int Sstart,
                                    const ConstRasterView& V)
{
  // to_blitz has no const element type; it only reads through it here.
  RasterView v(const_cast<int*>(V.data_handle()), V.mapping());
  data_(blitz::Range(Lstart, Lstart + static_cast<int>(v.extent(0)) - 1),
        blitz::Range(Sstart, Sstart + static_cast<int>(v.extent(1)) - 1)) =
      to_blitz(v);
}

void MemoryRasterSample::print(std::ostream& Os) const
{
  Os << "MemoryRasterSample " << number_line() << " x " << number_sample();
}

//-----------------------------------------------------------------------
// The "C library".
//-----------------------------------------------------------------------

int Skeleton::legacy_read_block(const LegacyRasterFile* F, int Lstart,
                                int Sstart, int Number_line, int Number_sample,
                                int* Buf)
{
  if(Lstart < 0 || Sstart < 0 || Lstart + Number_line > F->number_line ||
     Sstart + Number_sample > F->number_sample)
    return -1;
  for(int i = 0; i < Number_line; ++i)
    std::copy_n(
        F->buf.data() +
            static_cast<std::ptrdiff_t>(Lstart + i) * F->number_sample + Sstart,
        Number_sample, Buf + static_cast<std::ptrdiff_t>(i) * Number_sample);
  return 0;
}

int Skeleton::legacy_write_block(LegacyRasterFile* F, int Lstart, int Sstart,
                                 int Number_line, int Number_sample,
                                 const int* Buf)
{
  if(Lstart < 0 || Sstart < 0 || Lstart + Number_line > F->number_line ||
     Sstart + Number_sample > F->number_sample)
    return -1;
  for(int i = 0; i < Number_line; ++i)
    std::copy_n(Buf + static_cast<std::ptrdiff_t>(i) * Number_sample,
                Number_sample,
                F->buf.data() +
                    static_cast<std::ptrdiff_t>(Lstart + i) * F->number_sample +
                    Sstart);
  return 0;
}

//-----------------------------------------------------------------------
// BufferRasterSample
//-----------------------------------------------------------------------

BufferRasterSample::BufferRasterSample(int Number_line, int Number_sample)
  : f{.number_line = Number_line,
      .number_sample = Number_sample,
      .buf = std::vector<int>(static_cast<std::size_t>(Number_line) *
                              static_cast<std::size_t>(Number_sample))}
{
  md::mdspan<int, md::dextents<std::ptrdiff_t, 2>> v(f.buf.data(), Number_line,
                                                     Number_sample);
  for(std::ptrdiff_t i = 0; i < v.extent(0); ++i)
    for(std::ptrdiff_t j = 0; j < v.extent(1); ++j)
      v[i, j] = static_cast<int>(1000 * i + j);
}

void BufferRasterSample::read_into(int Lstart, int Sstart,
                                   const RasterView& Res) const
{
  int nl = static_cast<int>(Res.extent(0));
  int ns = static_cast<int>(Res.extent(1));
  int status;
  if(is_row_major_contiguous(Res))
    status = legacy_read_block(&f, Lstart, Sstart, nl, ns, Res.data_handle());
  else {
    ++number_copy_;
    std::vector<int> tmp(static_cast<std::size_t>(nl) *
                         static_cast<std::size_t>(ns));
    status = legacy_read_block(&f, Lstart, Sstart, nl, ns, tmp.data());
    if(status == 0)
      copy_view(md::mdspan<const int, md::dextents<std::ptrdiff_t, 2>>(
                    tmp.data(), nl, ns),
                Res);
  }
  if(status != 0)
    throw Exception("legacy_read_block failed with status {}", status);
}

void BufferRasterSample::write_from(int Lstart, int Sstart,
                                    const ConstRasterView& V)
{
  int nl = static_cast<int>(V.extent(0));
  int ns = static_cast<int>(V.extent(1));
  int status;
  if(is_row_major_contiguous(V))
    status = legacy_write_block(&f, Lstart, Sstart, nl, ns, V.data_handle());
  else {
    ++number_copy_;
    std::vector<int> tmp(static_cast<std::size_t>(nl) *
                         static_cast<std::size_t>(ns));
    copy_view(V, md::mdspan<int, md::dextents<std::ptrdiff_t, 2>>(tmp.data(),
                                                                  nl, ns));
    status = legacy_write_block(&f, Lstart, Sstart, nl, ns, tmp.data());
  }
  if(status != 0)
    throw Exception("legacy_write_block failed with status {}", status);
}

void BufferRasterSample::print(std::ostream& Os) const
{
  Os << "BufferRasterSample " << number_line() << " x " << number_sample();
}

//-----------------------------------------------------------------------
// SubRasterSample
//-----------------------------------------------------------------------

SubRasterSample::SubRasterSample(const std::shared_ptr<RasterSample>& Full,
                                 int Lstart, int Sstart, int Number_line,
                                 int Number_sample)
  : full_(Full), lstart(Lstart), sstart(Sstart), nline(Number_line),
    nsamp(Number_sample)
{
  if(!full_)
    throw Exception("SubRasterSample: Full is null");
  if(Lstart < 0 || Sstart < 0 || Number_line < 0 || Number_sample < 0 ||
     Lstart + Number_line > full_->number_line() ||
     Sstart + Number_sample > full_->number_sample())
    throw Exception("SubRasterSample: region is outside the full image");
}

void SubRasterSample::print(std::ostream& Os) const
{
  Os << "SubRasterSample " << number_line() << " x " << number_sample()
     << " at (" << lstart << ", " << sstart << ") of " << *full_;
}

//-----------------------------------------------------------------------
// CalcRasterSample
//-----------------------------------------------------------------------

void CalcRasterSample::read_into(int Lstart, int Sstart,
                                 const RasterView& Res) const
{
  data.resize(static_cast<int>(Res.extent(0)), static_cast<int>(Res.extent(1)));
  calc(Lstart, Sstart);
  // calc (maybe python) may have replaced data with a different size.
  if(data.rows() != Res.extent(0) || data.cols() != Res.extent(1))
    throw Exception("calc changed the size of data to {} x {}, expected "
                    "{} x {}",
                    data.rows(), data.cols(), Res.extent(0), Res.extent(1));
  blitz::Array<int, 2> r = to_blitz(Res);
  for(int i = 0; i < r.rows(); ++i)
    for(int j = 0; j < r.cols(); ++j)
      r(i, j) = static_cast<int>(std::lround(data(i, j)));
}

void CalcRasterSample::print(std::ostream& Os) const
{
  Os << "CalcRasterSample " << number_line() << " x " << number_sample();
}
