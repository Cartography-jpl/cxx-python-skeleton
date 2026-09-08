# This defines fixtures that give the paths to various directories with
# test data
from pathlib import Path

import pytest


@pytest.fixture(scope="function")
def unit_test_data():
    return Path(__file__).parent.parent.parent.parent.absolute() / "unit_test_data"
