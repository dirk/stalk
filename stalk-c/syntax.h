#ifndef SYNTAX_H
#define SYNTAX_H

#include "deps/uthash/src/utlist.h"

typedef unsigned char sl_syntax_type;

#define SL_SYNTAX_EXPR    0
#define SL_SYNTAX_SYM     1
#define SL_SYNTAX_STR     2
#define SL_SYNTAX_INT     3
#define SL_SYNTAX_FLOAT   4
#define SL_SYNTAX_BLOCK   5
#define SL_SYNTAX_COMMENT 6

#define SL_SYNTAX_TYPE sl_syntax_type type;
#define SL_SYNTAX_LINKS void *next; void *prev;
#define SL_SYNTAX_HEADER SL_SYNTAX_TYPE SL_SYNTAX_LINKS

typedef struct sl_s_base {
  SL_SYNTAX_HEADER;
} sl_s_base_t;

typedef struct sl_s_expr {
  SL_SYNTAX_HEADER;
  sl_s_base_t *head;// Start of inner expression
  // sl_s_base_t *tail;// End of inner expression
} sl_s_expr_t;

typedef struct sl_s_list {
  SL_SYNTAX_HEADER;
  sl_s_base_t *head;// Start of inner list of sl_s_sym_t's
} sl_s_list_t;

typedef struct sl_s_sym {
  SL_SYNTAX_HEADER;
  char *value;
  void* hint;//sl_d_sym_t
} sl_s_sym_t;

sl_s_expr_t* sl_s_expr_new();
sl_s_sym_t* sl_s_sym_new();
void sl_s_expr_free(sl_s_expr_t* s);
void sl_s_sym_free(sl_s_sym_t* s);
void* sl_s_eval(void* _s, void* scope);

#endif
