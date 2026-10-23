# Serialization options (and whether cereal is a good choice)

Status: **analysis only, no code changes.** Based on reading `swig_rules/lib/{serialize_shared_ptr.h,
weak_ptr_serialize_support.h, serialization_context.*, serialize_function.*}`, `swig_rules/include/{swig_pickle.i,
director_serialization.i}`, and `lib/*.cc` `serialize()` methods. Facts about cereal/Bitsery/etc. are from my knowledge of
those libraries, not from building against them here. Items marked **(verify)** are the ones I'm least sure of and
would check in a spike.

## Short answer on cereal

**Plausible, but not an obvious win, and not something I'd adopt on its own merits.** It is attractive for being small,
header-only and `std::`-native. But this repo's serialization is not "plain serialization". It leans on three Boost features
that cereal doesn't have or does differently:

1. **Polymorphic (runtime-selected) archives.** cereal has none (all archives are compile-time types).
2. **A hook into how `shared_ptr` is *loaded*** (for director objects). cereal's `shared_ptr` loader can't be
   subclassed the way `boost::serialization::shared_ptr_helper` can be.
3. **Boost's per-address object tracking** (also tracks by-value members, not only pointers).

None of these is fatal, but each needs a redesign, so cereal is a *rewrite of the serialization framework*, not a swap.
If the goal is mainly to get rid of Boost, cereal is a reasonable target. If the goal is to reduce the amount of
machinery, there is a more interesting option (D below) that removes much of it by leaning on python's own pickle.

## 1. What the current system actually does (requirements)

Understanding this is what decides the answer, so here it is explicitly:

| # | Requirement | How it is met today |
|---|---|---|
| R1 | Serialize graphs of `shared_ptr<GenericObject>` with **runtime polymorphism** | `BOOST_CLASS_EXPORT_KEY/IMPLEMENT`, `void_cast_register` |
| R2 | **Shared identity within one archive**: two pointers to one object come back as one object (`GenericObjectMap` test, `serialization2`) | Boost object tracking |
| R3 | **No identity across separate archives** (serializing `a` and `b` separately yields two copies of `a`; tested in `serialization1`) | Natural consequence of R2 |
| R4 | **Runtime choice of format**: XML (text/debug), binary (strings, pickle), gz-compressed files | `polymorphic_{xml,binary}_{i,o}archive`, so each `serialize()` is instantiated once for two polymorphic archive types, not once per concrete archive |
| R5 | **Custom `weak_ptr` policy**: keep a `weak_ptr` only if its target is also in the archive. Done as *two passes* (a "mark" pass into a throwaway binary archive, then the real pass) | Own `save/load` for `boost::weak_ptr` + `SerializationContextSingleton::ptr_referenced` set |
| R6 | **Python pickle bridge**: `__reduce__` calls `serialize_write_pickle` which gives a C++ binary blob, and `serialize_read_pickle` reverses it | `swig_pickle.i`, `serialize_function.cc` |
| R7 | **Re-entrancy between C++ and python**: a C++ object holds a python-subclass (director) object that holds C++ objects ... all inside *one* boost archive **and** one python `Pickler`, so identity is preserved across the boundary | `SerializationContextSingleton` holds the active `polymorphic_*archive*`, the stream, and the python pickler; `save_construct_data` / `load_construct_data` for `SwigDirector_*` call back into python `serialization_dumps/loads` |
| R8 | **Director objects load correctly**: when loading a `shared_ptr<T>` that turns out to be a python-subclass director, install the python-ref-counted deleter | The `our_shared_ptr` subclass trick (a hand-copied `boost/serialization/shared_ptr.hpp`) |
| R9 | Non-default-constructible classes with private ctors | `friend class boost::serialization::access` |
| R10 | Array (blitz) serialization with size header + flat data | 391 lines of `.cc`, 5 ranks x 5 types |
| R11 | Class versioning | `BOOST_CLASS_VERSION` via `SKELETON_CLASS_VERSION` (present as a macro; not used heavily) |

Two observations that matter:
- R7 and R8 are **specific to SWIG + directors**. If the binding layer changes (nanobind), these two are redone anyway,
  *whatever the serialization library is*. Don't count them as cereal-vs-boost costs.
