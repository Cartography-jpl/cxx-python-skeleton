import sys, subprocess
pre = """import gc, probe7 as p
class PyC(p.Camera):
    def number_band(self): return 77
def t(make, strong_first=False):
    h = p.Holder(); o = make()
    if strong_first: h.set_strong(o)
    h.set_weak(o); gc.collect()
    r = f"expired={h.expired()} apply={h.apply()}"
    del o; gc.collect()
    return r + f" | after del: expired={h.expired()}"
"""
code = {
 "shared_ptr<GenericObject> (virtual) -> type, call": "o=p.make_generic(); print(type(o).__name__, o.number_band(), o.psize())",
 "shared_ptr<WithParameter> (2nd virtual branch)": "o=p.make_withparam(); print(type(o).__name__, o.number_band(), o.psize())",
 "same C++ object returned twice -> same Python object": "c=p.make_camera(); print(p.identity_generic(c) is c)",
 "weak_ptr: object created in C++": "print(t(p.make_camera))",
 "weak_ptr: Python-constructed, C++ holds strong ref": "print(t(p.QuaternionCamera, True))",
 "weak_ptr: Python subclass, C++ holds strong ref": "print(t(PyC, True))",
 "weak_ptr: Python-constructed, only Python holds it": "print(t(p.QuaternionCamera))",
}
for k, c in code.items():
    r = subprocess.run([sys.executable, "-c", pre + c], capture_output=True, text=True)
    out = r.stdout.strip() if r.returncode == 0 else (f"CRASH (exit {r.returncode})" if r.returncode < 0 else "ERROR: " + r.stderr.strip().splitlines()[-1][:110])
    print(f"  {k:56s} {out}")
