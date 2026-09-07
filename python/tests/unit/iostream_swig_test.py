from test_support import *
from skeleton import IostreamSwig
from io import BytesIO

def test_iostream_write():
    '''Test writing to a python file handle like an iostream. This checks the
    handling by swig of the mapping between the two.'''
    iostream_swig = IostreamSwig("hello world")
    fh = BytesIO()
    fh.write(b"start string ")
    iostream_swig.write(fh)
    fh.write(b" end string")
    assert fh.getvalue() == b"start string hello world end string"

def test_iostream_read():
    '''Test reading from a python file handle like an iostream. This checks the
    handling by swig of the mapping between the two.'''
    iostream_swig = IostreamSwig("hello world", 2)
    start_string = b"start string "
    content = b"hello world"
    end_string = b"end string"
    fh = BytesIO(start_string + content + b" " + end_string)
    fh.read(len(start_string))
    assert iostream_swig.read(fh).encode('utf-8') == content
    assert fh.tell() == len(start_string) + len(content + b" ")
    assert fh.read() == end_string

def test_iostream_read_to_eof():
    '''Regression test for python_fh_device::read: reading exactly to the
    end of the underlying python file handle used to report a full read
    even when python's read() returned fewer bytes (or none, at EOF),
    which could feed garbage bytes into the stream or leave the C++ side
    unable to see EOF at all. This reads a stream with no trailing
    content, so the last extraction has to read right up against EOF.'''
    iostream_swig = IostreamSwig("hello world", 2)
    content = b"hello world"
    fh = BytesIO(content)
    assert iostream_swig.read(fh).encode('utf-8') == content
    assert fh.read() == b""
    
        
    
    
