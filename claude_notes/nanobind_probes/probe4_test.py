import gc, importlib, sys
p = importlib.import_module(sys.argv[1])
def rep(n, f):
    try: print(f"  {n:56s} {f()}", flush=True)
    except Exception as e: print(f"  {n:56s} EXC {type(e).__name__}: {e}", flush=True)
class PyB(p.Base):
    def func(self, v): return v + 100
def t(make, strong_first=False):
    h = p.Holder(); o = make()
    if strong_first: h.set_strong(o)
    h.set_weak(o); gc.collect()
    r = f"expired={h.expired()} apply={h.apply(1)}"
    del o; gc.collect()
    return r + f" | after del: expired={h.expired()}"
rep("created in C++ (shared_ptr<Base>), returned", lambda: t(p.make_cpp))
rep("Python-constructed Derived (nb::init)", lambda: t(p.Derived))
rep("Python-constructed Derived, C++ holds strong ref", lambda: t(p.Derived, True))
rep("Python subclass, C++ holds strong ref", lambda: t(PyB, True))
