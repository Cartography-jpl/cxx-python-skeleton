# Build system / code layout review

Scope: the repo's build tooling (autotools) and top-level code layout, as it
stands after the recent `python/` modernization (pip/pyproject.toml +
`tests/{unit,fixtures}`). Python layout itself isn't re-litigated here —
see the earlier restructuring for that.

## Snapshot of what's actually here

- Autotools (autoconf/automake/libtool), **single non-recursive top-level
  Makefile** assembled via `include *.am` directives from `skeleton.am`,
  `lib/lib.am`, `python/python.am`, `bindings/python/python.am`,
  `swig_rules/swig_rules.am`, `script/script.am`, `unit_test_data/unit_test_data.am`.
- `swig_rules/` and `config/m4` are `git subtree`s from separate shared repos
  (`swig-rules-repo`, `autoconf-repo`).
- SWIG generates all C++↔python bindings; a locally-patched `my_shared_ptr.i`
  and custom director/weak_ptr/serialization lifetime code
  (`swig_rules/lib/DirectorNotes.md`) is the real engineering asset here.
- Custom `AFIDS_PYTHON`/`AC_BOOST`/`AC_BLITZ`/`AC_DOXYGEN` m4 macros, mixed
  with stock autoconf ones.
- `env/linux/pixi.toml` + **committed `pixi.lock`** is the reproducible dev
  toolchain (python, boost, blitz, compilers, doxygen, swig, pytest, etc.).
- No CI config anywhere in the repo (checked for `.github/`, `.gitlab-ci.yml`,
  other `*.yml`/`*.yaml` — none found).
- `swig_rules/CMakeLists.txt` exists but is a **fragment** meant to be
  `add_subdirectory()`'d into some other project's own CMake build — it
  defines no targets and can't build this repo standalone. Autotools is the
  only real build path.

## Pros — things that are working well, not accidental debt