- The copy of `shared_ptr.hpp` in `serialize_shared_ptr.h` is a **fragility today**: it includes
  `boost/serialization/shared_ptr_helper.hpp` and relies on boost internals (`BOOST_SP_NOEXCEPT`, `mpl::integral_c_tag`).
  Boost has been reorganizing those internals across releases, so a Boost upgrade is the likeliest thing to break it. **(verify
  against the Boost version you intend to target.)**

## 2. Options

### A. Stay on Boost.Serialization, modernize around it
- Change to `std::shared_ptr`/`std::weak_ptr`/`std::optional`. Boost.Serialization has `std::shared_ptr` support in
  `<boost/serialization/shared_ptr.hpp>` in recent versions **(verify which version and whether `shared_ptr_helper` works with `std::`)**.
- Keep polymorphic archives, so R4, R5 and R6 carry over almost unchanged.
- Replace the hand-copied `shared_ptr` save/load with a smaller hook if possible. The need goes away if directors are
  handled elsewhere (see nanobind note below).
- Pros: lowest risk, file and pickle formats keep working (if the class layout is unchanged), keeps the existing
  tests as the acceptance suite, no new dependency (Boost is already required for other reasons here). The code is mature,
  and Boost is maintained.
- Cons: Boost dependency and heavy template compile times, boost-internal hacks remain, XML archive is slow and verbose.

### B. cereal
What it gives:
- Header-only, small, no linking, C++11 and later. Packaged on conda-forge.
- Built-in support for `std::shared_ptr`, `std::unique_ptr`, `std::weak_ptr`, `std::optional`, `std::variant`, `std::map`,
  `std::vector`, `std::array`, `std::string`, and others.
- Archives: binary, portable binary, JSON, XML. JSON is a real improvement over Boost's XML for debuggability.
- `shared_ptr` identity tracking within an archive (R2, R3 behave as now).
- Polymorphism with `CEREAL_REGISTER_TYPE` (and `_WITH_NAME`, which plays the role of `BOOST_CLASS_EXPORT_KEY2`) and
  `CEREAL_REGISTER_POLYMORPHIC_RELATION` / `cereal::base_class` / `cereal::virtual_base_class`.
- Class versions via `CEREAL_CLASS_VERSION`, private members via `friend class cereal::access`.
- Private default constructors work (like R9). `load_and_construct` exists for classes with no default ctor.

How it maps onto the requirements:

