# This contains support routines for unit tests.
import numpy as np
from numpy.testing import assert_almost_equal, assert_approx_equal
from unittest import SkipTest
import warnings
with warnings.catch_warnings():
    warnings.simplefilter("ignore")
    from skeleton_swig.all import *
import os.path
import os
import sys
import subprocess
import pytest
import filelock
import contextlib

# Marker that skips a test if we have a build without boost serialization
# support
# Temp
#require_serialize = pytest.mark.skipif(not have_serialize_supported(),
#    reason="need a geocal build with boost serialization support to run")

# Short hand for marking as unconditional skipping. Good for tests we
# don't normally run, but might want to comment out for a specific debugging
# reason.
skip = pytest.mark.skip

# Some tests are python 3 only. Don't want the python 2 tests to fail for
# python code that we know can't be run
require_python3 = pytest.mark.skipif(not sys.version_info > (3,),
       reason = "require python 3 to run")

@pytest.fixture(scope='session')
def lock(tmp_path_factory):
    base_temp = tmp_path_factory.getbasetemp()
    lock_file = base_temp.parent / 'serial.lock'
    yield filelock.FileLock(lock_file=str(lock_file))
    with contextlib.suppress(OSError):
        os.remove(path=lock_file)


# Tests with serial all run in serial, needed for our lifetime tests
# because we have a class level variable to keep track of the number
# of objects deleted
@pytest.fixture()
def serial(lock):
    with lock.acquire(poll_interval=0.1):
        yield
        