1. **Single non-recursive Makefile.** Avoids the classic recursive-make
   correctness/parallelism problems (Peter Miller's "Recursive Make
   Considered Harmful"). Genuinely above-average autotools hygiene.
2. **Proven on a real downstream consumer.** `geocal-repo` is built the same
   way at real scale — this isn't a toy setup.
3. **`git subtree` over submodule** for `swig_rules`/`config/m4` avoids the
   most common submodule footguns (forgetting `--recurse-submodules`,
   detached-HEAD commits) while still letting fixes flow both directions.
4. **The SWIG lifetime/director/serialization code is a hard-won asset**, not
   boilerplate — `PythonRefPtrCleanup`, the `SHARED_PTR_NO_OWN` typemap trick,
   and the director exception-propagation fix are exactly the kind of thing
   that's easy to get subtly wrong and expensive to re-derive.
5. **Good fast-loop escape hatches**: `--without-swig`, `--without-documentation`,
   `--without-boost-serialization` configure flags give a real inner dev loop
   without touching the underlying tooling.
6. **Reproducible dev toolchain is actually committed** (`env/linux/pixi.toml`
   + `pixi.lock`), not just documented in prose.
7. **Python packaging just modernized** (pip/pyproject.toml, `tests/{unit,fixtures}`)
   removes the worst pain point (per-file `.am` listing for every new `.py`).

## Cons — real pain points

1. **Autotools itself is a 20+ year old toolchain** with a steep onboarding
   cost (m4 indirection, `AC_SUBST`/`AC_DEFUN` opacity). `./bootstrap` already
   emits its own obsolescence warnings on this exact repo:
   `AC_PROG_LIBTOOL` is obsolete (should be `LT_INIT`), and libtoolize
   suggests `-I config/m4` in `ACLOCAL_AMFLAGS`. These aren't hypothetical —
   they printed during this session's `./bootstrap` run.
2. **`AM_INIT_AUTOMAKE([1.9 tar-pax])`** pins compatibility to automake 1.9
   (released 2005) with no evident reason to still support automake that old.
   Note `1.9` and `tar-pax` are independent options here, not a package —
   `tar-pax` (comment in `config/m4_local/skeleton_common.m4` /
   `config/m4/afids_common.m4` cites the ustar 99-char filename limit) is
   still worth keeping even though no path in *this* repo comes close to 99
   chars (longest is 56): it's shared via subtree into `geocal` and any future
   skeleton-derived project that might have deeper paths, and pax has no real
   downside on modern tar/automake. Only the `1.9` pin itself looks stale.
3. **Bespoke python-compat shims are a recurring tax.** The `distutils`
   removal saga documented in `CLAUDE.md` (`ac_python.m4`,
   `afids_python.m4` rewritten to use `sysconfig`/`packaging`/`numpy.get_include()`)
   is exactly the maintenance burden that a modern build backend
   (`meson-python`, `scikit-build-core`) absorbs for free, since those track
   CPython ABI/sysconfig changes upstream instead of in project-local m4.
4. **The flat `.am`-file namespace is fragile.** `skeleton.am`/`lib.am`/
   `python.am`/`swig_rules.am`/`bindings/python.am` all share one Makefile
   variable namespace; correctness depends entirely on naming discipline
   (`skeletonpkgpython`, `skeletoninc_HEADERS`, ...). This isn't theoretical —
   this session found and fixed a pre-existing dangling
   `install-python-skeleton` target referencing a nonexistent
   `install-pynitf` rule, silently broken since some past refactor.
5. **No CI.** Build/test correctness relies entirely on developers
   remembering to run `make check`/`make installcheck` locally before
   pushing. Given point 4, that's a real risk — the dangling-target bug above
   would have been caught immediately by a CI job running `make install`.
6. **SWIG is aging relative to pybind11/nanobind** for C++↔Python bindings:
   less active upstream investment specifically in the python target,
   slower compiles from generated glue code, and typemap-based lifetime
   tricks that pybind11's native `shared_ptr` support and trampoline classes
   handle more directly. (See Tier 3 below for why this isn't a "just do it.")
7. **`git subtree` push friction is self-documented as a problem** —
   `CLAUDE.md`'s own subtree section describes a known "tip of current
   branch is behind" failure mode requiring a manual `git subtree split`
   workaround.
8. **No CMake build actually exists for this repo**, despite `CLAUDE.md`
   calling it "an alternate packaging path" — `swig_rules/CMakeLists.txt`
   only collects variable lists for a *consumer's* CMake tree to use; there's
   no top-level `CMakeLists.txt` here at all.

## Modernization options, tiered by risk/effort

### Tier 1 — low risk, do first
- **Add CI** (GitHub Actions or similar) running
  `./bootstrap && configure && make && make check && make install && make installcheck`
  against the locked pixi env. This is the single highest-leverage change —
  it converts "hope it still builds" into a fact, and would have caught the
  dangling-target bug found this session automatically. **Explicitly out of
  scope for this repo** per project owner — CI belongs in the downstream
  repos (e.g. `geocal`) that actually consume this skeleton, not the
  skeleton itself.
- ~~Run `autoupdate`~~ — **done**. Ran clean, no changes needed to
  `configure.ac` itself.
- ~~Reconsider the automake-1.9 compat pin~~ — **done**. Dropped `1.9` from
  `AM_INIT_AUTOMAKE([1.9 tar-pax])` in both `config/m4/afids_common.m4` and
  `config/m4_local/skeleton_common.m4`; kept `tar-pax` (independent option,
  still worth having for downstream consumers with deeper paths — see cons
  item 2 below).
- ~~Clear the `AC_PROG_LIBTOOL` obsolescence warning~~ — **done**. Swapped
  `AC_PROG_LIBTOOL` → `LT_INIT` in both macro files above.
  `ACLOCAL_AMFLAGS = -I config/m4` was already present in `common.am`, so
  nothing to add there. Verified via `./bootstrap` (obsolescence warning
  gone) and a full `configure` run (libtool commands execute fine).

### Tier 2 — medium effort, real payoff, needs a prototype before committing
- **Evaluate `meson-python`/`scikit-build-core`** as the build backend behind
  `python/pyproject.toml`, replacing the current split where `pip install .`
  only installs pure-python `skeleton` and the SWIG extension is a separate
  autotools-built artifact installed alongside it. A unified build would let
  `pip install .` alone produce a working, importable package — no
  `./configure && make && make install` dance for python-only consumers.
  This is scoped to the packaging layer only (the C++ library itself doesn't
  need to move), so it's a much smaller bet than a full autotools→CMake
  migration.
- **Re-evaluate the `git subtree` sharing mechanism** for `swig_rules`/
  `config/m4` given the self-documented push friction — options include git
  submodules (different, not obviously fewer, footguns) or packaging
  `swig_rules` as an installable/versioned artifact consumed by both
  `geocal` and `swig-rules-skeleton`. Worth a real cost-benefit discussion,
  not an assumed win — subtree's "no separate clone step" property is
  genuinely valuable and submodules would give that up.

### Tier 3 — large effort, high uncertainty, only with a forcing function
- **Full autotools → CMake migration.** The existing `swig_rules/CMakeLists.txt`
  is a fragment, not a working build — this would be from-scratch, not
  "finish what's there." `geocal`, the one real downstream consumer, would
  need to migrate too or the two builds permanently diverge — this is a
  cross-repo coordination cost, not a skeleton-only decision.
- **SWIG → pybind11/nanobind.** High value in the abstract, but very high
  risk *specifically here* because the codebase's core value is the
  hard-won director/shared_ptr/weak_ptr/serialization lifetime logic built
  on SWIG's typemap model. Porting that to pybind11's different ownership
  model is a multi-week research-and-validate effort, not a mechanical
  translation — every lifetime edge case in `DirectorNotes.md` would need
  independent re-verification. Only worth pursuing if SWIG becomes an actual
  blocker (e.g., can't support a needed C++/Python version), not as
  modernization for its own sake.

## Recommendation

This repo is a *template* — its whole purpose is to be bootstrapped into new
projects, and its one real consumer (`geocal`) mirrors its build setup.
That argues for a conservative sequencing:

1. **Do Tier 1 now.** Cheap, no behavior change, immediately de-risks
   everything after it (CI in particular would have caught real bugs this
   session surfaced by hand).
2. **Prototype Tier 2's packaging-backend idea on a branch** and measure
   whether it actually simplifies python-only builds enough to justify
   propagating the change to every downstream project derived from this
   skeleton. Don't commit until proven.
3. **Leave Tier 3 alone** absent an actual forcing function. The
   lifetime-management code is the crown jewel of this codebase; changing
   its foundation should be deliberate, not routine modernization.
