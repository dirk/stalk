
import sys
import py

# print(sys.path)

deps_pypy = py.path.local("deps/pypy/")
if not deps_pypy.check():
    print("Need pypy source!")
    exit()

if not deps_pypy in sys.path:
    sys.path.append(py.path.local("deps").__str__())

import stalk.parser as parser

def main(argv):
    #if not len(argv) == 2:
    #    print "Error"
    #    return 1
    return 0

if __name__ == '__main__':
    #main(sys.argv)
    #     src = """
    # testing = testing
    # """
    #src = "testing = testing\n5 testing\nkeyword: test\n"
    #print src.__repr__()
    try:
        prog = py.path.local("example.sl").read()
        #prog = "one \\\ntwo three\nfour\n(five)\n"
        
        print parser.parse(prog)
    except Exception as e:
        print e.__repr__()
        if "getsourcepos" in dir(e):
            sp = e.getsourcepos()
            # def __init__(self, idx, lineno, colno):
            #     self.idx = idx
            #     self.lineno = lineno
            #     self.colno = colno
            print "idx: %d lineno: %d colno: %d" % (sp.idx, sp.lineno, sp.colno)
            print prog[sp.idx - 4:sp.idx + 4].__repr__()

# def main(argv):
#     if not len(argv) == 2:
#         print __doc__
#         return 1
#     f = open_file_as_stream(argv[1])
#     data = f.readall()
#     f.close()
#     interpret(data)
#     return 0
# 
# def target(driver, args):
#     return main, None
# 
# def jitpolicy(driver):
#     return JitPolicy()
#     
# 
# if __name__ == '__main__':
#     main(sys.argv)
# 