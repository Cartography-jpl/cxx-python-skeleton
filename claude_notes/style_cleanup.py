"""One-time formatting cleanup of the skeleton's C++ and .i files.

Run from the repository root. See the S1.6 row in
claude_notes/implementation_plan.md for what and why.
"""
import re
import subprocess

files = subprocess.run(
    ["git", "ls-files", "lib/*.h", "lib/*.cc", "lib/*.i", "swig_rules/lib/*.h",
     "swig_rules/lib/*.cc", "swig_rules/lib/*.sample", "swig_rules/include/*.i"],
    capture_output=True, text=True, check=True).stdout.split()

# Upstream forks: left exactly as they are, so they stay diffable against
# SWIG's boost_shared_ptr.i and Boost's serialization/shared_ptr.hpp.
forks = {"swig_rules/include/my_shared_ptr.i", "swig_rules/lib/serialize_shared_ptr.h"}
# Written in their upstream's style on purpose (Python C-API docs module
# init, SWIG's container traits): whitespace cleanup only.
whitespace_only = {"swig_rules/lib/python_lib_init.h",
                   "swig_rules/include/swig_vector_shared_ptr.i"}


def must(s, a, b, n=1):
    assert s.count(a) == n, (a, s.count(a))
    return s.replace(a, b)


for f in files:
    if f in forks:
        continue
    s = open(f).read()
    orig = s
    lines = [ln.expandtabs(8).rstrip() for ln in s.split("\n")]
    s = "\n".join(lines).rstrip("\n") + "\n"
    if f not in whitespace_only:
        s = re.sub(r"\b(if|for|while|switch|catch) \(", r"\1(", s)
        s = re.sub(r"\btemplate <", "template<", s)
        s = s.replace("Archive & ar", "Archive& ar")
        s = s.replace("const unsigned int )", "const unsigned int)")
        s = re.sub(r"\bPyObject \*([A-Za-z_])", r"PyObject* \1", s)
        s = re.sub(r"\b(void|char) \*(p|buf|rescp)\b", r"\1* \2", s)
        s = s.replace("boost::shared_ptr<T> *v3", "boost::shared_ptr<T>* v3")
        s = s.replace("boost::shared_ptr<NAMESPACE::TYPE> *smartresult",
                      "boost::shared_ptr<NAMESPACE::TYPE>* smartresult")
        s = s.replace("(Swig::DirectorException &e)", "(Swig::DirectorException& e)")
        if f == "swig_rules/lib/python_exception.h":
            s = must(s, "  mutable void *type, *value, *tb;\n",
                     "  mutable void* type;\n  mutable void* value;\n  mutable void* tb;\n")
        if f == "swig_rules/lib/weak_ptr_serialize_support.h":
            s = s.replace("boost::weak_ptr< T > &t", "boost::weak_ptr<T>& t")
            s = s.replace("boost::shared_ptr< T >", "boost::shared_ptr<T>")
        if f == "lib/unit_test_support.h":
            s = must(s, "BOOST_CHECK_MATRIX_CLOSE_TOL( A, B, tol )",
                     "BOOST_CHECK_MATRIX_CLOSE_TOL(A, B, tol)")
            s = must(s, "BOOST_CHECK_MATRIX_CLOSE( A, B )", "BOOST_CHECK_MATRIX_CLOSE(A, B)")
        if f == "swig_rules/include/director_serialization.i":
            s = must(s, "     void serialize(Archive& ar, SwigDirector_ ## TYPE& D, "
                     "const unsigned int /* version */) {\n",
                     "     void serialize(Archive& ar, SwigDirector_ ## TYPE& D,\n"
                     "                    const unsigned int /* version */)\n     {\n")
        if f == "swig_rules/include/swig_boost_array.i":
            s = must(s, "    blitz::Array<TYPE, 1> _v_ ## NAME() const {\n",
                     "    blitz::Array<TYPE, 1> _v_ ## NAME() const\n    {\n")
            s = must(s, "    void _v_ ## NAME(const blitz::Array<TYPE, 1>& V) {\n",
                     "    void _v_ ## NAME(const blitz::Array<TYPE, 1>& V)\n    {\n")
        if f == "swig_rules/include/swig_array.i":
            # Three copies of a call whose arguments had tabs in the middle
            # of the line (after expandtabs: long runs of spaces).
            s, n = re.subn(
                r"PyArray_SetBaseObject\(\(PyArrayObject\*\)( ?)([^,]+),\s*"
                r"SWIG_NewPointerObj\(SWIG_as_voidptr\(t\),\s*"
                r"\$descriptor\(blitz::Array<TYPE, DIM>\*\),\s*"
                r"SWIG_POINTER_NEW \| SWIG_POINTER_OWN \)\);",
                r"PyArray_SetBaseObject((PyArrayObject*)\1\2,\n"
                r"                        SWIG_NewPointerObj(SWIG_as_voidptr(t),\n"
                r"                          $descriptor(blitz::Array<TYPE, DIM>*),\n"
                r"                          SWIG_POINTER_NEW | SWIG_POINTER_OWN));", s)
            assert n == 3, n
    if s != orig:
        open(f, "w").write(s)
