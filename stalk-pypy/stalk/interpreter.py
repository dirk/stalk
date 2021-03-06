
from ast import S_List

from stalk.object import SL_Object

class SL_Scope(SL_Object):
    # Represents a specific scope within which expressions (normally from the
    # AST) are evaluated.
    
    def __init__(self):
        self.locals = {}
        self.parent = None
        self.root = None # Quick jump to the root scope.
        self.methods = {}
    
    def set_parent(self, parent):
        self.parent = parent
    
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
    # Override this to search the scope hierarchy
    def get_signatures(self):
        sigs = [m.get_signature() for m in self.methods.values()]
        if self.parent:
            sigs.extend(self.parent.get_signatures())
        return sigs
    # Override too for scope hierarchy
    def send(self, sig, params): # Receive (compiled) signature and data
        if sig in self.methods.keys():
            return self.methods[sig].call(self, params)
        else:
            if self.parent:
                return self.parent.send(sig, params)
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

class SL_Root_Scope(SL_Scope):
    def __init__(self):
        self.symbols = SymbolTable()
        #super(SL_Root_Scope, self).__init__()
        #SL_Root_Scope.__init__(self)
        # Can't do super-stuff:
        self.locals = {}
        self.parent = None
        self.root = self
        self.methods = {}

class Interpreter(object):
    def __init__(self, _list):
        self.root = SL_Root_Scope()
        self.list = _list # S_List
        if self.list.__class__ != S_List:
            raise Exception("Expression list must be an S_List")
        from stalk.bootstrap import bootstrap
        bootstrap(self.root)
    def eval(self):
        self.list.eval(self.root)

