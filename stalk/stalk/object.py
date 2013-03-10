from stalk.ast import S_Operator, S_Identifier, S_Keyword

def compile_signature(sig):
    #parts = []
    #for e in sig:
    #    if type(e) == S_Operator or type(e) == S_Identifier or type(e) == S_Keyword:
    #        parts.append(e.get_value())
    #return "".join(parts)
    if len(sig) == 1:
        s = sig[0]
        if type(s) == S_Identifier:
            return s.get_value()
        else:
            raise Exception("Unexpected signature: "+repr(sig))
    elif len(sig) == 2 and type(sig[0]) == S_Operator:
        return sig[0].get_value()
    else:
        parts = []
        for s in sig:
            if type(s) == S_Keyword:
                parts.append(s.get_value())
        return "".join(parts)

class SL_Object(object):
    def __init__(self):
        self.values    = {}
        self.methods   = {}
        self.prototype = None # Parent object (prototype)
        self.name      = "Object"
    def get_name(self):
        return self.name
    def get_values(self):
        return self.values
    def get_prototype(self):
        return self.prototype
    def get_methods(self):
        return self.methods
    def get_signatures(self):
        return [m.get_signature() for m in self.methods.values()]
    def receive(self, signature):
        return None # TODO: Make this work
    def define_method(self, method):
        self.methods[method.get_compiled_signature()] = method
    def send(self, sig, params): # Receive (compiled) signature and data
        if sig in self.methods.keys():
            return self.methods[sig].call(self, params)
        else:
            return None

class SL_Primitive(SL_Object):
    def get_values(self):
        return {}
    def get_methods(self):
        return self.methods
    # All these are overridden as necessary to actually return values
    def get_value_int(self):
        raise NotImplementedError("Called unavailable primitive method: get_value_int()")
    def get_value_string(self):
        raise NotImplementedError("Called unavailable primitive method: get_value_string()")
    def get_value_array(self):
        raise NotImplementedError("Called unavailable primitive method: get_value_array()")
    def get_value_block(self):
        raise NotImplementedError("Called unavailable primitive method: get_value_block()")

array_methods = {}
class SL_Array(SL_Primitive):
    def __init__(self, value):
        self.value = None
        self.prototype = None
        self.methods = array_methods
        self.value = value
    def get_name(self):
        return "Array"
    def get_prototype(self):
        return self.prototype
    def get_values(self):
        return {}
    def get_value(self):
        return self.value
    def get_value_array(self):
        return self.value
    def promote(self):
        raise NotImplementedError("Not implemented")
    def send(self, sig, params): # Receive (compiled) signature and data tuples
        if sig in self.methods.keys():
            return self.methods[sig].call(self, params)
        else:
            return None

class SL_Null(SL_Primitive):
    def __init__(self):
        self.value = None
        self.prototype = None
        self.methods = {}
    def get_name(self):
        return "Null"
    def get_prototype(self):
        return self.prototype
    def get_values(self):
        return {}
    def get_value(self):
        return self.value
    def promote(self):
        raise NotImplementedError("Not implemented")
    def send(self, sig, params): # Receive (compiled) signature and data tuples
        if sig in self.methods.keys():
            return self.methods[sig].call(self, params)
        else:
            return None

class SL_Exception(SL_Primitive):
    def __init__(self, desc):
        self.value = desc
        self.prototype = None
        self.methods = {}
    def get_name(self):
        return "Exception"
    def get_prototype(self):
        return self.prototype
    def get_values(self):
        return {}
    def get_value(self):
        return self.value
    def promote(self):
        raise NotImplementedError("Not implemented")
    def send(self, sig, params): # Receive (compiled) signature and data tuples
        if sig in self.methods.keys():
            return self.methods[sig].call(self, params)
        else:
            return None

# Is filled with primitive methods for integers at the bottom of this file.
int_methods = {}
class SL_Int(SL_Primitive):
    def __init__(self, value):
        self.value = int(value)
        self.prototype = None
        self.methods = int_methods
        
    def get_value_int(self):
        return self.value
    def get_value(self):
        return self.value
    def get_value_string(self):
        return str(self.value)
    def __repr__(self):
        return "SL_Int:"+str(self.value)
    def get_name(self):
        return "Int"
    def promote(self):
        # TODO: Make this work for promoting to a regular object.
        raise NotImplementedError("Not implemented")
    def send(self, sig, params): # Receive (compiled) signature and data tuples
        if sig in self.methods.keys():
            return self.methods[sig].call(self, params)
        else:
            return None

