# Just import any files we find in this directory, rather than listing
# everything.

# ***** Note
# This init is just used for local testing. In particular, it doesn't load the
# cython stuff. There is another __init__ file found at ../init/__init__.py that
# is used for the full installed library. The purpose of this init it to
# be able to locally run tests and do development without installing the full
# system.

from __future__ import absolute_import
import os
import re
import glob
# See https://github.com/swig/swig/issues/2881#issuecomment-2332652634
# As of Swig 4.3, have warning in pytest that we can't suppress. This
# might get fixed, they are currently showing this in Swig 4.4 as one of
# the scheduled items. But do this in the short term
import warnings
with warnings.catch_warnings():
    warnings.simplefilter("ignore")
    from skeleton_swig import *

for i in glob.glob(os.path.dirname(__file__) + "/*.py"):
    mname = os.path.basename(i).split('.')[0]
    # Don't load ipython, which is ipython magic extensions, or unit tests
    if(not mname == "ipython" and
       not mname == "cython_try" and
       not re.search('_test', mname)):
        exec("from .%s import *" % mname)
