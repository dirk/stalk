
import sys
import py



# print(sys.path)

def ensure_dep(path):
    dep = py.path.local(path)
    if not dep.check():
        print("Need pypy source!")
        exit()
    
    if not dep.__str__() in sys.path:
        sys.path.insert(0, dep.__str__())

ensure_dep("deps")
ensure_dep("deps/pypy")


from rpython.rlib.streamio import open_file_as_stream
import stalk.parser as parser
from stalk.interpreter import Interpreter

def read_file(filename):
    f = open_file_as_stream(filename)
    c = f.readall()
    f.close()
    return c

def entry_point(argv):
    import traceback
    
    try:
        filename = argv[1]
    except IndexError:
        print "You must supply a filename"
        return 1
    
    #with open(filename, 'r') as prog_file:
    #    prog = prog_file.read()
    prog = read_file(filename)
    
    # try:
    _list = parser.parse(prog)
    interp = Interpreter(_list)
    interp.eval()
    # except Exception as e:
    #     print e.__repr__()
    #     traceback.print_exc()
    #     if "getsourcepos" in dir(e):
    #         sp = e.getsourcepos()
    #         # def __init__(self, idx, lineno, colno):
    #         #     self.idx = idx
    #         #     self.lineno = lineno
    #         #     self.colno = colno
    #         print "idx: %d lineno: %d colno: %d" % (sp.idx, sp.lineno, sp.colno)
    #         print prog[sp.idx - 4:sp.idx + 4].__repr__()
    print interp
    print interp.root.locals
    return 0

def target(*args):
    # print args
    return entry_point, None

if __name__ == '__main__':
    entry_point(("stalk", sys.argv[1]))
    