| Req | cereal | Notes |
|---|---|---|
| R1 | Yes | Registration macros must be in a TU that has seen *all* archive headers you want to use. In static libraries, registrations can be dropped by the linker (`CEREAL_REGISTER_DYNAMIC_INIT` is the workaround). Libtool shared libraries are fine. **(verify with the project's one-big-`.so` layout)** |
| R2, R3 | Yes, for `shared_ptr` | Tracking is by pointer, so a by-value member plus a `shared_ptr` to the same object are **not** unified (boost's by-address tracking does that). Not used in the skeleton's tests. |
| R4 | **No** | No polymorphic archive. Each `serialize()` has to be instantiated for each concrete archive (explicit instantiation in the `.cc`, like `SKELETON_IMPLEMENT` does now for 2 archives; it would do e.g. XML/JSON + binary in/out = 4). Run-time choice of format is then a small dispatch function (`switch(fmt)`), which is fine for a closed list of formats. |
| R5 | Possible, with work | Provide your own `weak_ptr` save/load, but cereal's `<cereal/types/memory.hpp>` already defines one for `std::weak_ptr`, so use a small wrapper type or tag instead of fighting an ambiguous overload. The two-pass mark scheme itself is independent of the library (the mark pass can write to a binary archive over a null stream, as now). |
| R6 | Yes | The python blob just holds a cereal binary archive. Needs a concrete archive type in the context singleton (not `polymorphic_*archive*`). |
| R7 | Possible, redesigned | The singleton would hold a concrete `BinaryOutputArchive*`/`BinaryInputArchive*` (or a pair of function objects that type-erase it). The "python object nested in a C++ archive" idea does not depend on boost. |
| R8 | **The hard one** | cereal's `shared_ptr` loader constructs the `shared_ptr` itself; there is no helper to subclass. For director-created objects you either (a) specialise `load_and_construct` for those types so the construction returns a python-owned object, (b) wrap in a custom pointer type (changes every `shared_ptr<T>` member that can hold a director), or (c) fix this on the binding side after loading. With nanobind's `std::shared_ptr` handling the "second shared_ptr with python-refcount deleter" problem may not exist at all, so (c) may be natural. **(verify in the nanobind spike)** |
| R9 | Yes | `cereal::access` |
| R10 | Easier | `cereal::binary_data` (for binary archives) and generic loops (for text). Eigen has third-party cereal support. For blitz or xtensor, write one function per type. A single rank-templated shape + data function would replace the five hand-expanded overloads. |
| R11 | Yes | `CEREAL_CLASS_VERSION`, with the version passed as the 2nd arg of `serialize()` as now. |

Costs:
- **File-format break**: nothing written by boost archives can be read, and nothing already pickled (which embeds a boost
  binary blob) can be loaded. If long-lived data files exist downstream, this is the biggest argument against it, or for a
  one-shot converter written against the old code.
- Every `serialize()` method is touched. Mechanical: `ar & SKELETON_NVP_(v)` becomes `ar(cereal::make_nvp("v", v_))`.
  `ar & x` may also compile, but `operator()` is the idiom **(verify)**.
- The `.am`/configure plumbing for `--without-boost-serialization` and `HAVE_BOOST_SERIALIZATION` is replaced by cereal
  header detection.
- cereal is stable but low-activity (last release I know of is 1.3.2, 2022). Fine for a header-only library with a frozen
  design, but you would be depending on an infrequently updated project.
- No built-in gzip/compression (it is stream-based, so layering a compressing stream is the same approach as the current
  `OstreamCompress`, which uses boost.iostreams; that dependency would remain unless you swap in zlib or `zstr`).

### C. Other C++ serialization libraries

| Library | Fit | Why / why not |
|---|---|---|
| **Bitsery** | Weak-to-medium | Very fast, compact binary only. Has pointer ownership, polymorphism, and `std::` containers, but is binary-only (no XML/JSON for debugging), and you configure much more by hand (pointer linking contexts, polymorphic registration). Better for performance-critical data than for this. |
| **nlohmann::json / Boost.JSON, hand-written** | Weak for graphs | Great readability, but no pointer tracking, no polymorphism. You would build R1/R2 yourself. Acceptable if object graphs are shallow or trees. |
| **glaze / reflect-cpp (C++20 reflection-lite)** | Weak | No automatic `serialize()` writing needed, but no shared-identity handling. |
| **Protobuf / FlatBuffers / Cap'n Proto** | Poor fit | Schema-first, designed for messages, not graphs of polymorphic C++ objects with shared pointers. |
| **C++26 static reflection (P2996)** | Future | Will allow generating `serialize()` automatically. Not available in compilers you can use now. Worth tracking, not planning around. |
| **Boost.Serialization alternatives (Cista, zpp::bits)** | Poor | Optimized for flat, trivially copyable data. |

I don't see anything else that provides R1 + R2 + (runtime) R4 as completely as Boost, and only cereal comes close.

### D. Make python's `pickle` do the graph work (pickle-only, drop C++ archives)
With nanobind (and also possible in SWIG), each exposed class can implement `__getstate__` / `__setstate__` (or
`__reduce__`) that returns a tuple of its members, where members that are `shared_ptr<X>` are returned **as python
objects**. Python's pickle already:
- memoizes by object identity inside one `pickle.dumps` call, giving **R2 and R3 for free** (shared objects come back
  shared, separate dumps give copies, as in the existing `test_generic_object_map`),
- handles polymorphism, since the python object knows its own class (R1),
- handles python subclasses (directors) naturally, because they are python objects with state (R7 and R8 vanish: no C++
  archive nested inside a python pickler, no `save_construct_data`/`load_construct_data`, no `our_shared_ptr`),
- deals with versioning through your `__setstate__`.

Prerequisite: nanobind maps one C++ pointer to one python wrapper (it keeps a pointer-to-instance map **(verify)**), so
returning the same `shared_ptr` twice gives the same python object, which is what makes the memo work. The SWIG code
today needs the second shared_ptr trick precisely because it does not have that property.

What you give up:
- C++-only persistence (XML/binary/gz files read and written from C++ with no python). If downstream uses these (the
  `serialize_write("file.xml")` family), D alone isn't enough. A reduced version of A or B could remain for the C++ file
  path, as an add-on rather than the base of pickling.
- The custom `weak_ptr` rule (R5, "keep only if target is also in the archive"): in pickle, the equivalent is to have
  `__getstate__` return the weak reference's target and decide on inclusion by checking a per-dump context set,
  likely via a `copyreg`/`persistent_id` hook. Doable, but it moves the policy to python.
- Needs one `__getstate__` + `__setstate__` per class (or a reflection-based generic one), and objects with private data
  need accessors. That is roughly the same amount of per-class code as today's `serialize()` methods, but simple.

### E. Hybrid (my suggested direction if you want to simplify)
- Python pickle uses D (python-level, no C++ archive involved).
- C++ file persistence, if still needed, uses A (existing Boost, modernized) or B (cereal), with *no* python bridge and
  *no* director support in the C++ archive.
- This removes R6, R7 and R8 from the C++ library entirely. `SerializationContext*`, the `polymorphic_*archive*` pointers,
  the nested `SerializationToString`, and the `director_serialization.i` macro all disappear. What remains for C++ is
  simple, library-independent, and testable without python.

## 3. Comparison

| | A. Boost (modernized) | B. cereal | C. others | D. pickle-native | E. hybrid |
|---|---|---|---|---|---|
| Keeps existing file/pickle format | Likely | No | No | No | C++ files: depends on A/B; pickle: no |
| Amount of new framework code | Least | Medium | Large | Small | Small |
| Runtime format choice | Built in | Dispatch function | Varies | N/A | per half |
| Shared identity | Built in | Built in (shared_ptr only) | Hand-built | Built in (pickle memo) | Built in |
| Custom weak_ptr policy | Keep as is | Reimplement | Hand-built | Reimplement (python side) | per half |
| Directors | Special hook, keep or reimplement | Special hook, harder | Hard | Free | Free for pickle |
| Dependencies | Boost (already) | cereal | varies | none | varies |
| Python needed to use it | No | No | No | **Yes** | C++ half: No |
| Risk | Low | Medium (no polymorphic archives, shared_ptr hook) | High | Medium (relies on nanobind identity map) | Medium |

## 4. Recommendation

1. **Don't pick the serialization library yet.** The big driver is what you do about bindings (R7, R8 depend on it).
   The decision sequence that makes sense: `std::shared_ptr` conversion, then the nanobind spike, then decide.
2. **In the nanobind spike, test D directly** with the three existing scenarios (`GenericObjectMap` with shared `A`, `weak_ptr`
   case, `DirectorExample` subclass in python). If nanobind returns the same python object for the same `shared_ptr`
   and pickle memoization works, most of the framework in `swig_rules/lib` becomes unnecessary for pickle.
3. **For C++-side files, if they matter downstream:**
   - If format continuity matters or there are old files: keep **A** (Boost), converted to `std::shared_ptr`.
   - If not, and you want to be rid of Boost and get JSON: **B** (cereal) is reasonable. Do a spike of just
     `GenericObjectA/B/C` + `GenericObjectMap`, covering (i) shared identity, (ii) the two-pass `weak_ptr` rule through a
     wrapper type, and (iii) explicit instantiation across XML/JSON + binary in a shared library.
4. If you do try cereal, these are the things that decide it:
   - Can the `weak_ptr` mark-pass policy be expressed without fighting `<cereal/types/memory.hpp>`?
   - Does explicit instantiation of `serialize()` per archive work cleanly with registration in the one big shared
     library?
   - How does the director case (R8) end up after the nanobind work?

## 5. Wider survey, and is Boost still the right answer?

Section 2 covered the realistic candidates. This is the wider landscape, grouped by approach. As elsewhere, this is from
my knowledge of the libraries, not from building against them here.

| Approach | Examples | Shared pointers, polymorphism, identity? | Verdict here |
|---|---|---|---|
| Object-graph serializers (you write `serialize()` per class) | **Boost.Serialization**, **cereal**, Bitsery | Boost: yes, fully. cereal: yes, with the caveats in section 2. Bitsery: partly, binary only. | The only real candidates. |
| Reflection-driven (no per-class code) | **glaze**, **reflect-cpp**, Boost.Describe + Boost.JSON, Boost.PFR | No. They serialize trees of values, not graphs with shared identity. | Nice for plain data structs, wrong for `shared_ptr` graphs. |
| Flat or zero-copy binary | Cista, zpp::bits, Bitsery | Mostly no polymorphism or identity. | Built for speed on plain data. |
| Schema-first messaging | Protobuf, FlatBuffers, Cap'n Proto, Avro | No. They model messages, not C++ object graphs. | Poor fit. |
| Format libraries | nlohmann/json, Boost.JSON, RapidJSON, pugixml, yaml-cpp, HDF5 | No, the graph logic is yours to build. | Useful as the backend of a custom layer. |
| Python-side | `pickle` with `__getstate__`, `jsonpickle` (already in the pixi env), `cloudpickle` | Yes, python's pickle memo handles identity. | This is option D above. |
| C++26 static reflection (P2996) | Experimental compiler forks only | Could generate `serialize()` bodies, but not identity tracking. | Worth watching. It might eventually remove the per-class boilerplate, but it isn't usable now. |
| Roll your own thin layer | A `serialize(Archive&)` visitor over JSON or binary | You build exactly what you need. | Essentially reimplements cereal (about 500 lines). Only worth it if cereal's `shared_ptr`/`weak_ptr` behavior proves unworkable. |

### Is Boost still the best choice?
For C++ object-graph persistence with shared identity, runtime-selected formats, and a custom load hook: **yes.** Nothing
else matches all of those, and cereal is the only close one. Boost.Serialization is still maintained and ships with every
Boost release, though development is slow and mostly one person.

Its weaknesses are real but are not about capability:
- compile times;
- the verbose XML;
- the Boost internals that `serialize_shared_ptr.h` copies and depends on. This is a fragility in this repo's code, not a
  flaw in Boost, and it could break when Boost reorganizes those headers.

### Ways to shrink the Boost footprint
- **Remove everything except serialization.** `serialize_function.cc` also uses `boost::regex`, `boost::filesystem` and
  `boost::iostreams` (for gzip). `std::regex` and `std::filesystem` are direct replacements for the first two. gzip needs
  zlib or a small wrapper. The tests could also leave Boost.Test.
- **Keep Boost.Serialization for C++ files only.** Moving python pickling onto python's own `pickle` (option D) removes the
  nested-archive, director and `shared_ptr` hacks. What remains is a small, ordinary use of Boost.Serialization.

### Suggested path
Stay on Boost.Serialization and convert to `std::shared_ptr`. Use the nanobind spike to see whether pickle-native
serialization works. Revisit cereal only if you want JSON files, or if Boost becomes a real maintenance problem.

## 6. Does this change anything about autotools?
No. Serialization options are all header-only or Boost, and `configure.ac` only needs a header check (cereal) or the
existing Boost check. See the revised build-system section in `modernization_ideas.md`: nanobind can be built under
autotools without CMake.

## 7. What the geocal and framework surveys change

Both consumers (`/home/smyth/Local/geocal-repo`, `/home/smyth/Local/framework`) were surveyed; see section 10 of
`modernization_ideas.md` for the full comparison. The points that bear on serialization:

| | geocal | framework |
|---|---|---|
| Files with `serialize()`/`save`/`load` | ~190 | ~274 (~290 `serialize`, 45 `save`/`load` pairs) |
| Export macros | ~290 `GEOCAL_EXPORT_KEY`, ~254 `GEOCAL_IMPLEMENT` | ~325 `FP_EXPORT_KEY`, ~301 `FP_IMPLEMENT` |
| `%pickle_serialization` | ~245 | many (same mechanism) |
| Polymorphic archives (xml, binary, text) | yes | yes |
| `weak_ptr` scheme | Observer/Observable, via `weak_ptr_serialize_support` | **central**: `Observer<T>` / `Observable<T>` with `FP_OBSERVER_SERIALIZE`; `Observable` serializes `olist`, `colist`, `ref_list` |
| Directors that must pickle | 9 | 47 (not all necessarily pickled; not determined) |
| Pickle used for multiprocessing | **yes** (`multiprocessing.Pool` in `parallel_process.py`) | no (none found) |
| Pickle/shelve persistence | `sqlite_shelf.py`, `job_database.py` (long-term contents not confirmed) | `swig_shelve.py` |
| Persisted archives in test data | a few `.xml` | 9 `*.bin.gz` configuration fixtures + XML, ~119 `serialize_read/write` uses in 34 files |
| Format stability | CLAUDE.md says old Ecostress objects were knowingly broken | not determined |

### Effects on the earlier conclusions
1. **Option D (pickle-native) is much less attractive as the base.** It needs per-class `__getstate__` / `__setstate__`
   for ~700 exposed classes, replacing ~480 files whose `serialize()` methods already exist, are tested, and are the
   single source of truth. I previously suggested trying D first; given these numbers, **I'd now only use it for the
   nested/director part**, where the current machinery is most fragile, and keep the C++ `serialize()` methods as the
   pickle payload (as today).
2. **cereal is a bigger rewrite than I described for the skeleton**: about 480 files of `serialize()` and export macros,
   a regenerated test-fixture set, and a redesign of the weak_ptr scheme. The two repos' shared `GEOCAL_*` / `FP_*`
   macro layer is a mitigating factor: if every class goes through `X_EXPORT_KEY` / `X_IMPLEMENT` / `X_NVP_`, much of the
   change could be made in those macros, not in each class. That's testable cheaply in the skeleton, whose
   `SKELETON_*` macros are the same pattern. It would still need the fixtures regenerated and the weak_ptr
   scheme reimplemented.
3. **The weak_ptr scheme is a real feature, not a corner case**, particularly in framework (Observer/Observable, as
   documented in `weak_ptr_serialize_support.h`). Any alternative to Boost has to reproduce the two-pass "keep a
   `weak_ptr` only if its target is in the archive" rule. That was the cereal sticking point and it remains one.
4. **Boost cannot be dropped anyway** (math, lexical_cast, spirit, iostreams, date_time, ...), so replacing Boost.Serialization
   does not remove the dependency. The usual motivation for cereal (shed Boost) mostly disappears.
5. **Multiprocessing pickling in geocal** means pickle correctness and cost are exercised in production, not just
   in persistence. Any change to the pickle path needs a multiprocessing test; the current binary-blob-in-pickle design
   is at least known to work.
6. **Format stability is probably negotiable in geocal** (per its CLAUDE.md), but framework's `.bin.gz`/XML fixtures
   would need regenerating if the archive format changed, a one-time cost rather than a blocker.

### Revised recommendation
- **Stay on Boost.Serialization.** Convert to `std::shared_ptr` (the serialization headers need an update for this), keep the
  `X_EXPORT_KEY` macro layer, and keep `serialize()` methods unchanged.
- **Keep the pickle bridge on the C++ blob** for ordinary classes. Redo only the director and nested-context handling for
  nanobind, where `std::shared_ptr` handling may make the `our_shared_ptr` hack unnecessary.
- **Try D only for director classes** in the spike.
- **Revisit cereal only if** (a) Boost.Serialization breaks on a Boost upgrade (the copied `shared_ptr.hpp` is the likely
  trigger), or (b) JSON output becomes a requirement. In that case, prototype it by changing the macro layer in
  the skeleton first and measure how much of the per-class code actually has to change.

## 8. Long-term archives exist: effect on the recommendation

You confirmed that some long-term archives exist and that backward compatibility is not strict but definitely wanted. This
settles the Boost-vs-cereal question for now:
- **Stay on Boost.Serialization.** Changing library means a format break; with archives to keep, it would need a one-time
  converter built against the old code, plus the weak_ptr-scheme reimplementation and the ~480-file rewrite described in
  section 7. No benefit identified so far justifies that.
- **Make the `std::shared_ptr` conversion archive-neutral and prove it.** Keep the repo's own `shared_ptr` save/load (element
  name `"px"`, class version 1, `track_never`) and change only the pointer type. Do not switch to Boost's built-in
  `std::shared_ptr` support without checking that it writes the same bytes.
- **Build a golden-archive corpus first** (framework's `.bin.gz` and XML fixtures, geocal's `.xml`, plus real archives),
  and run it before and after every step. See `modernization_ideas.md` section 10.5.
- **Watch the `GenericObject` hierarchy change**: altering virtual inheritance or the registered base can change what
  is written for the base-object records. The corpus will show it.
- **Pickles** carry the same blob plus a Python module/class path; a name-alias shim is needed if the nanobind module
  layout differs from the SWIG one, and it should be tested against a pickle from the current SWIG build.
- A future format change (cereal or otherwise) should ship with a converter. That is a project in itself, so only do it for
  a concrete benefit.

### 8a. Refinement: the long-term archives are Boost XML; pickle is secondary
- **Corpus**: XML only (framework's `.bin.gz` fixtures are test data and can be regenerated). Details of what an XML file
  depends on (NVP names, export keys, class versions, tracking/object ids, base-object records, array element names, archive
  header version) are in `modernization_ideas.md` section 10.5a.
- **Staying on Boost is still the lowest-risk choice**, and it is reinforced: XML archives only break from changes to
  those items, not from nanobind or build changes. A newer Boost reads older archives, so Boost upgrades are safe for old
  files.
- **XML makes a converter cheap.** If a rename, class move, or even a library change (e.g. cereal's different XML layout)
  were ever needed, an XSLT or Python/lxml transform of the old files is much easier than a C++ converter. That lowers the
  cost of future changes, but it does not change today's recommendation.
- **Pickle** is allowed to break across the SWIG-to-nanobind change, so the module-alias shim is optional. This simplifies
  director handling: the nested-pickle machinery (`SerializationContext`, `save_construct_data` / `load_construct_data`
  for directors) only needs to work for the new build, not be wire-compatible with the old one.
- **Blitz replacement caution**: if arrays are ever swapped for another library, the array save/load must keep the same
  element names (`rows`, `cols`, `data`, ...) and order, otherwise old XML with arrays stops loading.

### 8b. File age and acceptable breakage
The XML files that matter are about a year old or less, older ones matter less, and some breakage is acceptable if it is
clearly identified. So: a rolling compatibility window (last ~year), a small corpus per release, loud and attributable load
failures (class, member or version named), and a recorded list of intentional breaks with class-version bumps. See
`modernization_ideas.md` section 10.5b. This reduces the cost of any future format change (a year of files converts in
one pass), but does not change the recommendation to stay on Boost.Serialization for now.

## 9. Update 2026-10-23 (geocal planning and nanobind probes)

See `implementation_plan.md` for the plan of record. Changes relevant to serialization:
- **Staying on Boost.Serialization is confirmed** for geocal and framework.
- **`GenericObject` keeps its virtual inheritance** (the non-virtual rework is dropped), so the base-object /
  `void_cast` records in archives do not change. That removes the second compatibility risk in 8 / 10.5.
- **`GenericObject` gains `std::enable_shared_from_this<GenericObject>`** for nanobind `weak_ptr` correctness. This adds a
  member to every object but is not serialized, so archives are unaffected. Verify with the golden corpus.
- **R5 (two-pass `weak_ptr` rule)** stays in C++. Under nanobind, `weak_ptr`s set from Python only behave if some C++
  `shared_ptr` owns the target (probe results, `implementation_plan.md` §2.1). The Observer lists must move from
  `to_ptr(*this)` to `weak_from_this()`.
- **R6–R8 (pickle bridge, nested C++/Python identity, director loading)** are redesigned for nanobind in phase S6.1. The
  unfinished SWIG-side re-entrant pickling work on `master` is frozen; its tests become `xfail(strict=True)` acceptance
  tests for the nanobind version. Ordinary classes keep the C++ binary blob in `__reduce__`.
- **geocal uses pickling with `multiprocessing.Pool` in production**, so a multiprocessing round-trip test is part of the
  S6 acceptance suite.
- **`--without-boost-serialization`** is kept but currently broken (4 files here, 12 in geocal); fix it and build it
  regularly.