string_methods = {}
class SL_String(SL_Primitive):
    def __init__(self, value):
        self.value = value
        self.prototype = None
        self.methods = string_methods
    def get_value(self):
        return self.value
    def get_value_string(self):
        return self.value
    def get_name(self):
        return "String"
    def __repr__(self):
        return self.value
    def promote(self):
        # TODO: Make this work for promoting to a regular object.
        return NotImplementedError("Not implemented")
    def send(self, sig, params): # Receive (compiled) signature and data tuples
        if sig in self.methods.keys():
            return self.methods[sig].call(self, params)
        else:
            return None

from stalk.interpreter import SL_Scope

block_methods = {}
class SL_Block(SL_Primitive):
    def __init__(self, block, closure):
        self.block = block # S_Block
        self.closure = closure # SL_Scope
        self.methods = block_methods
    def get_value_block(self):
        return self.block
    def get_closure(self):
        return self.closure
    def __repr__(self):
        return "SL_Block"
    def get_name(self):
        return "Block"
    def is_primitive(self):
        return False
    def send(self, sig, params): # Receive (compiled) signature and data tuples
        if sig in self.methods.keys():
            return self.methods[sig].call(self, params)
        else:
            return None



class SL_Method(SL_Object):
    def __init__(self, signature, block, closure):
        # Signature:
        #   For "a"
        #     (S_Identifier(a))
        #   For "a: b"
        #     (S_Keyword(a:), S_Identifier(b))
        #   For "a: b c: d"
        #     (
        #       S_Keyword(a:), S_Identifier(b),
        #       S_Keyword(c:), S_Identifier(d)
        #     )
        #   For "+ b"
        #     (S_Operator(+), S_Identifier(b))
        # self.parent = parent # The SL_Object the method is bound to.
        self.block = block # SL_Block
        self.signature = signature # [...]
        # self.methods = { self.get_compiled_signature(): self }
        self.methods = {}
        self.closure = closure
        self.compiled_signature = compile_signature(self.signature)
    def is_primitive(self):
        return False
    def get_closure(self):
        return self.closure
    def get_signature(self):
        return self.signature
    def get_compiled_signature(self):
        return self.compiled_signature
    def _params(self):
        p = []
        for s in self.signature:
            if type(s) == S_Identifier:
                p.append(s)
        return p
    def call(self, parent, args):
        s = SL_Scope()
        s.set_parent(self.closure)
        s.set_local("this", parent)
        i = 0
        params = self._params()
        while i < len(params):
            s.set_local(params[i].get_value(), args[i])
            i += 1
        ret = self.block.send("call:with:", [SL_Array([]), s])
        return ret
    # def call(self, parent, args):
    #     s = SL_Scope()
    #     s.set_parent(self.closure)
    #     s.set_local("this", parent)
    #     i = 0
    #     params = self._params()
    #     while i < len(args):
    #         s.set_local(params[i].get_value(), args[i])
    #         i += 1
    #     ret = self.block.eval(s)
    #     return ret

class SL_Primitive_Method(SL_Method):
    def __init__(self, signature, primitive):
        self.signature = signature
        self.primitive = primitive
        self.compiled_signature = compile_signature(self.signature)
    def is_primitive(self):
        return True
    def set_primitive(self, meth):
        self.primitive = meth
        return self
    def call(self, parent, args):
        if not self.primitive:
            raise NotImplementedError("Primitive not implemented")
        else:
            self.parent = parent
            return self.primitive(self, args)

import primitives
int_methods[primitives.int_addition.get_compiled_signature()] = primitives.int_addition
int_methods[primitives.int_println.get_compiled_signature()]  = primitives.int_println
int_methods[primitives.int_string.get_compiled_signature()]   = primitives.int_string
int_methods[primitives.int_times.get_compiled_signature()]    = primitives.int_times

string_methods[primitives.string_println.get_compiled_signature()] = primitives.string_println
string_methods[primitives.string_add.get_compiled_signature()]     = primitives.string_add
string_methods[primitives.string_string.get_compiled_signature()]  = primitives.string_string

array_methods[primitives.array_println.get_compiled_signature()]  = primitives.array_println
array_methods[primitives.array_string.get_compiled_signature()]   = primitives.array_string

block_methods[primitives.block_call_unary.get_compiled_signature()]   = primitives.block_call_unary
block_methods[primitives.block_call_shallow.get_compiled_signature()] = primitives.block_call_shallow
block_methods[primitives.block_call_binary.get_compiled_signature()]  = primitives.block_call_binary
block_methods[primitives.block_call_with.get_compiled_signature()]    = primitives.block_call_with

