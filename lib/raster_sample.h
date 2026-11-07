#ifndef RASTER_SAMPLE_H
#define RASTER_SAMPLE_H
#include "blitz_mdspan.h"
#include "printable.h"
#include <blitz/array.h>
#include <memory>
#include <vector>

namespace Skeleton {
/****************************************************************/ /**
  Sample for replacing a raw pointer plus sizes with an mdspan
  (claude_notes/implementation_plan.md S7). Modeled on geocal's
  RasterImage, whose virtual

    read_ptr(int Lstart, int Sstart, int Number_line, int Number_sample,
             int* Res) const

  is overridden in about 14 classes (and the same shape in TiledFile,
  VicarLiteFile, ...). Here the virtual is read_into(), and the size
  of the region comes from the extents of the mdspan, so the pointer
  and its sizes can't disagree.

  Choices made, for geocal to copy or change:
  * The view is a layout_stride mdspan (RasterView), so a caller can
    read into part of a bigger buffer (read_with_pad, a tile of a
    blitz array, a numpy slice). A plain row-major mdspan converts to
    it implicitly. An implementation that needs contiguous memory (a C
    library, see BufferRasterSample) checks and copies if it isn't.
  * read_ptr()/write_ptr() stay as non-virtual wrappers, so callers can
    move over one at a time; only the overrides have to change at once.
  * Overrides use different names (read_into, write_from) from the
    blitz overloads (read, write), so overriding one doesn't hide the
    others in the derived class.
  * Index type std::ptrdiff_t, the same as blitz_mdspan.h (to_mdspan of
    a blitz array gives a RasterView with no conversion).
*******************************************************************/

using RasterView = strided_mdspan<int, 2>;
using ConstRasterView = strided_mdspan<const int, 2>;

class RasterSample : public Printable<RasterSample> {
public:
  virtual int number_line() const = 0;
  virtual int number_sample() const = 0;

  //-----------------------------------------------------------------------
  /// Fill Res with the data starting at Lstart, Sstart. The size read is
  /// the extents of Res. Res can have any (non-negative) strides.
  /// Derived classes implement this; it is range checked by the
  /// callers below (read_into_checked).
  //-----------------------------------------------------------------------

  virtual void read_into(int Lstart, int Sstart,
                         const RasterView& Res) const = 0;

  //-----------------------------------------------------------------------
  /// Write V starting at Lstart, Sstart. The default throws (read only).
  //-----------------------------------------------------------------------

  virtual void write_from(int Lstart, int Sstart, const ConstRasterView& V);

  //-----------------------------------------------------------------------
  /// read_into with a range check.
  //-----------------------------------------------------------------------

  void read_into_checked(int Lstart, int Sstart, const RasterView& Res) const
  {
    check_region(Lstart, Sstart, Res.extent(0), Res.extent(1));
    read_into(Lstart, Sstart, Res);
  }

  //-----------------------------------------------------------------------
  /// The old pointer interface: Res has room for Number_line *
  /// Number_sample values, row major.
  //-----------------------------------------------------------------------

  void read_ptr(int Lstart, int Sstart, int Number_line, int Number_sample,
                int* Res) const
  {
    // Check before making the mdspan: a negative extent is a
    // precondition violation of its constructor.
    check_region(Lstart, Sstart, Number_line, Number_sample);
    read_into(Lstart, Sstart,
              md::mdspan<int, md::dextents<std::ptrdiff_t, 2>>(Res, Number_line,
                                                               Number_sample));
  }
  void write_ptr(int Lstart, int Sstart, int Number_line, int Number_sample,
                 const int* V)
  {
    check_region(Lstart, Sstart, Number_line, Number_sample);
    write_from(Lstart, Sstart,
               md::mdspan<const int, md::dextents<std::ptrdiff_t, 2>>(
                   V, Number_line, Number_sample));
  }

  //-----------------------------------------------------------------------
  /// Read into a new blitz array (a numpy array in python).
  //-----------------------------------------------------------------------

  blitz::Array<int, 2> read(int Lstart, int Sstart, int Number_line,
                            int Number_sample) const;

  //-----------------------------------------------------------------------
  /// Like read, but the region may go past the edges of the image; the
  /// part outside is Fill_value. Reads the part inside straight into
  /// the middle of the result (a submdspan), no temporary.
  //-----------------------------------------------------------------------

  blitz::Array<int, 2> read_with_pad(int Lstart, int Sstart, int Number_line,
                                     int Number_sample,
                                     int Fill_value = 0) const;

  //-----------------------------------------------------------------------
  /// Write a blitz array (a numpy array in python) at Lstart, Sstart.
  //-----------------------------------------------------------------------

  void write(int Lstart, int Sstart, const blitz::Array<int, 2>& V)
  {
    check_region(Lstart, Sstart, V.rows(), V.cols());
    write_from(Lstart, Sstart, to_mdspan(V));
  }

