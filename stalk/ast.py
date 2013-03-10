from rply.token import BaseBox

class ReturnException(Exception):
    def __init__(self, ret):
        self.ret = ret
    def get_return(self):
        return self.ret

# Primitive base of the language is the S_List, a list of statements to be
# executed.
class S_List(BaseBox):
    def __init__(self, nodes):
        self.nodes = nodes # List of statements
    def get_nodes(self):
        return self.nodes
    def __repr__(self):
        return "s("+(",".join([n.__repr__() for n in self.nodes]))+")"
    def eval(self, scope):
        # TODO: Fix all these co-dependent import hacks.
        from stalk.object import SL_Method
        ret = None
        try:
            for stmt in self.nodes:
                ret = stmt.eval(scope)
                # If it's a plain method then add it to the scope.
                if type(ret) == SL_Method:
                    scope.define_method(ret)
        except ReturnException as re:
            return re.get_return()
        return ret

from rpython.rlib.listsort import make_timsort_class
def _sig_comp(a, b):
    # return len(a) > len(b) # Descending order by length
    return len(a) < len(b) # Ascending order by length
SigSort = make_timsort_class(lt = _sig_comp)

class S_Statement(S_List):
    def __init__(self, exprs):
        self.expressions = exprs
    def get_expressions(self):
        return self.expressions
    def __repr__(self):
        return "st("+(",".join([e.__repr__() for e in self.expressions]))+")"
    def set_target(self):
        from stalk.object import SL_String, compile_signature
        scope = self.e_scope
        expr = self.e_expr
        # Peek ahead
        nexti = self.e_ec + 1
        if (nexti) < self.e_end:
            nexpr = self.expressions[nexti]
            if type(expr) == S_Identifier and type(nexpr) == S_Operator and \
               nexpr.get_value() == "=":
            #/if
                self.expressions.pop(nexti)
                self.expressions.insert(0, nexpr)
                self.e_ec += 2 # Move it to the next expr after the =
                ret = self.eval_assign()
                if self.e_target:
                    self.e_target.send(
                        compile_signature([nexpr]), # [S_Operator(=)]
                        [SL_String(expr.get_value()), ret]
                    )
                else:
                    scope.set_local(expr.get_value(), ret)
                self.e_ec = self.e_end
                return
        # TODO: Ensure 100% eval-availability so we can ditch this.
        #       Includes having to deal with comments.
        try:
            ev = expr.eval
        except AttributeError as e:
            raise NotImplementedError("Unknown expression: "+str(expr.__class__))
        self.e_target = ev(scope)
    
    def peeknext(self):
        ei = self.e_ec + 1
        if ei < self.e_end:
            return self.expressions[ei]
        else:
            return None
    def next(self):
        self.e_ec += 1
        if self.e_ec < self.e_end:
            self.e_expr = self.expressions[self.e_ec]
            return self.e_expr
        else:
            return None
    def eval_assign(self):
        scope = self.e_scope
        # Create a substatement to capture the assignment
        substatement = S_Statement(self.expressions[self.e_ec:])
        ret = substatement.eval(scope)
        return ret
    def match_def(self):
        sig = []
        block = None
        e = self.e_expr
        et = type(e)
        if not e:
            return False
        if et == S_Operator:
            ide = self.peeknext()
            if type(ide) == S_Identifier:
                sig = [e, ide]
                self.next()
                block = self.next()
                if type(block) != S_Block:
                    raise Exception("Missing block: "+repr(block))
            else:
                raise Exception("Unexpected following operator: "+repr(ide))
        if et == S_Identifier:
            sig = [e]
            block = self.next()
            if type(block) != S_Block:
                raise Exception("Missing block: "+repr(block))
        # Didn't match the operator shorthand
        if len(sig) == 0:
            is_keyword = True
            while e:
                if not e:
                    break
                if is_keyword:
                    if et == S_Keyword:
                        sig.append(e)
                    elif et == S_Block:
                        block = e
                        break
                    else:
                        raise Exception("Unexpected non-keyword: "+repr(e))
                else:
                    if et == S_Identifier:
                        sig.append(e)
                    else:
                        raise Exception("Unexpected non-identifier: "+repr(e))
                e = self.next()
                et = type(e)
                is_keyword = not is_keyword
        if len(sig) == 0:
            raise Exception("Missing method definition: "+repr(e))
        # TODO: Fix all these co-dependent import hacks.
        from stalk.object import SL_Method
        self.e_target = SL_Method(sig, block, self.e_scope)
        self.next()
        return True
        
    def match_send(self, recv):
        from stalk.object import compile_signature
        
        # Reset the send data
        self.e_send_sig = None
        self.e_send_params = None
        
        # if self.e_target:
        #     recv = self.e_target # Receiver of the call.
        # else:
        #     recv = self.e_scope
        expr = self.e_expr
        if not expr:
            return False
        
        #sigs = [m.get_signature() for m in recv.methods.values()]
        sigs = recv.get_signatures()
        
        if expr.__class__ == S_Identifier:
            for sig in sigs:
                if len(sig) != 1:
                    next
                s0 = sig[0]
                if type(s0) == S_Identifier and s0.get_value() == expr.get_value():
                    self.e_send_sig = compile_signature(sig)
                    self.e_send_params = []
                    return True
        if expr.__class__ == S_Operator:
            nexpr = self.next()
            if not nexpr:
                raise Exception("Need expression following "+
                    expr.__repr__()+" operator")
            if nexpr.__class__ == S_Keyword:
                raise Exception("Keyword cannot follow operator")
            for sig in sigs:
                if len(sig) != 2:
                    next
                s0 = sig[0]
                #if type(s0) == S_Operator and s0.get_value() == expr.get_value():
                if s0.eq(expr):
                    self.e_send_sig = compile_signature(sig)
                    self.e_send_params = [nexpr]
                    return True
        exprs = []
        si = 0
        is_keyword = True
        while expr:
            if len(sigs) == 0:
                return False
            if is_keyword:
                if expr.__class__ != S_Keyword:
                    # TODO: Decide whether this should be greedy or not
                    break # <- non-greedy
                    #raise Exception("Unexpected non-keyword: "+expr.__repr__())
                _sigs = []
                for sig in sigs:
                    if si < len(sig):
                        se = sig[si]
                        # TODO: Return to this and make it just one if (currently
                        #       the first one is just to appease PyPy).
                        #if isinstance(expr, S_Keyword) and isinstance(se, S_Keyword):
                        #    if expr.get_value() == se.get_value():
                        if expr.eq(se):
                                _sigs.append(sig)
                        #/if
                        #if expr.get_value() != se.get_value():
                sigs = _sigs
            else:
                if expr.__class__ != S_Keyword:
                    exprs.append(expr)
                else:
                    raise Exception("Unexpected keyword: "+expr.__repr__())
            expr = self.next()
            is_keyword = not is_keyword # Flip for style!
            si += 1
        
        if len(sigs) > 0:
            # Sort in descending order (longest matched sig first)
            SigSort(sigs).sort()
            self.e_send_sig = compile_signature(sigs[0])
            self.e_send_params = exprs
            return True
        # Didn't match!
        return False
    def do_send(self):
        if self.e_target:
            target = self.e_target # Receiver of the call.
        else:
            target = self.e_scope
        # Build up a signature for a send
        sig_head_index = self.e_ec
        matched = self.match_send(target)
        if matched:
            sig = self.e_send_sig
            exprs = self.e_send_params
            params = [expr.eval(self.e_scope) for expr in exprs]
            ret = target.send(sig, params)
            if ret:
                self.e_target = ret
            else:
                raise Exception("Method not found")
        else:
            if self.e_ec >= self.e_end:
                return # No signature found because end of statement
            if sig_head_index < self.e_end:
                sig_head = self.expressions[sig_head_index].__repr__()
                if target:
                    tr = repr(target)
                else:
                    tr = "unknown"
                raise Exception("No method "+sig_head+" for "+tr)
            else:
                # Shouldn't ever reach here.
                raise Exception("No method error")
    
    def eval(self, scope):
        self.e_ec = 0 # Current position in the expression of evaluation
        self.e_end = len(self.expressions) # Length of expression
        self.e_target = None # Target of message send
        self.e_expr = None
        self.e_scope = scope
        self.e_send_sig = None # Signature of current call
        self.e_send_params = None
        while self.e_ec < self.e_end:
            self.e_expr = self.expressions[self.e_ec] # Current expression it's on
            if type(self.e_expr) == S_Keyword:
                if self.e_expr.get_value() == "def:":
                    self.next()
                    self.match_def()
                if self.e_expr.get_value() == "return:":
                    substatement = S_Statement(self.expressions[self.e_ec + 1:])
                    ret = substatement.eval(self.e_scope)
                    # return ret
                    re = ReturnException(ret)
                    raise re
                else: # Not a def but still a top-level method call
                    self.do_send()
            if type(self.e_expr) == S_Identifier and self.e_expr.get_value() == "return":
                if self.e_target:
                    # return self.e_target
                    re = ReturnException(self.e_target)
                    raise re
                else:
                    raise Exception("Return without expressions")
            else:
                if not self.e_target:
                    # Get the target if there isn't one.
                    self.set_target()
                else:
                    self.do_send()
            self.e_ec += 1
        # Teardown
        self.e_ec = 0
        self.e_end = 0
        self.e_expr = None
        target = self.e_target
        self.e_target = None
        self.e_scope = None
        self.e_send_sig = None
        self.e_send_params = None
        return target

