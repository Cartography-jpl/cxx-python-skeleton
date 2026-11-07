// Python bindings for the serialization functions (serialize_function.h).
// File names are str (the C++ functions take std::filesystem::path), and
// binary archives are python bytes.
#include "skeleton_nb_opaque.h"
#include "serialize_function.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace Skeleton;

namespace {
std::string to_string(const nb::bytes& B)
{
  return {B.c_str(), B.size()};
}
nb::bytes to_bytes(const std::string& S)
{
  return nb::bytes(S.data(), S.size());
}
}

void init_serialize_function(nb::module_& m)
{
  using cxx_python_rules_nb::doc;
  using Obj = std::shared_ptr<GenericObject>;
  m.def("have_serialize_supported", &have_serialize_supported,
        doc("Skeleton::have_serialize_supported"));
  m.def(
      "serialize_write",
      [](const std::string& Fname, const Obj& O) { serialize_write(Fname, O); },
      "Fname"_a, "Obj"_a.none(), doc("Skeleton::serialize_write"));
  m.def("serialize_write_string", &serialize_write_string, "Obj"_a.none(),
        doc("Skeleton::serialize_write_string"));
  m.def(
      "serialize_read_generic",
      [](const std::string& Fname) { return serialize_read_generic(Fname); },
      "Fname"_a, doc("Skeleton::serialize_read_generic"));
  m.def("serialize_read_generic_string", &serialize_read_generic_string,
        "Data"_a, doc("Skeleton::serialize_read_generic_string"));
  m.def(
      "serialize_write_binary",
      [](const std::string& Fname, const Obj& O) {
        serialize_write_binary(Fname, O);
      },
      "Fname"_a, "Obj"_a.none(), doc("Skeleton::serialize_write_binary"));
  m.def(
      "serialize_read_binary_generic",
      [](const std::string& Fname) {
        return serialize_read_binary_generic(Fname);
      },
      "Fname"_a, doc("Skeleton::serialize_read_binary_generic"));
  m.def(
      "serialize_write_binary",
      [](const Obj& O) { return to_bytes(serialize_write_binary(O)); },
      "Obj"_a.none(), doc("Skeleton::serialize_write_binary"));
  m.def(
      "serialize_read_binary",
      [](const nb::bytes& Data) {
        return serialize_read_binary(to_string(Data));
      },
      "Data"_a, doc("Skeleton::serialize_read_binary"));
  m.def(
      "serialize_read_binary",
      [](const nb::bytes& Data, const std::string& Base_dir) {
        return serialize_read_binary(to_string(Data), Base_dir);
      },
      "Data"_a, "Base_dir"_a, doc("Skeleton::serialize_read_binary"));
  m.def(
      "set_archive_base_directory",
      [](const std::string& Dir) {
        return set_archive_base_directory(Dir).string();
      },
      "Dir"_a, doc("Skeleton::set_archive_base_directory"));
  m.def(
      "archive_base_directory",
      []() { return archive_base_directory().string(); },
      doc("Skeleton::archive_base_directory"));
}
