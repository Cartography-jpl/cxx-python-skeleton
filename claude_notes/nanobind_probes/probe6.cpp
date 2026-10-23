// Throwaway probe 6: GeoCal-style diamond with virtual bases
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <functional>
#include <map>
#include <memory>
#include <typeindex>
namespace nb = nanobind;
struct GenericObject { virtual ~GenericObject() = default; };
struct WithParameter : public virtual GenericObject { virtual int psize() const { return 2; } };
struct WithParameterNested : public virtual WithParameter { int psize() const override { return 5; } };
struct ImageGroundConnection : public virtual GenericObject { virtual int nline() const { return 10; } };
struct IgcRollingShutter : public virtual ImageGroundConnection, public virtual WithParameterNested {
  int nline() const override { return 42; }
};
// type mapper (C++ side dynamic_pointer_cast handles virtual bases)
using caster_t = std::function<nb::object(const std::shared_ptr<GenericObject>&)>;
std::map<std::type_index, caster_t>& reg() { static std::map<std::type_index, caster_t> r; return r; }
template<class T> void register_generic() {
  reg()[typeid(T)] = [](const std::shared_ptr<GenericObject>& p) { return nb::cast(std::dynamic_pointer_cast<T>(p)); };
}
template<class B> nb::object to_python(const std::shared_ptr<B>& b) {
  std::shared_ptr<GenericObject> p = std::dynamic_pointer_cast<GenericObject>(b);
  if(!p) return nb::none();
  auto it = reg().find(typeid(*p));
  return it == reg().end() ? nb::cast(b) : it->second(p);
}
NB_MODULE(probe6, m) {
  nb::class_<ImageGroundConnection>(m, "ImageGroundConnection").def("nline", &ImageGroundConnection::nline);
#ifdef BIND_VIRTUAL_BASE_IN_CHAIN
  nb::class_<IgcRollingShutter, ImageGroundConnection>(m, "IgcRollingShutter").def(nb::init<>())
#else
  nb::class_<IgcRollingShutter>(m, "IgcRollingShutter").def(nb::init<>())
    .def("nline", &IgcRollingShutter::nline)
#endif
    .def("psize", [](const IgcRollingShutter& i) { return i.psize(); });
  register_generic<IgcRollingShutter>();
  m.def("make_igc_raw", []() { return std::shared_ptr<ImageGroundConnection>(std::make_shared<IgcRollingShutter>()); });
  m.def("make_igc_mapped", []() { return to_python(std::shared_ptr<ImageGroundConnection>(std::make_shared<IgcRollingShutter>())); });
  m.def("make_withparam_mapped", []() { return to_python(std::shared_ptr<WithParameter>(std::make_shared<IgcRollingShutter>())); });
  m.def("take_igc", [](std::shared_ptr<ImageGroundConnection> i) { return i->nline(); });
}