def s_add_list(box_list, other):
    _list = box_list.get_nodes() if box_list is not None else []
    return S_List(_list + [other])

def s_add_statement(box_statement, other):
    _list = box_statement.get_expressions() if box_statement is not None else []
    return S_Statement(_list + [other])

class S_Expression(BaseBox):
    def __init__(self, _type, value):
        self.type = _type
        self.value = value
    def __repr__(self):
        return self.type+":"+self.value
    def get_value(self):
        return str(self.value)
    def eval(self, scope):
        raise NotImplementedError("Reached raw expression: "+repr(self))
    def eq(self, other):
        return self == other

class S_Comment(S_Expression):
    def __init__(self, value):
        self.type = "comment"
        self.value = value
    def __repr__(self):
        return "#"+repr(self.value)
    def eval(self, scope):
        pass
    def get_value(self):
        return "#comment"

class S_Operator(S_Expression):
    def __init__(self, value):
        self.type = "operator"
        self.value = value
    def __repr__(self):
        return self.value
    def get_value(self):
        return self.value
    def eq(self, other):
        if type(self) == type(other):
            return self.value == other.get_value()
        else:
            return False

class S_Identifier(S_Expression):
    def __init__(self, value):
        self.type = "identifier"
        self.value = value
    def __repr__(self):
        return self.value
    def get_value(self):
        return self.value
    def eval(self, scope):
        return scope.get(self.value)
        # slots = scope.keys()
        # if self.value in slots:
        #     return scope[self.value]
        # else:
        #     return SL_Exception("Not found: "+self.value)

