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
void yyerror(YYLTYPE* loc, sl_s_expr_t **head, yyscan_t scanner, char* source, const char *err);
extern int yylineno_extern;

// %glr-parser
%}

%code requires {
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#define SOURCE(a) a->source = source; a->line = (yylloc.first_line);
}

%locations
%pure-parser
%lex-param { yyscan_t scanner }
%parse-param { sl_s_expr_t **head }
%parse-param { yyscan_t scanner }
%parse-param { char* source }

%error-verbose

%union {
  char* p_string;
  int   p_int;
  float p_float;
  void* p_node;
}

%token <p_string> SL_T_IDENT
%token <p_int> SL_T_INTEGER
%token <p_float> SL_T_DECIMAL
%token <p_string> SL_T_STRING
%token <p_string> SL_T_SYMBOL
%token SL_T_COMMA
%token SL_T_LBRACK
%token SL_T_RBRACK
%token SL_T_VERT
%token SL_T_LPAREN
%token SL_T_RPAREN
%token SL_T_LSQ
%token SL_T_RSQ
%token SL_T_TERMINAL
%token <p_string> SL_T_KEYWORD
%token SL_T_SEP
%token SL_T_COMMENT
%token <p_string> SL_T_OPERATOR
%token SL_T_DEF
%token <p_string> SL_T_ASSIGN;


%left SL_T_LPAREN;
%left SL_T_ASSIGN;
%left SL_T_KEYWORD;
%left SL_T_TERMINAL;

%type <p_node> main
%type <p_node> plain_expr;
%type <p_node> expr
%type <p_node> exprs
%type <p_node> literal
%type <p_node> ident
%type <p_node> subexpr;
%type <p_node> message;
%type <p_node> messages
%type <p_node> keyword_pair;
%type <p_node> block;
%type <p_node> block_body;
%type <p_node> block_inside;

%%
main: exprs { /* DEBUG("main exprs = %p", $1); */ *head = $1; };

exprs: expr exprs {
  sl_s_expr_t* left = $1;
  sl_s_expr_t* right = $2;
  if(left == NULL) {
    $$ = right;
  } else if(right == NULL) {
    $$ = left;
  } else {
    left->next = right;
    right->prev = left;
    $$ = left;
  }
};
exprs: expr { $$ = $1; };

expr: plain_expr SL_T_TERMINAL {
  $$ = $1;
};

plain_expr: messages {
  sl_s_base_t* msg = $1;
  if(msg->type != SL_SYNTAX_MESSAGE) {
    LOG_ERR("Expression must contain messages (currently %d)", msg->type);
  }
  SOURCE(msg);
  sl_s_expr_t* e = sl_s_expr_new();
  e->head = msg;
  $$ = e;
}

subexpr: literal { $$ = $1; }
       | ident { $$ = $1; }
       ;
subexpr: SL_T_LPAREN plain_expr SL_T_RPAREN { $$ = $2; };
subexpr: SL_T_LPAREN subexpr SL_T_RPAREN { $$ = $2; };


messages: message messages {
  sl_s_message_t* left  = (sl_s_message_t*)$1;
  sl_s_message_t* right = (sl_s_message_t*)$2;
  if(left->type != SL_SYNTAX_MESSAGE) {
    LOG_ERR("Left must be message (currently %d)", left->type);
  }
  if(right->type != SL_SYNTAX_MESSAGE) {
    LOG_ERR("Right must be message (currently %d)", right->type);
  }
  if(left->head == NULL) {
    LOG_ERR("Left head is null");
  }
  if(right->head == NULL) {
    LOG_ERR("Right head is null");
  }
  SOURCE(left);
  SOURCE(right);
  left->next = right;
  right->prev = left;
  $$ = left;
};
messages: message {
  sl_s_message_t* msg = (sl_s_message_t*)$1;
  if(msg->type != SL_SYNTAX_MESSAGE) {
    LOG_ERR("Message must be message (currently %d)", msg->type);
  }
  if(msg->head == NULL) {
    LOG_ERR("Message head is null");
  }
  SOURCE(msg);
  $$ = msg;
};



