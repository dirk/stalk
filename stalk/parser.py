
from rply import ParserGenerator, LexerGenerator

lg = LexerGenerator()

lg.add("INTEGER", r"-?0|([1-9][0-9]*)")
lg.add("DECIMAL", r"\.[0-9]+")
_id = r"[A-Za-z][A-Za-z0-9_]*"
lg.add("KEYWORD", _id + r":")
lg.add("IDENTIFIER", _id)
lg.add("SYMBOL", r":" + _id)
_comment = r"[ \t]*#[^\n]*"
lg.add("COMMENT", _comment)
lg.add("LPAREN", r"\([ \t\n]*")
lg.add("RPAREN", r"[ \t\n]*\)")
# TODO: Maybe clear this up to be prettier.
lg.add("PREFACE", r"<[A-Za-z0-9_:@, \t\n]+>[ \t\n]*")
lg.add("LBRACK", r"{[ \t\n]*")
lg.add("RBRACK", r"[ \t\n]*}")
lg.add("VERT", r"\|[ \t\n]*")
lg.add("LSQ", r"\[[ \t\n]*")
lg.add("RSQ", r"[ \t\n]*\]")
lg.add("CONT", r"[ \t]+\\(" + _comment + r")?\n[ \t]*")
lg.add("SWS", r"[ \t]+")
lg.add("COMMA", ",[ \t\n]*")
lg.add("TERMINAL", r"[ \t]*\n[ \t\n]*")
# TODO: Make strings parsing not suck dick.
lg.add("STRING", r"\"[^\"]*\"")
# TODO: Finalize operators
lg.add("OPERATOR", r"[+\-=*/\^]")

pg = ParserGenerator(
    [r.name for r in lg.rules]
    # TODO: Add operator precedence and cache_id
)



from ast import *

@pg.production("main : statements")
def main(p):
    return p[0]

@pg.production("statements : statements statement")
def statements(p):
    return s_add_list(p[0], p[1])
    

@pg.production("statements : statement")
def statements_statement(p):
    #return p[0]
    return S_List([p[0]])

# Allow for terminals before the statement.
@pg.production("statements : TERMINAL statement")
def statements_terminal_pre_statement(p):
    #return p[1]
    return S_List([p[1]])

# Whole-line comments
@pg.production("statement : COMMENT TERMINAL")
def statement_comment(p):
    return S_Comment(p[0].getstr())

@pg.production("statement : expressions COMMENT TERMINAL")
def statement_with_comment(p):
    return p[0] # S_List

@pg.production("statement : expressions TERMINAL")
def statement_with_terminal(p):
    # p_s = P_Statement()
    # p_s.expressions = p[0]
    # return p_s
    return p[0] # S_Statement

@pg.production("expressions : expression")
def expressions_single(p):
    return S_Statement([p[0]])
    #return p[0]

@pg.production("expressions : expressions CONT expression")
@pg.production("expressions : expressions SWS expression")
def expressions(p):
    return s_add_statement(p[0], p[2])
    #return p[0]
    
@pg.production("expression : LPAREN expressions RPAREN")
def expression_group(p):
    return p[1] # S_Statement

@pg.production("block : block_right")
def block_no_preface(p):
    return p[0]

@pg.production("block : PREFACE block_right")
def block_preface(p):
    p[1].set_preface(p[0].getstr())
    return p[1]

@pg.production("block_right : LBRACK block_inside RBRACK")
def block(p):
    return p[1] # S_Block

@pg.production("block_inside : block_header block_body")
def block_inside_both(p):
    # S_Block(S_List, S_List)
    return S_Block(p[0], p[1])

@pg.production("block_inside : block_body")
def block_inside(p):
    # S_Block(None, S_List)
    return S_Block(None, p[0])

# Allow for multiple statements and a trailing expression (expression w/out
# terminals).
@pg.production("block_body : statements")
def block(p):
    return p[0] # S_List

@pg.production("block_body : statements expressions")
def block(p):
    return s_add_list(p[0], p[1])

@pg.production("block_body : expressions")
def block_body_single(p):
    return S_List([p[0]])

# TODO: Make special production for block header.
@pg.production("block_header : VERT array_inside VERT")
def block_header(p):
    return p[1]

@pg.production("array : LSQ array_inside RSQ")
def array(p):
    return p[1]

@pg.production("array : LSQ RSQ")
def array_empty(p):
    #return "empty array"
    #return p[0]
    return S_List([])

@pg.production("array_inside : expression COMMA array_inside")
def array_multi(p):
    #return [p[0], p[2]]
    #return p[0]
    return s_add_list(p[2], p[0])

@pg.production("array_inside : expression")
def array_single(p):
    #return p[0]
    return S_List([p[0]])

# Expressions:

@pg.production("expression : array")
def expression_array(p):
    return S_Array(p[0])

@pg.production("expression : block")
def expression_block(p):
    return p[0] # S_Block

@pg.production("expression : KEYWORD")
def expression_keyword(p):
    return S_Keyword(p[0].getstr())

@pg.production("expression : OPERATOR")
def expression_operator(p):
    return S_Operator(p[0].getstr())

@pg.production("expression : IDENTIFIER")
def expression_identifier(p):
    return S_Identifier(p[0].getstr())
    
@pg.production("expression : SYMBOL")
def expression_symbol(p):
    return S_Symbol(p[0].getstr())

@pg.production("expression : INTEGER")
def expression_int(p):
    return S_Int(int(p[0].getstr()))

@pg.production("expression : STRING")
def expression_string(p):
    return S_String(p[0].getstr())

@pg.production("expression : INTEGER DECIMAL")
def expression_float(p):
    f = p[0].getstr() + p[1].getstr()
    return S_Float(float(f))
    

lexer = lg.build()
parser = pg.build()

def parse(body):
    return parser.parse(lexer.lex(body))
