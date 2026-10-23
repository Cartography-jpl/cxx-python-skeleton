import gc
import probe2 as p

def report(name, fn):
    try:
        print(f"{name:62s} {fn()}", flush=True)
    except Exception as e:
        print(f"{name:62s} EXCEPTION {type(e).__name__}: {e}", flush=True)

report("(a) type mapper: shared_ptr<GenericObject> -> type", lambda: type(p.make_generic_mapped()).__name__)
report("(a) type mapper: method call works", lambda: p.make_generic_mapped().func(1))

def w_plain_init():
    h = p.Holder(); o = p.Plain(); h.set_weak_plain(o); gc.collect()
    return f"expired={h.plain_expired()}"
report("(b1) weak_ptr, plain class, nb::init (Python-owned)", w_plain_init)

def w_plain_new():
    h = p.Holder(); o = p.PlainShared(); h.set_weak_plain(o); gc.collect()
    r = f"expired={h.plain_expired()}"
    del o; gc.collect()
    return r + f" | after del: expired={h.plain_expired()}"
report("(b2) weak_ptr, plain class, nb::new_ shared_ptr factory", w_plain_new)

def w_cpp_made():
    h = p.Holder(); o = p.make_base_shared(); h.set_weak_base(o); gc.collect()
    r = f"expired={h.base_expired()} apply={h.apply(1)}"
    del o; gc.collect()
    return r + f" | after del: expired={h.base_expired()}"
report("(b3) weak_ptr, object created in C++ (shared_ptr), returned", w_cpp_made)

class PyB(p.Base):
    def func(self, v):
        return v + 100

def w_pysub_strong_first():
    h = p.Holder(); o = PyB()
    h.set_strong_base(o)       # some C++ owner holds a shared_ptr first
    h.set_weak_base(o); gc.collect()
    return f"expired={h.base_expired()} apply={h.apply(1)}"
report("(b4) weak_ptr, Python subclass, while a C++ shared_ptr exists", w_pysub_strong_first)

def w_pysub_only():
    h = p.Holder(); o = PyB(); h.set_weak_base(o); gc.collect()
    return f"expired={h.base_expired()}"
report("(b5) weak_ptr, Python subclass, only Python holds it", w_pysub_only)
