from stalk.stalk.object import SL_Object, SL_Int
from stalk.interpreter import Scope, RootScope

internal_constructors = {}

def memoize(name):
    def decorator(fn):
        def wrapper(*args, **kwargs):
            if name in internal_types:
                return internal_types[name]
            return fn(*args, **kwargs)
        return wrapper
    return decorator

#@memoize("int")
def _int():
    # if "int" in internal_types:
    #     return internal_types["int"]
    # _int = SL_Type()
    # _int.name = "Int"
    # # set up basic methods for integer types
    # return _int
    _int = SL_Object()
    # Methods
    _int.name = "Int"
    return _int



def bootstrap(scope):
    if scope.__class__ != Scope and scope.__class__ != RootScope:
        raise Exception("Bootstrap scope must be a Scope")
    
    # Create core types
    scope.locals["Int"] = _int()
    # scope.locals["one"] = SL_Int(1)
