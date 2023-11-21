from skeleton_swig.all import *
from test_support import *
import pickle

class DirectorPythonExample(DirectorExample):
    def __init__(self, v1, v2):
        super().__init__(v1)
        self.v2 = v2

    def func(self, v):
        return super().func(v) + 2 * self.v2

    def desc(self):
        '''Description to print for this object.'''
        return f'''DirectorPythonExample:
   value_to_add:        { self.value_to_add() }
   value_to_double_add: {self.v2}'''
    
def test_director_example():
    e = DirectorExample(5)
    assert e.func(10) == 5+10
    e2 = DirectorPythonExample(5,6)
    assert e2.func(10) == 5+10+2*6
    euser = DirectorExampleUser()
    euser.set_value(e2)
    assert euser.apply_func(10) == 5+10+2*6
    print(euser.value())
    
    
    
