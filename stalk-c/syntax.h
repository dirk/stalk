#ifndef SYNTAX_H
#define SYNTAX_H

#include "deps/uthash/src/utlist.h"

typedef unsigned char sl_syntax_type;
typedef unsigned char sl_message_type;

#define SL_SYNTAX_EXPR    0
#define SL_SYNTAX_SYM     1
#define SL_SYNTAX_STRING  2
#define SL_SYNTAX_INT     3
#define SL_SYNTAX_FLOAT   4
#define SL_SYNTAX_BLOCK   5
#define SL_SYNTAX_COMMENT 6
#define SL_SYNTAX_BASE    7
#define SL_SYNTAX_MESSAGE 8
#define SL_SYNTAX_ASSIGN  9
#define SL_SYNTAX_DEF     10
#define SL_SYNTAX_KEYWORD 11

#define SL_SYNTAX_TYPE sl_syntax_type type;
#define SL_SYNTAX_LINENO int lineno;
#define SL_SYNTAX_LINKS void *next; void *prev;
#define SL_SYNTAX_SOURCE char* source; int line;
#define SL_SYNTAX_HEADER SL_SYNTAX_TYPE SL_SYNTAX_LINENO SL_SYNTAX_LINKS SL_SYNTAX_SOURCE

typedef struct sl_s_base {
  SL_SYNTAX_HEADER;
} sl_s_base_t;

typedef struct sl_s_message {
  SL_SYNTAX_HEADER;
  sl_s_base_t* head;
  bool keyword;
  void*  hint;//sl_d_message_t
  void*  hint_args;// Pointer to an array of arg expressions
} sl_s_message_t;



typedef struct sl_s_expr {
  SL_SYNTAX_HEADER;
  // sl_s_base_t *target;// Target for evaluating messages
  sl_s_base_t *head;
  // sl_s_base_t *tail;// End of inner expression
} sl_s_expr_t;

typedef struct sl_s_keyword {
  SL_SYNTAX_HEADER;
  char* value;//cstring
  void* sym;//sl_d_sym_t
} sl_s_keyword_t;

typedef struct sl_s_sym {
  SL_SYNTAX_HEADER;
  char *value;//cstring
  bool literal;
  bool operator;
  bool assign;
  bool keyword;
  void* hint;//sl_d_sym_t
  void* message_hint;//sl_d_message_t (used by sl_s_sym_eval)
} sl_s_sym_t;



typedef struct sl_s_list {
  SL_SYNTAX_HEADER;
  sl_message_type message_type;
  sl_s_sym_t *head;// Start of inner list of sl_s_sym_t's
} sl_s_list_t;

typedef struct sl_s_string {
  SL_SYNTAX_HEADER;
  char *value;//cstring
  void* hint;//sl_d_sym_t
} sl_s_string_t;

typedef struct sl_s_int {
  SL_SYNTAX_HEADER;
  int value;
  void* hint;//sl_d_int_t
} sl_s_int_t;

typedef struct sl_s_float {
  SL_SYNTAX_HEADER;
  int value;
  void* hint;//sl_d_int_t
} sl_s_float_t;

typedef struct sl_s_block {
  SL_SYNTAX_HEADER;
  sl_s_expr_t* head;//inside of block
} sl_s_block_t;

typedef struct sl_s_assign {
  SL_SYNTAX_HEADER;
  char *name;//c_string
  void* name_sym;//sl_d_sym_t
  void* value;//sl_d_expr_t
  void* hint;// Item slot for assignment?
} sl_s_assign_t;

typedef struct sl_s_def {
  SL_SYNTAX_HEADER;
  sl_s_base_t* head;//Head of keyword pairs
  sl_s_block_t* block;//Associated block
  void* signature_hint;//sl_d_sym_t: Hint for compiled signature.
  void* block_hint;//sl_d_block_t
} sl_s_def_t;



void* sl_s_base_gen_new(sl_syntax_type type, size_t size);
sl_s_base_t* sl_s_base_new();

sl_s_message_t* sl_s_message_new();
sl_s_int_t* sl_s_int_new();
sl_s_float_t* sl_s_float_new();
sl_s_string_t* sl_s_string_new();
sl_s_keyword_t* sl_s_keyword_new();
sl_s_sym_t* sl_s_sym_new();
sl_s_assign_t* sl_s_assign_new();
sl_s_def_t* sl_s_def_new();
sl_s_block_t* sl_s_block_new();

sl_s_expr_t* sl_s_expr_new();
void sl_s_expr_unshift(sl_s_expr_t* expr, sl_s_base_t* s);

void sl_s_message_unshift(sl_s_message_t* message, sl_s_base_t* s);

void sl_s_expr_free(sl_s_expr_t* s);
void sl_s_sym_free(sl_s_sym_t* s);

void* sl_s_eval(void* _s, void* scope);
void* sl_s_expr_eval(sl_s_expr_t* expr, void* scope);
void* sl_s_expr_eval_shallow(sl_s_expr_t* expr, void* scope);
void* sl_s_int_eval(sl_s_int_t* s, void* scope);
void* sl_s_string_eval(sl_s_string_t* s, void* scope);

#endif
