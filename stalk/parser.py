import py
import sys

# from rpython.rlib.parsing.ebnfparse import parse_ebnf, make_parse_function
# from pypy.rlib.parsing.ebnfparse import parse_ebnf, make_parse_function

# grammar = py.path.local("stalk/grammar.txt").read("rt")
# regexs, rules, ToAST = parse_ebnf(grammar)


# print regexs
# print rules

# parse = make_parse_function(regexs, rules, eof=True)

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
#lg.add("LANGLE", r"<")
#lg.add("RANGLE", r">")
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

lg.add("STRING", r"\"[^\"]*\"")

lg.add("OPERATOR", r"[+\-=*/\^]")

pg = ParserGenerator(
    [r.name for r in lg.rules]
)

@pg.production("main : statements")
def main(p):
    return p[0]

@pg.production("statements : statements statement")
def statements(p):
    return p[0]

@pg.production("statements : statement")
def statements_statement(p):
    return p[0]

# Allow for terminals before the statement
@pg.production("statements : TERMINAL statement")
def statements_statement(p):
    return p[0]

@pg.production("statement : COMMENT TERMINAL")
def statement_comment(p):
    return p[0]

@pg.production("statement : expression COMMENT TERMINAL")
def statement_with_comment(p):
    return p[0]

@pg.production("statement : expression TERMINAL")
def statement(p):
    return p[0]

@pg.production("expression : expression CONT expression")
def expressions_with_cont(p):
    #print p
    return p[0]

@pg.production("expression : expression SWS expression")
def expressions(p):
    #print p
    return p[0]

@pg.production("expression : LPAREN expression RPAREN")
def expression_group(p):
    return p[0]


@pg.production("block : PREFACE block")
def block(p):
    return p[0]

@pg.production("block : LBRACK block_inside RBRACK")
def block(p):
    return p[0]

@pg.production("block_inside : block_header block_body")
def block(p):
    return p[0]

@pg.production("block_inside : block_body")
def block(p):
    return p[0]

# Allow for multiple statements and a trailing expression (expression w/out
# terminals).
@pg.production("block_body : statements")
def block(p):
    return p[0]

@pg.production("block_body : statements expression")
def block(p):
    return p[0]

@pg.production("block_body : expression")
def block(p):
    return p[0]

# TODO: Make special production for block header.
@pg.production("block_header : VERT array_inside VERT")
def block(p):
    return p[0]

@pg.production("array : LSQ array_inside RSQ")
def array(p):
    return p[0]

@pg.production("array : LSQ RSQ")
def array_empty(p):
    return p[0]

@pg.production("array_inside : expression COMMA array_inside")
def array_multi(p):
    return p[0]

@pg.production("array_inside : expression")
def array_single(p):
    return p[0]

@pg.production("expression : array")
def expression_array(p):
    return p[0]

@pg.production("expression : block")
def expression_block(p):
    return p[0]

@pg.production("expression : KEYWORD")
def expression_keyword(p):
    return p[0].value

@pg.production("expression : OPERATOR")
def expression_operator(p):
    return p[0].value

@pg.production("expression : IDENTIFIER")
def expression_identifier(p):
    return p[0].value

@pg.production("expression : SYMBOL")
def expression_symbol(p):
    return p[0].value

@pg.production("expression : INTEGER")
def expression_int(p):
    return p[0].value

@pg.production("expression : STRING")
def expression_string(p):
    return p[0].value


@pg.production("expression : INTEGER DECIMAL")
def expression_float(p):
    return p[0].value

lexer = lg.build()
parser = pg.build()

def parse(body):
    parser.parse(lexer.lex(body))

# main(_star_symbol0(statement(expression(identifier(Symbol('IDENTIFIER', 'testing'))), _star_symbol1(sws(_plus_symbol0(Symbol('__0_ ', ' '))), expression(operator(Symbol('__3_=', '='))), _star_symbol1(sws(_plus_symbol0(Symbol('__0_ ', ' '))), expression(identifier(Symbol('IDENTIFIER', 'tseting'))))), __statement_rest_0_0(terminal(__terminal_rest_0_0(nl(Symbol('__2_\n', '\n'))))))), Symbol('EOF', 'EOF'))
