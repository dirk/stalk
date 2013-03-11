#import <stdlib.h>
#import <stdio.h>
#import <assert.h>

#include "stalk.h"
#include "data.h"
#include "syntax.h"

sl_s_expr_t* sl_s_expr_init() {
  sl_s_expr_t* s = malloc(sizeof(sl_s_expr_t));
  assert(s != NULL);
  s->type = SL_SYNTAX_EXPR;
  s->next = NULL;
  s->prev = NULL;
  s->head = NULL;
  s->tail = NULL;
  return s;
}
void sl_s_expr_free(sl_s_expr_t* s) {
  free(s);
}

static inline void sl_s_expr_eval(sl_s_expr_t* s, void* scope) {
  printf("there\n");
}
void sl_s_eval(void* _s, void* scope) {
  // Cast it into the base struct
  sl_s_base_t* s = _s;
  
  sl_syntax_type type = s->type;
  switch(type) {
  case SL_SYNTAX_EXPR:
    sl_s_expr_eval((sl_s_expr_t*)s, scope);  
  default:
    break;
  }
}

void test() {
  sl_s_expr_t* s = sl_s_expr_init();
  
  void* scope = malloc(sizeof(NULL));
  
  sl_s_eval(s, scope);
  
  free(scope);
  
  sl_s_expr_free(s);
}
