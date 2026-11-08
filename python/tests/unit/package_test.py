def test_star_import():
    """from skeleton import * gives the C++ classes as well as the python
    code (review 2026-11-07 third, section 3.5)."""
    ns = {}
    exec("from skeleton import *", ns)  # noqa: S102 (a star import needs a namespace)
    assert "IostreamSwig" in ns
    assert "read_shelve" in ns
