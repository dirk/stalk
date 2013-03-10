from stalk.ast import S_Operator, S_Identifier, S_Keyword

def compile_signature(sig):
    parts = []
    for e in sig:
        if type(e) == S_Operator or type(e) == S_Identifier or type(e) == S_Keyword:
            parts.append(e.get_value())
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
    def receive(self, signature):
        return None # TODO: Make this work

class SL_Primitive(SL_Object):
    def get_values(self):
        return {}
    def get_methods(self):
        return self.methods
    def get_value_int(self):
        return 0
    def get_value_string(self):
        return ""
    def get_value_array(self):
        return []

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
        return NotImplementedError("Not implemented")
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
        return NotImplementedError("Not implemented")
    def send(self, sig, params): # Receive (compiled) signature and data tuples
        if sig in self.methods.keys():
            return self.methods[sig].call(self, params)
        else:
            return None

# TODO: Maybe not have this be a primitive?
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
        return NotImplementedError("Not implemented")
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
        return NotImplementedError("Not implemented")
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

class SL_Method(object):
    def __init__(self, parent):
        self.signature = None
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
        self.parent = parent # The SL_Object the method is bound to.
        self.list = None # S_List[S_Block]
    def is_primitive(self):
        return False
    def get_signature(self):
        return self.signature
    def get_compiled_signature(self):
        return compile_signature(self.signature)

class SL_Primitive_Method(object):
    # _annspecialcase_ = 'specialize:ctr_location'
    
    def __init__(self, signature, primitive):
        self.signature = signature
        self.primitive = primitive
        self.parent = None
    def is_primitive(self):
        return True
    def set_primitive(self, meth):
        self.primitive = meth
        return self
    def get_compiled_signature(self):
        return compile_signature(self.signature)
    def get_signature(self):
        return self.signature
    
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

string_methods[primitives.string_println.get_compiled_signature()] = primitives.string_println
string_methods[primitives.string_add.get_compiled_signature()]     = primitives.string_add
string_methods[primitives.string_string.get_compiled_signature()]  = primitives.string_string

array_methods[primitives.array_println.get_compiled_signature()]  = primitives.array_println
array_methods[primitives.array_string.get_compiled_signature()]   = primitives.array_string

