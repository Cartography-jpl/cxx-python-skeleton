import numpy as np

from skeleton import NumpySample

# Note that these tests should be run with valgrind, otherwise we don't know
# if the lifetimes are correct or not.
#
# A nice way to run this is with --free-fill=0xff in valgrind, so it forces
# the tests to fail (since the values get changed). Something like:
#
# PYTHONPATH=`pwd`:${PYTHONPATH} valgrind --log-file=valgrind.out --free-fill=0xff python `which py.test` -s tests/unit/numpy_sample_test.py
#
# Note that python has lots and lots of things that valgrind hates. You should
# ignore all of this - it is optimization stuff that the python code writers
# need to worry about. We stick all of this in a separate file, just so we can
# see the output. If you need to track something down, you can go ahead and
# write to the screen and then look for the string "Find me in valgrind" for
# where you can start paying attention to problems.
#
# Can also compile with flags to check lifetime:
#
# CXXFLAGS="-ggdb -DBZ_DEBUG -fbounds-check -DBZ_DEBUG_LOG_ALLOCATIONS -DBZ_DEBUG_LOG_REFERENCES"


def test_lifetime_return_array():
    """Return a blitz::array, and then make sure numpy still valid after
    original blitz::array goes away."""
    print("Find me in valgrind")
    t = NumpySample()
    a = t.data
    print("Deleting NumpySample")
    t = None
    print(a[0, 0])
    assert a[0, 0] == 1
    print("Deleting numpy array")
    a = None
    print("End of find me in valgrind")


def test_lifetime_passed_array():
    """Send a numpy array to C++, and make sure it is still a valid
    blitz::array when the numpy object goes away."""
    print("Find me in valgrind")
    a = np.array([[1.0, 2], [3, 4]])
    t = NumpySample(a)
    print("Deleting numpy array")
    a = None
    # Note, python doesn't actually free small objects, but keeps a pool
    # to reuse it. See
    # http://deeplearning.net/software/theano/tutorial/python-memory-management.html#internal-memory-management
    # Not really sure how this interacts with numpy, but if you monitor
    # a run of this (at least with the particular python/numpy version I
    # tested with) the array never actually goes away. As a work around,
    # reallocate a array of the same size. This seems to reuse the memory,
    # so we can indirectly test if t has an old bad reference.
    a = np.array([[4.0, 5], [6, 7]])
    print(t.data[0, 0])
    assert t.data[0, 0] == 1
    print("Deleting NumpySample")
    t = None
    print("End of find me in valgrind")


def test_lifetime_round_trip_array():
    """Send a numpy array to C++, and back out. Send a blitz::Array from
    C++ to python, and then back to C++"""
    a = np.array([[1.0, 2], [3, 4]])
    t = NumpySample(a)
    print("Deleting numpy array")
    a = None
    # Note, python doesn't actually free small objects, but keeps a pool
    # to reuse it. See
    # http://deeplearning.net/software/theano/tutorial/python-memory-management.html#internal-memory-management
    # Not really sure how this interacts with numpy, but if you monitor
    # a run of this (at least with the particular python/numpy version I
    # tested with) the array never actually goes away. As a work around,
    # reallocate a array of the same size. This seems to reuse the memory,
    # so we can indirectly test if t has an old bad reference.
    a = np.array([[4.0, 5], [6, 7]])
    print(t.data[0, 0])
    assert t.data[0, 0] == 1
    b = t.data
    print("Deleting NumpySample array")
    t = None
    print(b[0, 0])
    assert b[0, 0] == 1
    print("Deleting numpy array")
    b = None

    t = NumpySample()
    a = t.data
    print("Deleting NumpySample")
    t = NumpySample(a)
    print("Deleting numpy array")
    a = None
    print(t.data[0, 0])
    assert t.data[0, 0] == 1


def test_maybe_range():
    """Regression test for the boost::optional<blitz::Range> out typemap
    in swig_boost_optional.i. This used to check "if (&$1)" (the address
    of a local, which is never null) instead of "if ($1)" (whether the
    optional is engaged), so a disengaged optional would call .get() on
    itself, invoking undefined behavior, instead of returning None."""
    t = NumpySample()
    r = t.maybe_range(False)
    assert r is not None
    assert r.first() == 0
    assert r.last() == t.data.shape[0] - 1
    assert t.maybe_range(True) is None


def test_str():
    assert str(NumpySample()) == "NumpySample 2 x 2"
