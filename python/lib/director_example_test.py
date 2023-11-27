from skeleton_swig.all import *
from .swig_shelve import read_shelve, write_shelve
from test_support import *
import pickle
import sys

class DirectorPythonExample(DirectorExample):
    def __init__(self, v1, v2, name):
        super().__init__(v1, name)
        self.v2 = v2
        print(f"Creating Python {self.name()}", flush=True)

    def __del__(self):
        print(f"Destroying Python {self.name()}", flush=True)

    def print_ref_count(self):
        # We have an extra 2 counts because this got passed as an
        # arg to getrefcount, and we got this as a arg.
        print(f"Ref count {self.name()} {sys.getrefcount(self)-2}")
        
    def func(self, v):
        self.print_ref_count()
        return super().func(v) + 2 * self.v2

    def desc(self):
        '''Description to print for this object.'''
        return f'''DirectorPythonExample:
   name:                { self.name() }
   ref_count:           { sys.getrefcount(self)-2 }
   value_to_add:        { self.value_to_add() }
   value_to_double_add: {self.v2}'''
    
def test_director_example():
    print("May want to valgrind this, to make sure we have lifetime etc. ok with memory usage.")
    print("Do this with PYTHONMALLOC=malloc valgrind $(which python) $(which pytest) -s lib/director_example_test.py")
    print("(Note PYTHONMALLOC is important, otherwise you get a ton of spurious errors)")
    e1 = DirectorExample(5, "e1")
    assert e1.func(10) == 5+10
    e1 = None
    e2 = DirectorPythonExample(5,6, "e2")
    assert e2.func(10) == 5+10+2*6
    euser = DirectorExampleUser()
    euser.set_value(e2)
    e2 = None
    assert euser.apply_func(10) == 5+10+2*6
    print(euser.value())
    # Test a simple std::vector
    euser.set_vec_int([1,2,3])
    print(euser.value_vec_int())
    if True:
        # Test a std::vector of boost::shared_ptr
        v = [DirectorExample(5, "v1"),
             DirectorExample(6, "v2"),
             DirectorPythonExample(5,6, "vp3"),
             DirectorPythonExample(10,7, "vp4")]
    else:
        # Directly use std::vector. Shouldn't be needed any longer, passing
        # in a list works now. But leave this around in case we run into a
        # future issue we need to diagnose
        v = Vector_DirectorExample()
        v.push_back(DirectorExample(5, "v1"))
        v.push_back(DirectorExample(6, "v2"))
        v.push_back(DirectorPythonExample(5,6, "vp3"))
        v.push_back(DirectorPythonExample(10,7, "vp4"))
    euser.set_vec(v)
    v = None
    assert euser.apply_func(10, 0) == 5+10
    assert euser.apply_func(10, 1) == 6+10
    assert euser.apply_func(10, 2) == 5+10+2*6
    assert euser.apply_func(10, 3) == 10+10+2*7
    print(euser.value_vec()[0])
    print(euser.value_vec()[1])
    print(euser.value_vec()[2])
    print(euser.value_vec()[3])
    print("Should see e2 and v1 through vp4 deleted", flush=True)
    euser = None
    print("All Done", flush=True)

def test_director_pickle_shelve(isolated_dir):
    print("May want to valgrind this, to make sure we have lifetime etc. ok with memory usage.")
    print("Do this with PYTHONMALLOC=malloc valgrind $(which python) $(which pytest) -s lib/director_example_test.py")
    print("(Note PYTHONMALLOC is important, otherwise you get a ton of spurious errors)")
    e1 = DirectorExample(5, "e1")
    assert e1.func(10) == 5+10
    # This is tiny, but go ahead and compress just to test the interface
    write_shelve("e1.xml.gz", e1)
    e1_s = read_shelve("e1.xml.gz")
    pickle.dump(e1, open("e1.pkl", "wb"))
    e1_p = pickle.load(open("e1.pkl", "rb"))
    e1 = None
    assert e1_s.func(10) == 5+10
    assert e1_p.func(10) == 5+10
    print(e1_s)
    print(e1_p)
    e1_s = None
    e1_p = None
    euser = DirectorExampleUser()
    v = [DirectorExample(5, "v1"),
         DirectorExample(6, "v2"),
         DirectorPythonExample(5,6, "vp3"),
         DirectorPythonExample(10,7, "vp4")]
    euser.set_vec(v)
    print(euser.value_vec()[0])
    print(euser.value_vec()[1])
    print(euser.value_vec()[2])
    print(euser.value_vec()[3], flush=True)
    write_shelve("euser.xml", euser)
    euser_s = read_shelve("euser.xml")
    e2 = DirectorPythonExample(5,6, "e2")
    assert e2.func(10) == 5+10+2*6
    t = e2.this
    e2.this = None
    e2.this = t
    # Temp, try getting pickling working first. Might be able to
    # leverage this for C++ shelve
    write_shelve("e2.xml", e2)
    e2_s = read_shelve("e2.xml")
    #pickle.dump(e2, open("e2.pkl", "wb"))
    #e2_p = pickle.load(open("e2.pkl", "rb"))
    e2 = None
    print("hi 1")
    assert e2_s.func(10) == 5+10+2*6
    print("hi 2")
    #assert e2_p.func(10) == 5+10+2*6
    print(e2_s)
    print("hi 3")
    #print(e2_p)
    e2_s = None
    e2_p = None
    
    
    
    
    
    
    
