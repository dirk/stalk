
from ast import S_List

class Scope(object):
    # Represents a specific scope within which expressions (normally from the
    # AST) are evaluated.
    
    def __init__(self):
        self.locals = {}
        self.parent = None
        self.root = None # Quick jump to the root scope.
    
    # Resolves an identifier
    def get(self, name):
        if name in self.locals:
            v = self.locals[name]
        elif self.parent:
            v = self.parent.get(name)
        # elif self.root != self:
        #    v = self.root.get(name)
        else:
            raise Exception("Name '"+name+"' not found")
        return v
        
    def get_local(self, name):
        if name in self.locals:
            return self.locals[name]
        else:
            return None
    
    def set(self, name, val):
        self.locals[name] = val
    def set_local(self, name, val):
        self.set(name, val)

class SymbolTable(object):
    def __init__(self):
        self.table = {}
        self.counter = 0
    def lookup(self, name):
        if name in self.table:
            return self.table[name]
        else:
            counter += 1
            self.table[name] = counter
            return counter

class RootScope(Scope):
    def __init__(self):
        self.symbols = SymbolTable()
        #super(RootScope, self).__init__()
        #RootScope.__init__(self)
        # Can't do super-stuff:
        self.locals = {}
        self.parent = None
        self.root = self

class Interpreter(object):
    def __init__(self, _list):
        self.root = RootScope()
        self.list = _list # S_List
        if self.list.__class__ != S_List:
            raise Exception("Expression list must be an S_List")
        from stalk.bootstrap import bootstrap
        bootstrap(self.root)
    def eval(self):
        self.list.eval(self.root)

