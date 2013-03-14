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

#ifndef YYLTYPE_IS_DECLARED
#define YYLTYPE_IS_DECLARED
typedef struct YYLTYPE {
  char yydummy;
} YYLTYPE;
#endif;

extern int yylex();
extern int yyparse();
void yyerror(sl_s_expr_t **head, yyscan_t scanner, const char *err);
extern int yylineno_extern;
%}

%glr-parser
%pure-parser
%lex-param { yyscan_t scanner }
%parse-param { sl_s_expr_t **head }
%parse-param { yyscan_t scanner }

%error-verbose

%union {
  char* p_string;
  int p_int;
  float p_float;
  void* p_node;
}

%token <p_string> SL_P_IDENT
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
%token SL_P_SEP
%token SL_P_COMMENT
%token <p_string> SL_P_OPERATOR
%token SL_P_DEF
%token <p_string> SL_P_ASSIGN

%type <p_node> main
%type <p_node> plain_expr;
%type <p_node> expr
%type <p_node> exprs
%type <p_node> literal
%type <p_node> ident
%type <p_node> subexpr;
%type <p_node> message
%type <p_node> messages
%type <p_node> keywords
%type <p_node> keyword_pair;
%type <p_node> block;
%type <p_node> block_body;
%type <p_node> block_body_inside;
%type <p_node> block_inside;
%type <p_node> target;


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

expr_terminal: SL_P_COMMENT SL_P_TERMINAL | SL_P_TERMINAL;

plain_expr: target SL_P_SEP messages {
  sl_s_message_t* m = $3;
  sl_s_base_t* t = $1;
  
  sl_s_expr_t* e = sl_s_expr_new();
  e->target = t;
  e->messages = m;
  $$ = e;
}
plain_expr: messages {
  sl_s_message_t* m = $1;
  sl_s_expr_t* e = sl_s_expr_new();
  e->target = NULL;
  e->messages = m;
  $$ = e;
}

expr: plain_expr expr_terminal {
  $$ = $1;
};
expr: SL_P_COMMENT SL_P_TERMINAL { $$ = NULL; };

target: literal { $$ = $1; };
target: SL_P_LPAREN plain_expr SL_P_RPAREN { $$ = $2; };
target: SL_P_LPAREN subexpr SL_P_RPAREN { $$ = $2; };

subexpr: literal { $$ = $1; }
       | ident { $$ = $1; }
       ;
subexpr: SL_P_LPAREN plain_expr SL_P_RPAREN { $$ = $2; };
subexpr: SL_P_LPAREN subexpr SL_P_RPAREN { $$ = $2; };


