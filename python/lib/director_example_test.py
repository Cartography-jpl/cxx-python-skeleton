from skeleton_swig.all import *
from .swig_shelve import read_shelve, write_shelve
from test_support import *
import pickle
import sys
import gc
import ctypes

# For testing, this calls the low level python c API for decreasing the reference
# count. Can use to try to sort out issues with lifetime
_decref = ctypes.pythonapi.Py_DecRef
_decref.argtypes = [ctypes.py_object]
_decref.restype = None

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
        return super().func(v) + 2 * self.v2

    def desc(self):
        '''Description to print for this object.'''
        return f'''DirectorPythonExample:
   name:                { self.name() }
   ref_count:           { sys.getrefcount(self)-2 }
   value_to_add:        { self.value_to_add() }
   value_to_double_add: {self.v2}'''

    def print_obj():
        '''Print a list of active DirectorPythonExample, and the first one if it
        exists. Useful when sorting through lifetimes issues.'''
        olist = [obj for obj in gc.get_objects() if isinstance(obj, DirectorPythonExample)]
        print(f"Found in active objects: {olist}", flush=True)
        if(len(olist) > 0):
            print("First object in active objects:")
            if(olist[0].this is not None):
                print(olist[0])
                print(gc.get_referents(olist[0].this), flush=True)
                print(sys.getrefcount(olist[0].this))
            else:
                print(sys.getrefcount(olist[0]))
            print("Stuff referring to this:")
            print(gc.get_referents(olist[0]), flush=True)
    
    
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
    e2 = DirectorPythonExample(5,6, "e2")
    assert e2.func(10) == 5+10+2*6
    write_shelve("e2.xml", e2)
    e2_s = read_shelve("e2.xml")
    pickle.dump(e2, open("e2.pkl", "wb"))
    e2_p = pickle.load(open("e2.pkl", "rb"))
    e2 = None
    assert e2_s.func(10) == 5+10+2*6
    assert e2_p.func(10) == 5+10+2*6
    print(e2_s)
    print(e2_p)
    e2_s = None
    e2_p = None
    
    euser = DirectorExampleUser()
    v = [DirectorExample(5, "v1"),
         DirectorExample(6, "v2"),
         DirectorPythonExample(5,6, "vp3"),
         DirectorPythonExample(10,7, "vp4")]
    euser.set_vec(v)
    v = None
    write_shelve("euser.xml", euser)
    euser_s = read_shelve("euser.xml")
    pickle.dump(euser, open("euser.pkl", "wb"))
    euser_p = pickle.load(open("euser.pkl", "rb"))
    euser = None
    assert euser_s.apply_func(10, 0) == 5+10
    assert euser_s.apply_func(10, 1) == 6+10
    assert euser_s.apply_func(10, 2) == 5+10+2*6
    assert euser_s.apply_func(10, 3) == 10+10+2*7
    assert euser_p.apply_func(10, 0) == 5+10
    assert euser_p.apply_func(10, 1) == 6+10
    assert euser_p.apply_func(10, 2) == 5+10+2*6
    assert euser_p.apply_func(10, 3) == 10+10+2*7
    print(euser_s.value_vec()[0])
    print(euser_s.value_vec()[1])
    print(euser_s.value_vec()[2])
    print(euser_s.value_vec()[3])
    print(euser_p.value_vec()[0])
    print(euser_p.value_vec()[1])
    print(euser_p.value_vec()[2])
    print(euser_p.value_vec()[3])
    euser_s = None
    euser_p = None
    print("All done", flush=True)

def test_director_lifetime_shelve(isolated_dir):
    print("Should see creation message", flush=True)
    e1 = DirectorPythonExample(5,6, "e1")
    write_shelve("e1.xml", e1)
    print("Should see deletion message", flush=True)
    e1 = None
    print("All done with e1", flush=True)
    e1_s = read_shelve("e1.xml")
    print(e1_s)
    print("Should see deletion message", flush=True)
    e1_s = None
    print("All done with e1_s", flush=True)
    print("Should see creation message", flush=True)
    euser = DirectorExampleUser()
    euser.set_vec([DirectorPythonExample(5,6, "e2"),])
    write_shelve("euser.xml", euser)
    print("Should see deletion message", flush=True)
    euser = None
    print("All done with euser")
    euser_s = read_shelve("euser.xml")
    print(euser_s.value_vec()[0])
    print("Should see deletion message", flush=True)
    euser_s = None
    print("All done with euser_s")

    

    
    
    
