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

typedef struct sl_s_base {
  SL_SYNTAX_TYPE
  SL_SYNTAX_LINKS
} sl_s_base_t;

typedef struct sl_s_expr {
  SL_SYNTAX_TYPE
  SL_SYNTAX_LINKS
  void *head;// Start of inner expression
  void *tail;// End of inner expression
} sl_s_expr_t;

typedef struct sl_s_sym {
  SL_SYNTAX_TYPE
  SL_SYNTAX_LINKS
  void* hint;//sl_d_sym_t
} sl_s_sym_t;

sl_s_expr_t* sl_s_expr_init();
void test();

#endif
