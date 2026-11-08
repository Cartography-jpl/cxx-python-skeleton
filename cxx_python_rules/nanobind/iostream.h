#pragma once
// A python file object (anything with read(n) / write(bytes)) as a
// std::istream or std::ostream, for C++ functions that take a stream
// (nanobind has nothing for this). Use in a
// binding lambda:
//
//   .def("write", [](const Foo& self, nb::object fh) {
//          cxx_python_rules_nb::py_ostream os(fh);
//          self.write(os);
//          os.flush();
//        })
//
// Input is unbuffered (one byte per read call), so after the C++ read the
// python file position is just past what C++ consumed, as with SWIG.
// Output is buffered. Flush it explicitly at the end, as above: then an
// error in the last write (e.g. a full disk) comes out of the call. The
// destructor flushes too, but it can't throw, so there an error is only
// reported as "Exception ignored in ..." (sys.unraisablehook).
//
// An exception raised by the python file object's write or read (or a
// TypeError, e.g. a text-mode file given bytes) comes out of the C++ code
// that used the stream: the streams have badbit in their exception mask,
// so iostreams rethrow what the streambuf threw instead of only setting
// badbit. An ordinary end of file still doesn't throw.
#include <nanobind/nanobind.h>
#include <array>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <streambuf>
#include <string>

// Hidden, like nanobind's own namespace (these hold nanobind objects).
namespace cxx_python_rules_nb __attribute__((visibility("hidden"))) {
  namespace nb = nanobind;

  class py_streambuf final : public std::streambuf {
  public:
    explicit py_streambuf(nb::object Fh) : fh(std::move(Fh))
    {
      setp(obuf.data(), obuf.data() + obuf.size());
    }
    py_streambuf(const py_streambuf&) = delete;
    py_streambuf& operator=(const py_streambuf&) = delete;
    ~py_streambuf() override
    {
      try {
        flush();
      } catch(...) { // NOLINT(bugprone-empty-catch)
        // A destructor can't throw. py_ostream's destructor has flushed
        // already (and reported a failure), so this is only a fallback.
      }
    }
  protected:
    int_type underflow() override
    {
      nb::bytes r = nb::cast<nb::bytes>(fh.attr("read")(1));
      if(r.size() == 0)
        return traits_type::eof();
      ibuf = r.c_str()[0];
      setg(&ibuf, &ibuf, &ibuf + 1);
      return traits_type::to_int_type(ibuf);
    }
    int_type overflow(int_type c) override
    {
      flush();
      if(!traits_type::eq_int_type(c, traits_type::eof())) {
        *pptr() = traits_type::to_char_type(c);
        pbump(1);
      }
      return traits_type::not_eof(c);
    }
    int sync() override
    {
      flush();
      return 0;
    }
  private:
    // Write the buffer to python. The buffer is emptied first, so data
    // whose write failed isn't written again by a later flush. A raw file
    // (open(..., buffering=0)) may write less than it was given and return
    // the count; then write the rest. Other file objects return None, or
    // the full count.
    void flush()
    {
      auto n = static_cast<size_t>(pptr() - pbase());
      setp(obuf.data(), obuf.data() + obuf.size());
      const char* p = obuf.data();
      while(n > 0) {
        nb::object r = fh.attr("write")(nb::bytes(p, n));
        if(!nb::isinstance<nb::int_>(r))
          return;
        auto written = nb::cast<size_t>(r);
        if(written == 0 || written > n)
          throw std::runtime_error("python file object's write wrote " +
                                   std::to_string(written) + " of " +
                                   std::to_string(n) + " bytes");
        p += written;
        n -= written;
      }
    }
    nb::object fh;
    char ibuf = 0;
    std::array<char, 4096> obuf{};
  };

  class py_ostream final : public std::ostream {
  public:
    explicit py_ostream(nb::object Fh)
      : std::ostream(nullptr), buf(std::move(Fh))
    {
      rdbuf(&buf);
      // After rdbuf: the constructor's null buffer set badbit, and a mask
      // with a bit that is already set throws at once.
      exceptions(std::ios::badbit);
    }
    py_ostream(const py_ostream&) = delete;
    py_ostream& operator=(const py_ostream&) = delete;
    ~py_ostream() override
    {
      try {
        flush();
      } catch(nb::python_error& e) {
        // Can't throw from here; let python print it.
        e.discard_as_unraisable("cxx_python_rules_nb::py_ostream");
      } catch(...) { // NOLINT(bugprone-empty-catch)
        // Nothing to report it to.
      }
    }
  private:
    py_streambuf buf;
  };

  class py_istream final : public std::istream {
  public:
    explicit py_istream(nb::object Fh)
      : std::istream(nullptr), buf(std::move(Fh))
    {
      rdbuf(&buf);
      exceptions(std::ios::badbit); // after rdbuf, as in py_ostream
    }
  private:
    py_streambuf buf;
  };
}
