from rply.token import BaseBox

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
        for stmt in self.nodes:
            stmt.eval(scope)

from rpython.rlib.listsort import make_timsort_class
def _sig_comp(a, b):
    return len(a) > len(b) # Descending order by length
SigSort = make_timsort_class(lt = _sig_comp)

class S_Statement(S_List):
    def __init__(self, exprs):
        self.expressions = exprs
    def get_expressions(self):
        return self.expressions
    def __repr__(self):
        return "st("+(",".join([e.__repr__() for e in self.expressions]))+")"
    def set_target(self):
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
                        [nexpr], # [S_Operator(=)]
                        [SL_String(expr.get_value()), ret]
                    )
                else:
                    scope.set_local(expr.get_value(), ret)
                self.e_ec = self.e_end
                return
        if expr.__class__ == S_Int or expr.__class__ == S_Identifier:
            self.e_target = expr.eval(scope)
        else:
            print scope.locals
            raise NotImplementedError("Unknown expression: "+str(expr.__class__))
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
    def match_send(self):
        from stalk.object import compile_signature # TODO: Inline this
        
        # Reset the send data
        self.e_send_sig = None
        self.e_send_params = None
        
        recv = self.e_target # Receiver of the call.
        expr = self.e_expr
        if not expr:
            return False
        
        sigs = [m.get_signature() for m in recv.methods.values()]
        
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
                if type(s0) == S_Operator and s0.get_value() == expr.get_value():
                    self.e_send_sig = compile_signature(sig)
                    self.e_send_params = [nexpr]
                    return True
                    # 
                    # # Peek ahead
                    # if (self.e_ec + 1) < self.e_end:
                    #     nexpr = self.expressions[self.e_ec + 1]
                    #     if nexpr.__class__ != S_Keyword and \
                    #        sig[1].__class__ == S_Identifier:
                    #     #/if
                    #         return (sig, (nexpr,))
        exprs = []
        si = 0
        is_keyword = True
        while expr:
            if len(sigs) == 0:
                return False
            if is_keyword:
                if expr.__class__ != S_Keyword:
                    raise Exception("Unexpected non-keyword: "+expr.__repr__())
                _sigs = []
                for sig in sigs:
                    se = sig[si]
                    # TODO: Return to this and make it just one if (currently
                    #       the first one is just to appease PyPy).
                    if isinstance(expr, S_Keyword) and isinstance(se, S_Keyword):
                        if expr.get_value() == se.get_value():
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
            if not self.e_target:
                # Get the target if there isn't one.
                self.set_target()
            else:
                # Build up a signature for a send
                sig_head_index = self.e_ec
                matched = self.match_send()
                if matched:
                    sig = self.e_send_sig
                    exprs = self.e_send_params
                    params = [expr.eval(scope) for expr in exprs]
                    ret = self.e_target.send(sig, params)
                    if ret:
                        self.e_target = ret
                    else:
                        raise Exception("Method not found")
                else:
                    if self.e_ec >= self.e_end:
                        break # No signature found because end of statement
                    if sig_head_index < self.e_end:
                        sig_head = self.expressions[sig_head_index].__repr__()
                        if self.e_target:
                            target = repr(self.e_target)
                        else:
                            target = "unknown"
                        raise Exception("No method "+sig_head+" for "+target)
                    else:
                        raise Exception("No method error")
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
        raise NotImplementedError("Reached raw expression")

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

class S_Array(S_Expression):
    def __init__(self, value):
        self.type = "array"
        self.value = "[array]"
        # List of expressions to be evaluated into array contents.
        self.list = value
    def __repr__(self):
        return "s_array("+self.list.__repr__()+")"
    def get_value(self):
        #return self.list
        return "[array]"

class S_Block(S_Expression):
    def __init__(self, _header, value):
        self.type = "block"
        # List of expressions to be evaluated for the block.
        self.list = value
        self.header = _header
        self.preface = None
    def __repr__(self):
        return "s_block()"
    def get_value(self):
        #return (self.preface, self.list)
        return "[block]"
    def set_preface(self, preface):
        self.preface = preface
    
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

class S_Operator(S_Expression):
    def __init__(self, value):
        self.type = "operator"
        self.value = value
    def __repr__(self):
        return self.value
    def get_value(self):
        return self.value

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

from stalk.bootstrap import SL_Int

class S_Int(S_Literal):
    def __init__(self, value):
        self.type = "int"
        self.value = "[int]"
        self.int = value
    def __repr__(self):
        return str(self.int)
    def get_value(self):
        return str(self.int)
    def to_primitive(self):
        return SL_Int(self.int)
    def eval(self, scope):
        return self.to_primitive()

class S_Float(S_Literal):
    def __init__(self, value):
        self.type = "float"
        self.value = "[float]"
        self.float = value
    def __repr__(self):
        return str(self.float)
    def get_value(self):
        return str(self.float)

class S_String(S_Literal):
    def __init__(self, value):
        self.type = "string"
        self.value = "[string]"
        self.string = value
    def __repr__(self):
        return repr(self.string)
    def get_value(self):
        return self.string