  virtual void print(std::ostream& Os) const;
protected:
  void check_region(std::ptrdiff_t Lstart, std::ptrdiff_t Sstart,
                    std::ptrdiff_t Number_line,
                    std::ptrdiff_t Number_sample) const;
};

//-----------------------------------------------------------------------
/// Copy In to Out in tiles of Tile_line x Tile_sample, through one
/// buffer (geocal's copy(RasterImage, RasterImage) does this). The
/// buffer is a std::vector viewed as an mdspan; the last tiles are
/// smaller views of the same buffer.
//-----------------------------------------------------------------------

void copy_raster(const RasterSample& In, RasterSample& Out, int Tile_line = 100,
                 int Tile_sample = 100);

/****************************************************************/ /**
  A RasterSample in memory (geocal's MemoryRasterImage). read_into
  assigns to a blitz view of the mdspan's memory, so the copy is done
  by blitz with any strides on either side.
*******************************************************************/

class MemoryRasterSample : public RasterSample {
public:
  MemoryRasterSample(int Number_line, int Number_sample, int Fill_value = 0)
    : data_(Number_line, Number_sample)
  {
    data_ = Fill_value;
  }
  // Copies Data.
  explicit MemoryRasterSample(const blitz::Array<int, 2>& Data)
    : data_(Data.copy())
  {
  }
  int number_line() const override { return data_.rows(); }
  int number_sample() const override { return data_.cols(); }
  void read_into(int Lstart, int Sstart, const RasterView& Res) const override;
  void write_from(int Lstart, int Sstart, const ConstRasterView& V) override;
  const blitz::Array<int, 2>& data() const { return data_; }
  void print(std::ostream& Os) const override;
private:
  blitz::Array<int, 2> data_;
};

/****************************************************************/ /**
  Stand-in for a C library that reads and writes blocks through a
  pointer (VICAR zvreadw, GDAL RasterIO, an HDF read): it needs
  contiguous row-major memory and returns a status code. The "file" is
  just a vector.
*******************************************************************/

struct LegacyRasterFile {
  int number_line = 0;
  int number_sample = 0;
  std::vector<int> buf;
};
// Return 0 on success, -1 if the block is outside the file.
int legacy_read_block(const LegacyRasterFile* F, int Lstart, int Sstart,
                      int Number_line, int Number_sample, int* Buf);
int legacy_write_block(LegacyRasterFile* F, int Lstart, int Sstart,
                       int Number_line, int Number_sample, const int* Buf);

/****************************************************************/ /**
  A RasterSample read through the C-style API above. When the mdspan is
  contiguous and row major, its pointer goes straight to the library;
  otherwise (a strided view, e.g. a numpy slice or part of a padded
  read) it reads into a temporary and copies. number_copy() counts the
  second case, for the tests.
*******************************************************************/

class BufferRasterSample : public RasterSample {
public:
  // A file of Number_line x Number_sample, value 1000 * line + sample.
  BufferRasterSample(int Number_line, int Number_sample);
  int number_line() const override { return f.number_line; }
  int number_sample() const override { return f.number_sample; }
  void read_into(int Lstart, int Sstart, const RasterView& Res) const override;
  void write_from(int Lstart, int Sstart, const ConstRasterView& V) override;
  int number_copy() const { return number_copy_; }
  void print(std::ostream& Os) const override;
private:
  LegacyRasterFile f;
  mutable int number_copy_ = 0;
};

/****************************************************************/ /**
  A subset of another RasterSample (geocal's SubRasterImage). Passes
  the view on unchanged, with the offset added.
*******************************************************************/

class SubRasterSample : public RasterSample {
public:
  SubRasterSample(const std::shared_ptr<RasterSample>& Full, int Lstart,
                  int Sstart, int Number_line, int Number_sample);
  int number_line() const override { return nline; }
  int number_sample() const override { return nsamp; }
  void read_into(int Lstart, int Sstart, const RasterView& Res) const override
  {
    full_->read_into(Lstart + lstart, Sstart + sstart, Res);
  }
  void write_from(int Lstart, int Sstart, const ConstRasterView& V) override
  {
    full_->write_from(Lstart + lstart, Sstart + sstart, V);
  }
  const std::shared_ptr<RasterSample>& full() const { return full_; }
  void print(std::ostream& Os) const override;
private:
  std::shared_ptr<RasterSample> full_;
  int lstart, sstart, nline, nsamp;
};

/****************************************************************/ /**
  geocal's CalcRaster pattern, which python subclasses use: a derived
  class (C++ or python) implements calc(), filling the protected data
  array (a writable numpy view in python) for the region asked for;
  read_into then converts it to int into the caller's mdspan.

  Not thread safe (data is shared scratch space), as in geocal.
*******************************************************************/

class CalcRasterSample : public RasterSample {
public:
  int number_line() const final { return nline; }
  int number_sample() const final { return nsamp; }
  void read_into(int Lstart, int Sstart, const RasterView& Res) const final;
  void print(std::ostream& Os) const override;
protected:
  CalcRasterSample(int Number_line, int Number_sample)
    : nline(Number_line), nsamp(Number_sample)
  {
  }

  //-----------------------------------------------------------------------
  /// Fill data for the region starting at Lstart, Sstart. data has
  /// already been resized to the size of the region.
  //-----------------------------------------------------------------------

  virtual void calc(int Lstart, int Sstart) const = 0;
  blitz::Array<double, 2>& data_ptr() const { return data; }
  mutable blitz::Array<double, 2> data;
private:
  int nline, nsamp;
};
}
#endif
