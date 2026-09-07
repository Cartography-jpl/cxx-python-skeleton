# Just import any files we find in this directory, rather than listing
# everything.

from __future__ import absolute_import
import os as _os
import re as _re
import glob as _glob

# See https://github.com/swig/swig/issues/2881#issuecomment-2332652634
# As of Swig 4.3, have warning in pytest that we can't suppress. This
# might get fixed, they are currently showing this in Swig 4.4 as one of
# the scheduled items. But do this in the short term
import warnings as _warnings
with _warnings.catch_warnings():
    _warnings.simplefilter("ignore")
    from skeleton_swig.all import *

for _i in _glob.glob(_os.path.dirname(__file__) + "/*.py"):
    _mname = _os.path.basename(_i).split('.')[0]
    # Don't load ipython, which is ipython magic extensions, or unit tests
    if(not _mname == 'ipython' and
       not _re.search('_test', _mname)):
        exec("from .%s import *" % _mname)

del _os
del _re
del _glob
del _warnings
del _i
del _mname
