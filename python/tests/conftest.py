import os
import sys
from pathlib import Path

import pytest

# Add source to path. For some reason pytest can miss this even if we
# have this installed editable with a --prefix pip install (probably
# some weird PYTHONPATH interaction not worth tracking down when we can
# just work around it).
sys.path.append(str(Path(os.path.dirname(__file__)).parent))

# Short hand for marking as unconditional skipping. Good for tests we
# don't normally run, but might want to comment out for a specific debugging
# reason.
skip = pytest.mark.skip

# Marker for slow tests. Only run with --run-slow
slow_test = pytest.mark.slow_test


def pytest_addoption(parser):
    parser.addoption("--run-slow", action="store_true", help="run slow tests")


def pytest_collection_modifyitems(config, items):
    if not config.getoption("--run-slow"):
        skip_slow_test = pytest.mark.skip(reason="need --run-slow option to run")
        for item in items:
            if "slow_test" in item.keywords:
                item.add_marker(skip_slow_test)


# ------------------------------------------
# Includes fixtures, made available to all tests.
# ------------------------------------------

pytest_plugins = [
    "fixtures.dir_fixture",
    "fixtures.misc_fixture",
]
