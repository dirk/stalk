from object import SL_Int, SL_Primitive_Method, SL_Exception
from stalk.ast import S_Operator, S_Identifier

def _int_add_primitive(method, args):
    # args[0] = other
    if args[0].__class__ == SL_Int:
        return SL_Int(method.parent.value + args[0].get_value())
    else:
        return SL_Exception("Cannot add "+args[0].get_name()+" to "+method.parent.get_name())
int_addition = SL_Primitive_Method(
    (S_Operator("+"), S_Identifier("other")),
    _int_add_primitive
)
        