message: SL_T_DEF {
  sl_s_message_t* msg = sl_s_message_new();
  sl_s_sym_t* def = sl_s_sym_new();
  def->value = "def:";
  msg->head = (sl_s_base_t*)def;
  SOURCE(msg);
  $$ = msg;
};
message: keyword_pair {
  sl_s_sym_t* head = $1;
  sl_s_message_t* msg = sl_s_message_new();
  msg->head = (sl_s_base_t*)head;
  msg->keyword = true;
  SOURCE(msg);
  $$ = msg;
};
message: ident SL_T_ASSIGN subexpr {
  sl_s_message_t* msg = sl_s_message_new();
  
  sl_s_sym_t* assign = sl_s_sym_new();
  assign->value = "=";
  assign->assign = true;
  sl_s_sym_t* ident = $1;
  sl_s_expr_t* value = $3;
  
  assign->next = ident;
  ident->prev = assign;
  
  ident->next = value;
  value->prev = ident;
  
  msg->head = (sl_s_base_t*)assign;
  SOURCE(msg);
  $$ = msg;
};
message: SL_T_OPERATOR subexpr {
  sl_s_sym_t* op = sl_s_sym_new();
  op->value = $1;
  op->operator = true;
  sl_s_expr_t* value = $2;
  
  op->next = value;
  value->prev = op;
  
  sl_s_message_t* msg = sl_s_message_new();
  msg->head = (sl_s_base_t*)op;
  SOURCE(msg);
  $$ = msg;
};
message: subexpr {
  sl_s_message_t* msg = sl_s_message_new();
  sl_s_base_t* sub = $1;
  if(sub->type == SL_SYNTAX_MESSAGE) {
    LOG_ERR("Subexpression may not contain messages (%d)", msg->type);
  }
  msg->head = sub;
  SOURCE(msg);
  $$ = msg;
}


keyword_pair: SL_T_KEYWORD subexpr {
  sl_s_sym_t* kw = sl_s_sym_new();
  kw->value = $1;
  kw->keyword = true;
  sl_s_expr_t* se = $2;
  
  kw->next = se;
  se->prev = kw;
  $$ = kw;
};

literal: SL_T_INTEGER {
  sl_s_int_t* s = sl_s_int_new();
  s->value = $1;
  $$ = s;
};
literal: SL_T_DECIMAL {
  sl_s_float_t* s = sl_s_float_new();
  s->value = $1;
  $$ = s;
};
literal: SL_T_STRING {
  sl_s_string_t* s = sl_s_string_new();
  const char* str = $1;
  int len = strlen(str);
  s->value = strndup(&str[1], len - 2);
  $$ = s;
};
literal: SL_T_SYMBOL {
  sl_s_sym_t* s = sl_s_sym_new();
  s->literal = true;
  s->value = $1;
  $$ = s;
};

literal: block { $$ = $1; };
literal: array { $$ = NULL; };

array: SL_T_LSQ array_inside SL_T_RSQ;
array: SL_T_LSQ SL_T_RSQ;
array_inside: subexpr SL_T_COMMA array_inside;
array_inside: subexpr;

block: SL_T_LBRACK block_inside SL_T_RBRACK {
  sl_s_block_t* b = sl_s_block_new();
  sl_s_expr_t* head = $2;
  b->head = head;
  $$ = b;
};
block_inside: block_header block_body { $$ = $2; };
block_inside: block_body { $$ = $1; };
block_header: SL_T_VERT block_header_inside SL_T_VERT;
block_header_inside: ident SL_T_COMMA block_header_inside;
block_header_inside: ident;

block_body: plain_expr SL_T_TERMINAL block_body {
  sl_s_expr_t* left = $1;
  sl_s_expr_t* right = $3;
  
  if(left->type != SL_SYNTAX_EXPR) {
    LOG_ERR("Must be an expression (currently %d)", left->type);
  }
  if(right->type != SL_SYNTAX_EXPR) {
    LOG_ERR("Must be an expression (currently %d)", right->type);
  }
  
  left->next = right;
  right->prev = left;
  $$ = left;
};
block_body: plain_expr { $$ = $1; };

ident: SL_T_IDENT {
  sl_s_sym_t* s = sl_s_sym_new();
  s->value = $1;
  $$ = s;
};

%%

void yyerror(YYLTYPE* loc, sl_s_expr_t **head, yyscan_t scanner, char* source, const char *err) {
  DEBUG("Parse error in %s on line %d: %s", source, yylineno_extern, err);
}
