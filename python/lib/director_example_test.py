from skeleton_swig.all import *
from test_support import *
import pickle

class DirectorPythonExample(DirectorExample):
    def __init__(self, v1, v2, name):
        super().__init__(v1, name)
        self.v2 = v2
        print(f"Creating Python {self.name()}", flush=True)

    def __del__(self):
        print(f"Destroying Python {self.name()}", flush=True)
        
    def func(self, v):
        return super().func(v) + 2 * self.v2

    def desc(self):
        '''Description to print for this object.'''
        return f'''DirectorPythonExample:
   name:                { self.name() }
   value_to_add:        { self.value_to_add() }
   value_to_double_add: {self.v2}'''
    
def test_director_example():
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
    euser.set_vec([DirectorExample(5, "v1"),
                   DirectorExample(6, "v2"),
                   DirectorPythonExample(5,6, "vp3"),
                   DirectorPythonExample(10,7, "vp3")])
    assert euser.apply_func(10, 0) == 5+10
    assert euser.apply_func(10, 1) == 6+10
    # Currently failing
    #assert euser.apply_func(10, 2) == 5+10+2*6
    #assert euser.apply_func(10, 3) == 10+10+2*7
    if True:
        print(euser.value_vec()[0])
        print(euser.value_vec()[1])
        print(euser.value_vec()[2])
        print(euser.value_vec()[3])
    euser.set_vec_int([1,2,3])
    print(euser.value_vec_int())
    euser = None
    print("All Done", flush=True)
    
    
