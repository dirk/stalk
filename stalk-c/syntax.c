#import <stdlib.h>
#import <stdio.h>
#import <assert.h>

#include "debug.h"

#include "stalk.h"
#include "syntax.h"
#include "data.h"

void* sl_s_base_gen_new(sl_syntax_type type, size_t size) {
  sl_s_base_t* b = malloc(size);
  assert(b != NULL);
  b->type = type;
  b->next = NULL;
  b->prev = NULL;
  return b;
}

sl_s_base_t* sl_s_base_new() {
  return sl_s_base_gen_new(SL_SYNTAX_BASE, sizeof(sl_s_base_t));
}

sl_s_int_t* sl_s_int_new() {
  sl_s_int_t* s = sl_s_base_gen_new(SL_SYNTAX_INT, sizeof(sl_s_int_t));
  s->value = 0;
  s->hint = NULL;
  return s;
}

sl_s_float_t* sl_s_float_new() {
  sl_s_float_t* s = sl_s_base_gen_new(SL_SYNTAX_FLOAT, sizeof(sl_s_float_t));
  s->value = 0;
  s->hint = NULL;
  return s;
}

sl_s_string_t* sl_s_string_new() {
  sl_s_string_t* s = sl_s_base_gen_new(SL_SYNTAX_STRING, sizeof(sl_s_string_t));
  s->value = NULL;//cstring
  s->hint = NULL;
  return s;
}

sl_s_expr_t* sl_s_expr_new() {
  sl_s_expr_t* s = sl_s_base_gen_new(SL_SYNTAX_EXPR, sizeof(sl_s_expr_t));
  s->head = NULL;
  return s;
}
void sl_s_expr_unshift(sl_s_expr_t* expr, sl_s_base_t* s) {
  DL_PREPEND(expr->head, s);
}
sl_s_message_t* sl_s_message_new() {
  sl_s_message_t* s = sl_s_base_gen_new(
    SL_SYNTAX_MESSAGE, sizeof(sl_s_message_t)
  );
  s->head = NULL;
  return s;
}
void sl_s_message_unshift(sl_s_message_t* message, sl_s_base_t* s) {
  DL_PREPEND(message->head, s);
}

sl_s_sym_t* sl_s_sym_new() {
  sl_s_sym_t* s = sl_s_base_gen_new(SL_SYNTAX_SYM, sizeof(sl_s_sym_t));
  s->value = NULL;//cstring
  s->literal = false;
  s->hint = NULL;
  return s;
}
void sl_s_expr_free(sl_s_expr_t* s) {
  free(s);
}
void sl_s_sym_free(sl_s_sym_t* s) {
  free(s->value);
  free(s);
}

static inline void* sl_s_expr_eval(sl_s_expr_t* expr, void* scope) {
  sl_s_base_t* s = expr->head;
  
  // TODO: Make this actually do stuff besides just looking up a method and
  //       calling its primitive.
  
  while(s != NULL) {
    void* _ret = sl_s_eval(s, scope);
    if(_ret != NULL) {
      sl_d_obj_t* ret_raw = (sl_d_obj_t*)_ret;
      if(ret_raw->type == SL_DATA_SYM) {
        sl_d_sym_t* ret = (sl_d_sym_t*)ret_raw;
        
        sl_d_obj_t* _method = sl_d_obj_get_method(scope, ret);
        if(_method != NULL) {
          sl_d_method_t* method = (sl_d_method_t*)_method;
          if(method->hint != NULL) {
            return (*method->hint)(scope, NULL);
          } else {
            // TODO: Regular method calling
          }
        }
  
        // TODO: Lookups and stuff
        
        
        
      }
    }
    s = s->next;
  }
  return NULL;
}
static inline void* sl_s_sym_hint(sl_s_sym_t* s) {
  if(s->hint == NULL) {
    char *value = s->value;
    if(value[0] == ':') {
      value = &s->value[1];
      s->literal = true;
    } else {
      s->literal = false;
    }
    sl_d_sym_t *sym = sl_d_sym_new(value);//cstring
    s->hint = sym;
    
    sl_d_sym_t* _sym = (sl_d_sym_t*)s->hint;
    DEBUG("new sym(id=%u, sym_id=%u, value(%d)=%.*s)",
      _sym->id,
      _sym->sym_id,
      _sym->length,
      _sym->length,
      _sym->value
    );
    // char* buff = sl_i_sym_value_to_cstring(_sym);
    // printf("buff = %s\n", buff);
    // free(buff);
    // return sym;
    return NULL;
  } else {
    DEBUG("hinted sym(id=%u)", ((sl_d_sym_t*)s->hint)->id);
    // return (sl_d_sym_t*)s->hint;
    return NULL;
  }
}
static inline void* sl_s_sym_eval(sl_s_sym_t* s, void* scope) {
  sl_s_sym_hint(s);
  return s->hint;
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
