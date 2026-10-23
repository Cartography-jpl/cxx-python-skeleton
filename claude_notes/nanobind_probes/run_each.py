import re, subprocess, sys
src = open("probe_test.py").read()
header, *_ = src.split("# --- Downcasting")
# collect report(...) lines and their helper defs: run whole file but only one report enabled
lines = src.splitlines()
idx = [i for i, l in enumerate(lines) if l.startswith("report(")]
for k in idx:
    prog = "\n".join(l if (not l.startswith("report(") or i == k) else "pass" for i, l in enumerate(lines))
    r = subprocess.run([sys.executable, "-c", prog], capture_output=True, text=True, timeout=60)
    out = (r.stdout.strip().splitlines() or [""])[-1]
    if r.returncode != 0:
        name = re.match(r'report\("([^"]*)"', lines[k]).group(1)
        out = f"{name:55s} CRASH (exit {r.returncode})"
    print(out)
