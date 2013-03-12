#ifndef SYNTAX_H
#define SYNTAX_H

#include "deps/uthash/src/utlist.h"

typedef unsigned char sl_syntax_type;

#define SL_SYNTAX_EXPR    0
#define SL_SYNTAX_SYM     1
#define SL_SYNTAX_STRING  2
#define SL_SYNTAX_INT     3
#define SL_SYNTAX_FLOAT   4
#define SL_SYNTAX_BLOCK   5
#define SL_SYNTAX_COMMENT 6
#define SL_SYNTAX_BASE    7
#define SL_SYNTAX_MESSAGE 8

#define SL_SYNTAX_TYPE sl_syntax_type type;
#define SL_SYNTAX_LINENO int lineno;
#define SL_SYNTAX_LINKS void *next; void *prev;
#define SL_SYNTAX_HEADER SL_SYNTAX_TYPE SL_SYNTAX_LINENO SL_SYNTAX_LINKS

typedef struct sl_s_base {
  SL_SYNTAX_HEADER;
} sl_s_base_t;

typedef struct sl_s_expr {
  SL_SYNTAX_HEADER;
  sl_s_base_t *head;// Start of inner expression
  // sl_s_base_t *tail;// End of inner expression
} sl_s_expr_t;

typedef struct sl_s_message {
  SL_SYNTAX_HEADER;
  sl_s_base_t* head;
} sl_s_message_t;

typedef struct sl_s_list {
  SL_SYNTAX_HEADER;
  sl_s_base_t *head;// Start of inner list of sl_s_sym_t's
} sl_s_list_t;

typedef struct sl_s_sym {
  SL_SYNTAX_HEADER;
  char *value;//cstring
  bool literal;
  void* hint;//sl_d_sym_t
} sl_s_sym_t;

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

void* sl_s_base_gen_new(sl_syntax_type type, size_t size);
sl_s_base_t* sl_s_base_new();

sl_s_message_t* sl_s_message_new();
sl_s_int_t* sl_s_int_new();
sl_s_float_t* sl_s_float_new();
sl_s_string_t* sl_s_string_new();
sl_s_sym_t* sl_s_sym_new();

sl_s_expr_t* sl_s_expr_new();
void sl_s_expr_unshift(sl_s_expr_t* expr, sl_s_base_t* s);

sl_s_message_t* sl_s_message_new();
void sl_s_message_unshift(sl_s_message_t* message, sl_s_base_t* s);

void sl_s_expr_free(sl_s_expr_t* s);
void sl_s_sym_free(sl_s_sym_t* s);
void* sl_s_eval(void* _s, void* scope);

#endif
