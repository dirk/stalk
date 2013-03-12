%{
#include <stdio.h>
#include "debug.h"
#include "stalk.h"
#include "parse.h"
#include "syntax.h"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

extern int yylex();
extern int yyparse();
void yyerror(sl_s_expr_t **head, yyscan_t scanner, char *err);
extern int yylineno_extern;
%}


%pure-parser
%lex-param { yyscan_t scanner }
%parse-param { sl_s_expr_t **head }
%parse-param { yyscan_t scanner }

%union {
  char* p_string;
  int p_int;
  float p_float;
  void* p_node;
}

%token SL_P_IDENT
%token SL_P_ASSIGN
%token <p_int> SL_P_INTEGER
%token <p_float> SL_P_DECIMAL
%token <p_string> SL_P_STRING
%token <p_string> SL_P_SYMBOL
%token SL_P_COMMA
%token SL_P_LBRACK
%token SL_P_RBRACK
%token SL_P_VERT
%token SL_P_LPAREN
%token SL_P_RPAREN
%token SL_P_LSQ
%token SL_P_RSQ
%token SL_P_TERMINAL
%token <p_string> SL_P_KEYWORD
%token SL_P_CONT
%token SL_P_SWS
%token SL_P_COMMENT
%token <p_string> SL_P_OPERATOR

%left SL_P_IDENT

%type <p_node> main;
%type <p_node> expr;
%type <p_node> exprs;
%type <p_node> literal;
%type <p_node> ident;
%type <p_node> subexpr;
%type <p_node> message;
%type <p_node> messages;
%type <p_node> keyword;
%type <p_node> keywords;
%type <p_node> keyword_pair;

%%

main: exprs { *head = $1; };
exprs: expr exprs {
  sl_s_expr_t* left = $1;
  sl_s_expr_t* right = $2;
  // sl_s_expr_unshift(right, left);
  left->next = right;
  right->prev = left;
};
exprs: expr { $$ = $1; };

expr: messages SL_P_COMMENT SL_P_TERMINAL { $$ = $1 };
expr: messages SL_P_TERMINAL { $$ = $1; };


subexpr: literal { $$ = $1; }
       | ident { $$ = $1; }
       ;
subexpr: SL_P_LPAREN expr SL_P_RPAREN { $$ = $2; };

block: SL_P_LBRACK block_inside SL_P_RBRACK;
block_inside: block_header block_body;
block_header: SL_P_VERT block_header_inside SL_P_VERT;
block_header_inside: ident SL_P_COMMA block_header_inside;
block_header_inside: ident;
block_body: exprs;

messages: message cont messages {
  sl_s_message_unshift($3, $1);
  $$ = $3;
};
messages: message sws messages {
  sl_s_message_unshift($3, $1);
  $$ = $3;
};
messages: message {
  $$ = $1;
};

message: ident sws SL_P_ASSIGN cont subexpr;
message: ident sws SL_P_ASSIGN sws subexpr;
message: keywords {
  sl_s_message_t* m = sl_s_message_new();
  m->head = $1;
  $$ = m;
};
message: subexpr SL_P_OPERATOR subexpr;
message: subexpr {
  sl_s_message_t* m = sl_s_message_new();
  m->head = $1;
  $$ = m;
}

keywords: keyword_pair cont keywords {
  sl_s_sym_t* left = $1;
  sl_s_sym_t* right = $3;
  sl_s_expr_t* left_value = left->next;
  left_value->next = right;
  right->prev = left_value;
  $$ = left;
};
keywords: keyword_pair sws keywords {
  sl_s_sym_t* left = $1;
  sl_s_sym_t* right = $3;
  sl_s_expr_t* left_value = left->next;
  left_value->next = right;
  right->prev = left_value;
  $$ = left;
};
keywords: keyword_pair { $$ = $1; };
keyword_pair: keyword sws subexpr {
  sl_s_expr_t* se = $3;
  sl_s_sym_t*  kw = $1;
  kw->next = se;
  se->prev = kw;
  $$ = kw;
};
keyword: SL_P_KEYWORD {
  sl_s_sym_t* s = sl_s_sym_new();
  s->value = $1;
  $$ = s;
};


literal: SL_P_INTEGER {
  sl_s_int_t* s = sl_s_int_new();
  s->value = $1;
  $$ = s;
};
literal: SL_P_DECIMAL {
  sl_s_float_t* s = sl_s_float_new();
  s->value = $1;
  $$ = s;
};
literal: SL_P_STRING {
  sl_s_string_t* s = sl_s_string_new();
  s->value = $1;
  $$ = s;
};
literal: SL_P_SYMBOL {
  DEBUG("sym: %s", $1);
  sl_s_sym_t* s = sl_s_sym_new();
  s->value = $1;
  $$ = s;
};
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

void yyerror(sl_s_expr_t **head, yyscan_t scanner, char *err) {
  DEBUG("Parse error on line %d: %s", yylineno_extern, err);
}
