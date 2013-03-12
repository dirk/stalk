%{
#include <stdio.h>
#include "stalk.h"
#include "parse.h"
#include "syntax.h"

extern int yylex();
extern int yyparse();
void yyerror(char *err);
extern int yylineno_extern;
%}

%union {
  char* sl_p_string;
  int sl_p_int;
  float sl_p_float;
}

%token SL_P_IDENT
%token SL_P_ASSIGN
%token <sl_p_int> SL_P_INTEGER
%token <sl_p_float> SL_P_DECIMAL
%token <sl_p_string> SL_P_STRING
%token <sl_p_string> SL_P_SYMBOL
%token SL_P_COMMA
%token SL_P_LBRACK
%token SL_P_RBRACK
%token SL_P_VERT
%token SL_P_LPAREN
%token SL_P_RPAREN
%token SL_P_LSQ
%token SL_P_RSQ
%token SL_P_TERMINAL
%token <sl_p_string> SL_P_KEYWORD
%token SL_P_CONT
%token SL_P_SWS
%token SL_P_COMMENT
%token <sl_p_string> SL_P_OPERATOR

%left SL_P_IDENT

%%

main: exprs;
exprs: expr exprs;
exprs: expr;

expr: messages SL_P_TERMINAL;
expr: messages SL_P_COMMENT SL_P_TERMINAL;
expr: SL_P_COMMENT SL_P_TERMINAL { };

subexpr: literal
       | ident
       ;
subexpr: SL_P_LPAREN expr SL_P_RPAREN;

block: SL_P_LBRACK block_inside SL_P_RBRACK;
block_inside: block_header block_body;
block_header: SL_P_VERT block_header_inside SL_P_VERT;
block_header_inside: ident SL_P_COMMA block_header_inside;
block_header_inside: ident;
block_body: exprs;

messages: message cont messages;
messages: message sws messages;
messages: message;

message: ident sws SL_P_ASSIGN cont subexpr;
message: ident sws SL_P_ASSIGN sws subexpr;
message: keywords;
message: subexpr SL_P_OPERATOR subexpr;
message: subexpr;

keywords: keyword_pair cont keywords;
keywords: keyword_pair sws keywords;
keywords: keyword_pair;
keyword_pair: keyword subexpr;
keyword: SL_P_KEYWORD;


literal: SL_P_INTEGER;
literal: SL_P_DECIMAL;
literal: SL_P_STRING;
literal: SL_P_SYMBOL;
literal: block;
literal: array;

array: SL_P_LSQ array_inside SL_P_RSQ;
array: SL_P_LSQ SL_P_RSQ;
array_inside: subexpr SL_P_COMMA array_inside;
array_inside: subexpr;

ident: SL_P_IDENT;

cont: SL_P_CONT;
sws: SL_P_SWS;

%%


void yyerror(char *err) {
  printf("Parse error on line %d: %s", yylineno_extern, err);
}
