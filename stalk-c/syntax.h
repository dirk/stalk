#ifndef STALK_H
#define STALK_H

#include "deps/uthash/src/utlist.h"

typedef unsigned char sl_type_int;

#define SL_TYPE_EXPR    0
#define SL_TYPE_SYM     1
#define SL_TYPE_STR     2
#define SL_TYPE_INT     3
#define SL_TYPE_FLOAT   4
#define SL_TYPE_BLOCK   5
#define SL_TYPE_COMMENT 6

#define SL_TYPE sl_type_int type;
#define SL_LINKS void *next; void *prev;

typedef struct sl_expression {
  SL_TYPE
  SL_LINKS
  void *head;// Start of inner expression
  void *tail;// End of inner expression
} sl_expression_t;

typedef struct sl_sym {
  SL_TYPE
  SL_LINKS
} sl_sym_t;

#endif
