import gc
import probe

def report(name, fn):
    try:
        print(f"{name:55s} {fn()}")
    except Exception as e:
        print(f"{name:55s} EXCEPTION {type(e).__name__}: {e}")

# --- Downcasting and multiple / virtual inheritance ---
report("make_camera_as_base() type", lambda: type(probe.make_camera_as_base()).__name__)
report("make_camera_as_base().number_band()", lambda: probe.make_camera_as_base().number_band())
report("make_generic() type (virtual base GenericObject)", lambda: type(probe.make_generic()).__name__)
report("make_generic().number_band()", lambda: probe.make_generic().number_band())
report("make_withparam() type (2nd C++ base, bound base)", lambda: type(probe.make_withparam()).__name__)
report("make_withparam().number_band()", lambda: probe.make_withparam().number_band())
report("str(QuaternionCamera())", lambda: str(probe.QuaternionCamera()))
report("use_withparam(QuaternionCamera())", lambda: probe.use_withparam(probe.QuaternionCamera()))
report("add_observer_count via base member ptr", lambda: (lambda c: (c.add_observer_count(), c.observer_count())[1])(probe.QuaternionCamera()))
report("use_observable (unbound base via lambda)", lambda: probe.use_observable(probe.QuaternionCamera()))
report("isinstance(QuaternionCamera(), WithParameter)", lambda: isinstance(probe.QuaternionCamera(), probe.WithParameter))

# --- Directors held by shared_ptr ---
class PyD(probe.Director):
    def func(self, v):
        return v + 100

def director_strong():
    u = probe.DirectorUser()
    u.set(PyD())          # Python temp dropped right after the call
    gc.collect()
    return u.apply(1)     # expect 101 if C++ keeps the Python object alive
report("director held by shared_ptr after Python ref dropped", director_strong)

def director_identity():
    u = probe.DirectorUser()
    d = PyD()
    u.set(d)
    return u.get() is d
report("shared_ptr round trip returns same Python object", director_identity)

def director_exception():
    class Bad(probe.Director):
        def func(self, v):
            raise RuntimeError("original message")
    u = probe.DirectorUser(); u.set(Bad())
    try:
        u.apply(1)
    except RuntimeError as e:
        return f"RuntimeError propagated: {e}"
    return "no exception"
report("exception from Python override propagates", director_exception)

# --- weak_ptr to Python-subclass objects ---
def weak_plain():
    u = probe.DirectorUser()
    d = PyD()
    u.set_weak(d)
    gc.collect()
    return f"expired={u.weak_expired()} apply={u.apply_weak(1)} (Python object still alive)"
report("weak_ptr to Python subclass, no enable_shared_from_this", weak_plain)

def weak_plain_cpp():
    u = probe.DirectorUser()
    d = probe.Director()
    u.set_weak(d)
    gc.collect()
    return f"expired={u.weak_expired()} apply={u.apply_weak(1)}"
report("weak_ptr to C++-created (Python-constructed) object", weak_plain_cpp)

class PyE(probe.DirectorEsft):
    def func(self, v):
        return v + 100

def weak_esft():
    u = probe.DirectorEsftUser()
    d = PyE()
    u.set_weak(d)
    gc.collect()
    r1 = f"expired={u.weak_expired()} apply={u.apply_weak(1)}"
    del d; gc.collect()
    return r1 + f" | after del: expired={u.weak_expired()}"
report("weak_ptr to Python subclass, enable_shared_from_this", weak_esft)
