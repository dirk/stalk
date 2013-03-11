from object import SL_Int, SL_Primitive_Method, SL_Exception, SL_Null, SL_String, SL_Block, SL_Array
from stalk.interpreter import SL_Scope
from stalk.ast import S_Operator, S_Identifier, S_Keyword

# from rpython.rlib.objectmodel import specialize, enforceargs

def _int_add(method, args):
    if type(args[0]) == SL_Int:
        v = method.parent.get_value_int() + args[0].get_value_int()
        return SL_Int(v)
    else:
        return SL_Exception("Cannot add "+args[0].get_name()+" to "+method.parent.get_name())
int_addition = SL_Primitive_Method([S_Operator("+"), S_Identifier("other")], _int_add)

def _int_println(method, args): # args = empty
    print method.parent.get_value_string()
    return method.parent
int_println = SL_Primitive_Method([S_Identifier("println")], _int_println)

def _int_string(method, args): # args = empty
    return SL_String(method.parent.get_value_string())
int_string = SL_Primitive_Method([S_Identifier("string")], _int_string)

def _int_times(method, args): # args[0] = block
    if type(args[0]) == SL_Block:
        r = range(0, method.parent.get_value_int())
        _i = r[0]
        for i in r:
            # call: = [S_Keyword("call:"), S_Identifier("params")]
            args[0].send("call:", [SL_Array([SL_Int(i)])])
            _i = i
        return SL_Int(_i + 1)
    else:
        return SL_Exception("Must be block")
    return method.parent
int_times = SL_Primitive_Method([S_Keyword("times:"), S_Identifier("block")], _int_times)




def _string_println(method, args):
    print method.parent.get_value_string()
    return method.parent
string_println = SL_Primitive_Method([S_Identifier("println")], _string_println)

def _string_string(method, args):
    return method.parent
string_string = SL_Primitive_Method([S_Identifier("string")], _string_string)

def _string_add(method, args):
    if type(args[0]) == SL_String:
        v = method.parent.get_value_string() + args[0].get_value_string()
        return SL_String(v)
    else:
        return SL_Exception("Cannot add "+args[0].get_name()+" to "+method.parent.get_name())
string_add = SL_Primitive_Method([S_Operator("+"), S_Identifier("other")], _string_add)




def _array_string(method, args):
    a = method.parent.get_value_array()
    sig = "string" #[S_Identifier("string")]
    ao = [o.send(sig, []).get_value_string() for o in a]
    return SL_String("["+",".join(ao)+"]")
array_string = SL_Primitive_Method([S_Identifier("string")], _array_string)

def _array_println(method, args):
    print _array_string(method, [])
    return method.parent
array_println = SL_Primitive_Method([S_Identifier("println")], _array_println)

def _block_call_unary(method, args): # args = empty
    sb = method.parent.get_value_block() # S_Block
    sc = method.parent.get_closure()
    
    ret = sb.get_list().eval(sc)
    return ret
block_call_unary = SL_Primitive_Method([S_Identifier("call")], _block_call_unary)

def _block_call_shallow(method, args): # args = empty
    sb = method.parent.get_value_block() # S_Block
    sc = method.parent.get_closure()
    
    ret = sb.get_list().eval_shallow(sc)
    return ret
block_call_shallow = SL_Primitive_Method([S_Identifier("call_shallow")], _block_call_shallow)

def _block_call_binary(method, args):
    # args[0] = SL_Array
    b_block = method.parent.get_value_block() # S_Block
    b_params = b_block.get_header()
    sent_params = args[0]
    if type(sent_params) != SL_Array:
        # TODO: Make this duck-check instead of type-check
        #return SL_Exception("Params must be an array")
        raise Exception("Params must be an array")
    sent_params_array = sent_params.get_value_array()#[SL_Object...]
    recv_params_array = b_params.get_nodes()#[S_Identifier...]
    if len(sent_params_array) != len(recv_params_array):
        raise Exception("Sent and receiving param lengths do not match")
    
    scope = SL_Scope()
    i = 0
    for s_i in recv_params_array:
        scope.set_local(s_i.get_value(), sent_params_array[i])
        i += 1
    b_closure = method.parent.get_closure()
    scope.set_parent(b_closure)
    
    b_list = b_block.get_list()
    return b_list.eval(scope)
block_call_binary = SL_Primitive_Method([S_Keyword("call:"), S_Identifier("params")], _block_call_binary)

def _block_call_with(method, args):
    # args[0] = SL_Array
    # args[1] = SL_Scope
    b_block = method.parent.get_value_block() # S_Block
    b_params = b_block.get_header()
    sent_params = args[0]
    if type(sent_params) != SL_Array:
        # TODO: Make this duck-check instead of type-check
        #return SL_Exception("Params must be an array")
        raise Exception("Params must be an array")
    sent_params_array = sent_params.get_value_array()#[SL_Object...]
    recv_params_array = b_params.get_nodes()#[S_Identifier...]
    if len(sent_params_array) != len(recv_params_array):
        raise Exception("Sent and receiving param lengths do not match")
    
    scope = args[1]
    i = 0
    for s_i in recv_params_array:
        v = s_i.get_value()
        if scope.get_local(v):
            print "WARNING: Overwriting magic parameter '"+v+"'"
        scope.set_local(v, sent_params_array[i])
        i += 1
    b_closure = method.parent.get_closure()
    scope.set_parent(b_closure)
    
    b_list = b_block.get_list()
    return b_list.eval(scope)
block_call_with = SL_Primitive_Method(
    [S_Keyword("call:"), S_Identifier("params"), S_Keyword("with:"), S_Identifier("scope")],
    _block_call_with
)

def _cond_if(method, args):
    # args[0] = SL_Object
    # args[1] = SL_Block
    
    test = args[0]
    block = args[1] # SL_Block
    
    # TODO: Evaluate truthiness of arg
    if type(test) != SL_Null:
        #b_list = block.get_list()
        #return b_list.eval_shallow(method.parent.get_closure())
        ret = block.send("call_shallow", [])
        return ret
    else:
        return SL_Null()
cond_if = SL_Primitive_Method(
    [S_Keyword("if:"), S_Identifier("cond"), S_Keyword("then:"), S_Identifier("block")],
    _cond_if
)