messages: message SL_P_SEP messages {
  sl_s_message_t* left  = (sl_s_message_t*)$1;
  sl_s_message_t* right = (sl_s_message_t*)$3;
  if(left->type != SL_SYNTAX_MESSAGE) {
    DEBUG("Left must be message (currently %d)", left->type);
  }
  if(right->type != SL_SYNTAX_MESSAGE) {
    DEBUG("Right must be message (currently %d)", right->type);
  }
  if(left->head == NULL) {
    DEBUG("Left head is null");
  }
  if(right->head == NULL) {
    DEBUG("Right head is null");
  }
  left->next = right;
  right->prev = left;
  $$ = left;
};
messages: message {
  sl_s_message_t* m = (sl_s_message_t*)$1;
  if(m->type != SL_SYNTAX_MESSAGE) {
    DEBUG("Message must be message (currently %d)", m->type);
  }
  if(m->head == NULL) {
    DEBUG("Message head is null");
  }
  $$ = m;
};
/*
message types:
sl_s_message_t;//calls (operator, message, assign, def)
sl_s_expr_t;

message heads:
sl_s_sym_t;
*/
message: ident {
  sl_s_message_t* msg = sl_s_message_new();
  sl_s_base_t* ident = $1;
  msg->head = ident;
  $$ = msg;
}
message: SL_P_DEF SL_P_SEP message SL_P_SEP block {
  sl_s_message_t* params = $3;
  if(params->type != SL_SYNTAX_MESSAGE) {
    yyerror(head, scanner, "Message must follow def:");
    YYERROR;
  }
  
  /* flatten message into sequence of symbols */
  sl_s_base_t* part = (sl_s_base_t*)params->head;
  while(part != NULL) {
    if(part->type == SL_SYNTAX_EXPR) {
      sl_s_expr_t* expr = (sl_s_expr_t*)part;
      if(expr->target != NULL) {
        yyerror(head, scanner, "Value in definition may not have target");
        YYERROR;
      }
      sl_s_base_t* ident = expr->messages->head;
      if(ident->type != SL_SYNTAX_SYM) {
        yyerror(head, scanner, "Identifier must follow keyword in def:");
        YYERROR;
      } else {
        sl_s_base_t* prev = (sl_s_base_t*)part->prev;
        ident->prev = prev;
        if(prev != NULL) {
          prev->next = ident;
        } else {
          yyerror(head, scanner, "Previous is null in message symbol checker");
          YYERROR;
        }
        
        sl_s_base_t* next = (sl_s_base_t*)part->next;
        ident->next = next;
        if(next != NULL) {
          next->prev = ident;
        }
        
        sl_s_expr_free((sl_s_expr_t*)part);
        part = ident;
      }
    } else if(part->type == SL_SYNTAX_SYM) {
      // DEBUG("part sym = %p", part);
      // pass
    } else {
      yyerror(head, scanner, "Unexpected value type in def:");
      YYERROR;
    }
    part = part->next;
  }
  
  sl_s_sym_t* def = sl_s_sym_new();
  def->value = "def:";
  sl_s_block_t* bl = $5;
  
  def->next    = params;
  params->prev = def;
  
  params->next = bl;
  bl->prev     = params;
  
  sl_s_message_t* msg = sl_s_message_new();
  msg->head = (sl_s_base_t*)def;
  
  $$ = msg;
};
message: ident SL_P_SEP SL_P_ASSIGN SL_P_SEP subexpr {
  sl_s_message_t* msg = sl_s_message_new();
  
  sl_s_sym_t* assign = sl_s_sym_new();
  assign->value = "=";
  assign->assign = true;
  sl_s_sym_t* ident = $1;
  sl_s_expr_t* value = $5;
  
  assign->next = ident;
  ident->prev = assign;
  
  ident->next = value;
  value->prev = ident;
  
  msg->head = (sl_s_base_t*)assign;
  $$ = msg;
};
message: SL_P_OPERATOR SL_P_SEP subexpr {
  sl_s_sym_t* op = sl_s_sym_new();
  op->value = $1;
  op->operator = true;
  sl_s_expr_t* value = $3;
  
  op->next = value;
  value->prev = op;
  
  sl_s_message_t* msg = sl_s_message_new();
  msg->head = (sl_s_base_t*)op;
  $$ = msg;
};
message: keywords {
  sl_s_sym_t* head = $1;
  sl_s_message_t* msg = sl_s_message_new();
  msg->head = (sl_s_base_t*)head;
  $$ = msg;
};



/* Keywords generates a regular list just like any other expression but it also
verifies the correctness of the syntax ahead of time so we can interpret faster
later on. */
keywords: keyword_pair SL_P_SEP keywords {
  sl_s_sym_t* left = $1;
  sl_s_sym_t* right = $3;
  sl_s_expr_t* left_value = left->next;
  left_value->next = right;
  right->prev = left_value;
  $$ = left;
};
keywords: keyword_pair { $$ = $1; };

keyword_pair: SL_P_KEYWORD SL_P_SEP subexpr {
  sl_s_sym_t* kw = sl_s_sym_new();
  kw->value = $1;
  kw->keyword = true;
  sl_s_expr_t* se = $3;
  
  kw->next = se;
  se->prev = kw;
  $$ = kw;
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
  sl_s_sym_t* s = sl_s_sym_new();
  s->value = $1;
  $$ = s;
};

literal: block { $$ = NULL; };
literal: array { $$ = NULL; };

array: SL_P_LSQ array_inside SL_P_RSQ;
array: SL_P_LSQ SL_P_RSQ;
array_inside: subexpr SL_P_COMMA array_inside;
array_inside: subexpr;

block: SL_P_LBRACK block_inside SL_P_RBRACK {
  sl_s_block_t* b = sl_s_block_new();
  sl_s_expr_t* head = $2;
  b->head = head;
  $$ = b;
};
block_inside: block_header block_body { $$ = $2 };
block_inside: block_body { $$ = $1 };
block_header: SL_P_VERT block_header_inside SL_P_VERT;
block_header_inside: ident SL_P_COMMA block_header_inside;
block_header_inside: ident;

block_body: block_body_inside { $$ = $1; };

block_body_inside: exprs plain_expr {
  sl_s_expr_t* left = $1;
  sl_s_expr_t* right = $2;
  
  while(left->next != NULL) {
    left = left->next;
  }
  left->next = right;
  right->prev = left;
  $$ = left;
};
block_body_inside: exprs { $$ = $1; };
block_body_inside: plain_expr { $$ = $1; };

ident: SL_P_IDENT {
  sl_s_sym_t* s = sl_s_sym_new();
  s->value = $1;
  $$ = s;
};

%%

void yyerror(sl_s_expr_t **head, yyscan_t scanner, const char *err) {
  DEBUG("Parse error on line %d: %s", yylineno_extern, err);
}
