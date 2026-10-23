import sys, subprocess
code = {
 "raw shared_ptr<virtual base>: type": "print(type(p.make_igc_raw()).__name__)",
 "raw shared_ptr<virtual base>: call nline()": "print(p.make_igc_raw().nline())",
 "mapped shared_ptr<virtual base>: type + nline() + psize()": "o=p.make_igc_mapped(); print(type(o).__name__, o.nline(), o.psize())",
 "mapped shared_ptr<WithParameter> (2nd virtual branch)": "o=p.make_withparam_mapped(); print(type(o).__name__, o.nline(), o.psize())",
 "pass derived to shared_ptr<virtual base> param": "print(p.take_igc(p.IgcRollingShutter()))",
}
mod = sys.argv[1]
for k, c in code.items():
    r = subprocess.run([sys.executable, "-c", f"import {mod} as p\n{c}"], capture_output=True, text=True)
    out = r.stdout.strip() if r.returncode == 0 else (f"CRASH (exit {r.returncode})" if r.returncode < 0 else "ERROR: " + r.stderr.strip().splitlines()[-1][:110])
    print(f"  {k:58s} {out}")
