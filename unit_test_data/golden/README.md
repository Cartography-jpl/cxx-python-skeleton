# Golden archives

Boost serialization archives (XML and binary) written by the code as it was
at the start of the modernization (phase S0, 2026-10-23, boost 1.90, `boost::shared_ptr`, `boost::weak_ptr`).

Later phases (`std::shared_ptr`, `std::array`, `std::optional`, nanobind)
must still be able to **read** these files, and (S4) writing the same
objects must give byte-identical XML (`golden_archive/rewrite_identical` checks every object archive, XML and binary). The tests are `lib/golden_archive_test.cc`
(C++) and `python/tests/unit/golden_archive_test.py` (through `read_shelve`).

Don't regenerate these to make a test pass. Regenerating is a deliberate act
that changes what we promise to be able to read. If you have to (for example
a new class is added to the corpus), run

    make golden-archives

in the build directory (which runs `golden_archive_generator`, built from
`lib/golden_archive_generator.cc`), review the diff, and update the date above.
New files should be added in the generator and both tests together.

Contents:

| Files | Object |
|---|---|
| `generic_object_a` | `GenericObjectA` (v=7) |
| `generic_object_b` | `GenericObjectB` with a shared `A` |
| `generic_object_b2` | `GenericObjectB2` (`A` held by value, v=8) |
| `generic_object_c` | `GenericObjectC` (`weak_ptr` to an `A` not otherwise serialized, reads back null) |
| `generic_object_map` | `GenericObjectMap` with `a`, `b` and `c` sharing one `A` (c weak), and a `B2` |
| `director_example`, `director_example_nested`, `director_example_user` | the C++-only `DirectorExample*` objects |
| `director_example_map` | map holding a `DirectorExample`, a nested one sharing it, and a `DirectorExampleWeakPtr` to it |
| `blitz_{double,int,bool}_array_{1..5}d`, `blitz_string_array_{1,2}d` | `blitz::Array` of rank 1-5, element value `i` or `-i` (strings `s<i>`), extents 3,2,2,2,2 |
| `blitz_{float,uint,ushort,short}_array_{1..5}d` | Same, for the other element types. **Added 2026-10-30** (phase ST), when their `BOOST_SERIALIZATION_SPLIT_FREE` was added to `blitz_array_serialize_support.h`; none of the files above changed. (For the unsigned types `-i` wraps around, which is fine, it is the same on read.) |
| `array_sample`, `array_sample_empty` | `ArraySample`: `boost::array<double,3>`, a nested `boost::array<boost::array<double,3>,2>`, and `boost::optional<int>`/`<double>`, all set (values differ from the defaults), and with both optionals empty. **Added 2026-11-05** (phase S3.1), written with the `boost::` types before S3.2 converts them to `std::array`/`std::optional`; none of the files above changed. |
| `observer_map`, `observer_ctor` | `ObservableExample` with observers: `ctor` (registered in its constructor and again in `load()`), `plain` (`add_observer`), `kept` (`add_observer_and_keep_reference`), `dropped` (not in the archive, so not kept), and a `CacheExample`, in a `GenericObjectMap`; and the `ctor` observer on its own (it brings the observable, with `ctor` and `kept`). **Added 2026-11-06** (phase S5), with `boost::shared_ptr`; none of the files above changed. |
| `simple_camera_like`, `diamond` | `SimpleCameraLike` (a `Printable`, an `Observable` and a `WithParameterLike` base) and `Diamond` (virtual diamond). **Added 2026-11-06** (phase S5). |

The arrays are written with the element name `array` and read directly (not
through a `GenericObject`), so they are checked only by the C++ test.
