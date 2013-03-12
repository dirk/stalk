#import <stdlib.h>
#import <stdio.h>
#import <assert.h>

#include "debug.h"

#include "stalk.h"
#include "data.h"
#include "syntax.h"

sl_s_expr_t* sl_s_expr_new() {
  sl_s_expr_t* s = malloc(sizeof(sl_s_expr_t));
  assert(s != NULL);
  s->type = SL_SYNTAX_EXPR;
  s->next = NULL;
  s->prev = NULL;
  s->head = NULL;
  s->tail = NULL;
  return s;
}
sl_s_sym_t* sl_s_sym_new() {
  sl_s_sym_t* s = malloc(sizeof(sl_s_expr_t));
  assert(s != NULL);
  s->type = SL_SYNTAX_SYM;
  s->next = NULL;
  s->prev = NULL;
  s->value = NULL;
  s->hint = NULL;
  return s;
}
void sl_s_expr_free(sl_s_expr_t* s) {
  free(s);
}
void sl_s_sym_free(sl_s_sym_t* s) {
  free(s);
}

static inline void* sl_s_expr_eval(sl_s_expr_t* s, void* scope) {
  return NULL;
}
static inline void* sl_s_sym_eval(sl_s_sym_t* s, void* scope) {
  if(s->hint == NULL) {
    sl_d_sym_t *sym = sl_d_sym_new(s->value);
    s->hint = sym;
    
    sl_d_sym_t* _sym = (sl_d_sym_t*)s->hint;
    DEBUG("new sym(id=%u, sym_id=%u, value=%*s)",
      _sym->id,
      _sym->sym_id,
      _sym->length,
      _sym->value
    );
    // char* buff = sl_i_sym_value_to_cstring(_sym);
    // printf("buff = %s\n", buff);
    // free(buff);
    return sym;
  } else {
    DEBUG("hinted sym(id=%u)", ((sl_d_sym_t*)s)->id);
    return (sl_d_sym_t*)s->hint;
  }
  
}
void* sl_s_eval(void* _s, void* scope) {
  // Cast it into the base struct
  sl_s_base_t* s = _s;
  
  sl_syntax_type type = s->type;
  switch(type) {
  case SL_SYNTAX_EXPR:
    return sl_s_expr_eval((sl_s_expr_t*)s, scope);
  case SL_SYNTAX_SYM:
    return sl_s_sym_eval((sl_s_sym_t*)s, scope);
  default:
    SENTINEL("Unknown expression type: %d", (int)type);
  }
  return NULL;
error:
  return NULL;
}
