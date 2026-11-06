#ifndef CXX_PYTHON_RULES_NB_IOSTREAM_H
#define CXX_PYTHON_RULES_NB_IOSTREAM_H
// A python file object (anything with read(n) / write(bytes)) as a
// std::istream or std::ostream, for C++ functions that take a stream
// (nanobind has nothing for this; replaces swig_iostream.i). Use in a
// binding lambda:
//
//   .def("write", [](const Foo& self, nb::object fh) {
//          cxx_python_rules_nb::py_ostream os(fh);
//          self.write(os);
//        })
//
// Input is unbuffered (one byte per read call), so after the C++ read the
// python file position is just past what C++ consumed, as with SWIG.
// Output is buffered and flushed when the stream is destroyed.
#include <nanobind/nanobind.h>
#include <istream>
#include <ostream>
#include <streambuf>
#include <string>

// Hidden, like nanobind's own namespace (these hold nanobind objects).
namespace cxx_python_rules_nb __attribute__((visibility("hidden"))) {
  namespace nb = nanobind;

  class py_streambuf : public std::streambuf {
  public:
    explicit py_streambuf(nb::object Fh) : fh(std::move(Fh))
    {
      setp(obuf, obuf + sizeof(obuf));
    }
    ~py_streambuf() override
    {
      try {
        flush();
      } catch(...) {
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
    void flush()
    {
      std::ptrdiff_t n = pptr() - pbase();
      if(n > 0)
        fh.attr("write")(nb::bytes(pbase(), static_cast<size_t>(n)));
      setp(obuf, obuf + sizeof(obuf));
    }
    nb::object fh;
    char ibuf = 0;
    char obuf[4096];
  };

  class py_ostream : public std::ostream {
  public:
    explicit py_ostream(nb::object Fh)
      : std::ostream(nullptr), buf(std::move(Fh))
    {
      rdbuf(&buf);
    }
    ~py_ostream() override { flush(); }
  private:
    py_streambuf buf;
  };

  class py_istream : public std::istream {
  public:
    explicit py_istream(nb::object Fh)
      : std::istream(nullptr), buf(std::move(Fh))
    {
      rdbuf(&buf);
    }
  private:
    py_streambuf buf;
  };
}
#endif