class S_Keyword(S_Expression):
    def __init__(self, value):
        self.type = "keyword"
        self.value = value
    def __repr__(self):
        return self.value
    def get_value(self):
        return self.value
    def eq(self, other):
        if type(self) == type(other):
            return self.value == other.get_value()
        else:
            return False

from stalk.object import SL_Null, SL_Array

class S_Array(S_Expression):
    def __init__(self, value):
        self.type = "array"
        self.value = "[array]"
        # List of expressions to be evaluated into array contents.
        self.list = value
    def __repr__(self):
        return "s_array("+self.list.__repr__()+")"
    def eval(self, scope):
        nodes = self.list.get_nodes()
        a = []
        for st in nodes:
            a.insert(0, st.eval(scope))
        return SL_Array(a)
    def get_value(self):
        #return self.list
        return "[array]"

class S_Block(S_Expression):
    def __init__(self, _header, value):
        self.type = "block"
        # List of expressions to be evaluated for the block.
        self.list = value # S_List
        self.header = _header
        self.preface = None
    def __repr__(self):
        return "s_block("+repr(self.list)+")"
    def get_value(self):
        #return (self.preface, self.list)
        return "[block]"
    def set_preface(self, preface):
        self.preface = preface
    def eval(self, scope):
        ret = self.list.eval(scope)
        return ret


class S_Literal(S_Expression):
    pass

class S_Symbol(S_Literal):
    def __init__(self, value):
        self.type = "symbol"
        self.value = value
    def __repr__(self):
        return self.value
    def get_value(self):
        return self.value
    def eq(self, other):
        if type(self) == type(other):
            return self.value == other.get_value()
        else:
            return False

from stalk.object import SL_Int

class S_Int(S_Literal):
    def __init__(self, value):
        self.type = "int"
        self.value = "[int]"
        self.int = value
    def __repr__(self):
        return str(self.int)
    def get_value(self):
        return str(self.int)
    def eval(self, scope):
        return SL_Int(self.int)
    def eq(self, other):
        if type(self) == type(other):
            return self.value == other.get_value()
        else:
            return False

class S_Float(S_Literal):
    def __init__(self, value):
        self.type = "float"
        self.value = "[float]"
        self.float = value
    def __repr__(self):
        return str(self.float)
    def get_value(self):
        return str(self.float)
    def eq(self, other):
        if type(self) == type(other):
            return self.value == other.get_value()
        else:
            return False

from stalk.object import SL_String

class S_String(S_Literal):
    def __init__(self, value):
        self.type = "string"
        self.value = "[string]"
        # TODO: String parsing
        self.string = value[1:-1]# Strip off leading and trailing "s
    def __repr__(self):
        return repr(self.string)
    def get_value(self):
        return self.string
    def eval(self, scope):
        return SL_String(self.string)
    def eq(self, other):
        if type(self) == type(other):
            return self.value == other.get_value()
        else:
            return False
