import gc, probe3 as p
def rep(n, f):
    try: print(f"{n:58s} {f()}", flush=True)
    except Exception as e: print(f"{n:58s} EXC {type(e).__name__}: {e}", flush=True)
class PyE(p.E):
    def func(self, v): return v + 100
def t(make, strong_first=False):
    h = p.Holder(); o = make()
    if strong_first: h.set_strong(o)
    h.set_weak(o); gc.collect()
    r = f"expired={h.expired()} use_count={h.use_count()} apply={h.apply(1)}"
    del o; gc.collect()
    return r + f" | after del: expired={h.expired()}"
rep("esft, created in C++ (make_shared), returned to Python", lambda: t(p.make_cpp))
rep("esft, Python-constructed C++ class (nb::init)", lambda: t(p.E))
rep("esft, Python subclass (trampoline)", lambda: t(PyE))
rep("esft, Python subclass, C++ also holds strong ref", lambda: t(PyE, True))
