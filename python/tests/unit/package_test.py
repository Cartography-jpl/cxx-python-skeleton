import pytest


@pytest.mark.xfail(strict=True, reason="T5: __all__ lists only python names")
def test_star_import():
    """from skeleton import * gives the C++ classes as well as the python
    code (review 2026-11-07 third, section 3.5)."""
    ns = {}
    exec("from skeleton import *", ns)
    assert "IostreamSwig" in ns
    assert "read_shelve" in ns
