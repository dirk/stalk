from object import SL_Int, SL_Primitive_Method, SL_Exception, SL_Null, SL_String
from stalk.ast import S_Operator, S_Identifier

# from rpython.rlib.objectmodel import specialize, enforceargs

def _int_add(method, args):
    if type(args[0]) == SL_Int:
        v = method.parent.get_value_int() + args[0].get_value_int()
        return SL_Int(v)
    else:
        return SL_Exception("Cannot add "+args[0].get_name()+" to "+method.parent.get_name())
int_addition = SL_Primitive_Method(
    [S_Operator("+"), S_Identifier("other")],
    _int_add
)
def _int_println(method, args): # args = empty
    print method.parent.__repr__()
    return method.parent
int_println = SL_Primitive_Method(
    [S_Identifier("println")],
    _int_println
)

def _int_string(method, args): # args = empty
    return SL_String(method.parent.get_value_string())
int_string = SL_Primitive_Method(
    [S_Identifier("string")],
    _int_string
)

def _string_println(method, args):
    print method.parent.get_value_string()
    return method.parent
string_println = SL_Primitive_Method(
    [S_Identifier("println")],
    _string_println
)

def _string_add(method, args):
    if type(args[0]) == SL_String:
        v = method.parent.get_value_string() + args[0].get_value_string()
        return SL_String(v)
    else:
        return SL_Exception("Cannot add "+args[0].get_name()+" to "+method.parent.get_name())
string_add = SL_Primitive_Method(
    [S_Operator("+"), S_Identifier("other")],
    _string_add
)
