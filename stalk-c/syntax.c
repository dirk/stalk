#import <stdlib.h>
#import <stdio.h>
#import <assert.h>

#include "debug.h"

#include "stalk.h"
#include "syntax.h"
#include "data.h"

SL_DATA_EXTERN;

void* sl_s_base_gen_new(sl_syntax_type type, size_t size) {
  sl_s_base_t* b = malloc(size);
  assert(b != NULL);
  b->type = type;
  b->next = NULL;
  b->prev = NULL;
  return b;
}

sl_s_base_t* sl_s_base_new() {
  LOG_WARN("Should not reach sl_s_base_new()");
  return sl_s_base_gen_new(SL_SYNTAX_BASE, sizeof(sl_s_base_t));
}

// LITERALS -------------------------------------------------------------------

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

// ASSIGN ---------------------------------------------------------------------

sl_s_assign_t* sl_s_assign_new() {
  sl_s_assign_t* a = sl_s_base_gen_new(
    SL_SYNTAX_ASSIGN, sizeof(sl_s_assign_t)
  );
  a->name = NULL;//cstring
  a->name_sym = NULL;//sl_d_sym_t
  a->value = NULL;
  a->hint = NULL;
  return a;
}

// DEF ------------------------------------------------------------------------

sl_s_def_t* sl_s_def_new() {
  sl_s_def_t* def = sl_s_base_gen_new(
    SL_SYNTAX_DEF, sizeof(sl_s_def_t)
  );
  def->head = NULL;
  def->block = NULL;
  def->signature_hint = NULL;
  def->block_hint = NULL;
  return def;
}

// EXPR -----------------------------------------------------------------------

sl_s_expr_t* sl_s_expr_new() {
  sl_s_expr_t* s = sl_s_base_gen_new(SL_SYNTAX_EXPR, sizeof(sl_s_expr_t));
  s->head = NULL;
  return s;
}
void sl_s_expr_unshift(sl_s_expr_t* expr, sl_s_base_t* s) {
  DL_PREPEND(expr->head, s);
}

// MESSAGE --------------------------------------------------------------------

sl_s_message_t* sl_s_message_new() {
  sl_s_message_t* s = sl_s_base_gen_new(
    SL_SYNTAX_MESSAGE, sizeof(sl_s_message_t)
  );
  s->head = NULL;
  s->hint = NULL;
  return s;
}
void sl_s_message_unshift(sl_s_message_t* message, sl_s_sym_t* s) {
  DL_PREPEND(message->head, s);
}

// SYMBOL ---------------------------------------------------------------------

sl_s_sym_t* sl_s_sym_new() {
  sl_s_sym_t* s = sl_s_base_gen_new(SL_SYNTAX_SYM, sizeof(sl_s_sym_t));
  s->value = NULL;//cstring
  s->literal = false;
  s->operator = false;
  s->assign = false;
  s->hint = NULL;
  return s;
}
static inline sl_d_sym_t* sl_i_sym_hint(sl_s_sym_t* s) {
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
    assert(sym == _sym);
    /*
    DEBUG("new sym(id=%u, sym_id=%u, value(%d)=%.*s)",
      _sym->id,
      _sym->sym_id,
      _sym->length,
      _sym->length,
      _sym->value
    );
    */
    // char* buff = sl_i_sym_value_to_cstring(_sym);
    // printf("buff = %s\n", buff);
    // free(buff);
    // return sym;
    return _sym;
  } else {
    // DEBUG("hinted sym(val=%s)(id=%u)", s->value, ((sl_d_sym_t*)s->hint)->id);
    // return (sl_d_sym_t*)s->hint;
    return s->hint;
  }
}

sl_s_block_t* sl_s_block_new() {
  sl_s_block_t* s = sl_s_base_gen_new(SL_SYNTAX_BLOCK, sizeof(sl_s_block_t));
  s->head = false;
  return s;
}


void sl_s_expr_free(sl_s_expr_t* s) {
  free(s);
}
void sl_s_sym_free(sl_s_sym_t* s) {
  free(s->value);
  free(s);
}



// EVALUATION -----------------------------------------------------------------

static inline void* sl_s_expr_messages_eval(sl_s_expr_t* expr, void* scope) {
  sl_s_base_t* m;
  
  // TODO: Make this actually do stuff besides just looking up a method and
  //       calling its primitive.
  
  sl_d_obj_t* target = scope;
  m = expr->head;
  
  sl_d_obj_retain(target);
  while(m != NULL) {
    // DEBUG(
    //   "expr = %p, message = %p, message->type = %d",
    //   expr,
    //   m,
    //   m->type
    // );
    sl_d_obj_t* ret = sl_s_eval(m, scope);
    if(ret != NULL) {
      // Literals
      if(ret->type == SL_DATA_INT) {
        if(target == scope) {
          target = ret;
        } else {
          SENTINEL("Cannot send literal (%d)", ret->type);
        }
      // Messages
      } else if(ret->type == SL_DATA_MESSAGE) {
        ret = sl_d_obj_send(target, (sl_d_message_t*)ret);
        sl_d_obj_release(target);
        target = ret;
      } else {
        SENTINEL("Unexpected return type: %d", ret->type);
      }
      
    } else {
      LOG_ERR("Unexpected NULL return");
    }
    m = m->next;
  }
  return target;
error:
  return NULL;
}

static inline void* sl_s_expr_eval(sl_s_expr_t* expr, void* scope) {
  while(expr != NULL) {
    // DEBUG(
    //   "expr = %p, expr->next = %p, expr->type = %d",
    //   expr,
    //   expr->next,
    //   expr->type
    // );
    if(expr->head == NULL) {
      SENTINEL("Reached empty expression");
    }
    sl_s_expr_messages_eval(expr, scope);
    expr = expr->next;
  }
  return NULL;
error:
  return NULL;
}


static inline sl_d_sym_t* sl_s_sym_eval(sl_s_sym_t* s, void* scope) {
  return sl_i_sym_hint(s);
}

static inline sl_d_block_t* sl_s_block_eval(sl_s_block_t* b, void* scope) {
  sl_d_block_t* block = sl_d_block_new();
  block->expr = b->head;
  block->closure = scope;
  // TODO: Make this parse and use the block header.
  // block->params = NULL;
  return block;
}

// This will parse a message definition starting with the first sl_s_sym
// (the one after the invoking def: symbol). It returns an sl_d_sym* with
// the signature of the method and pushes any parameters it encounters as
// sl_d_sym_t*s onto the given sl_d_array*.
static inline sl_d_sym_t* sl_i_message_signature_params(
  sl_s_sym_t* start,
  sl_d_array_t* params
) {
  if(start->operator) {
    // DEBUG("op = %s", start->value);
    sl_d_sym_t* sig = sl_s_sym_eval(start, NULL);
    sl_s_sym_t* param_1 = start->next;
    sl_s_sym_t* param_2 = param_1->next;
    
    sl_d_sym_t* param_ident = sl_s_sym_eval(param_1, NULL);
    sl_d_array_push(params, (sl_d_obj_t*)param_ident);
    sl_d_sym_t* param_value = sl_s_sym_eval(param_2, NULL);
    sl_d_array_push(params, (sl_d_obj_t*)param_value);
    return sig;
  }
  if(start->assign) {
    // DEBUG("assign = %s", start->value);
    sl_d_sym_t* sig = sl_s_sym_eval(start, NULL);
    
    sl_d_sym_t* param_value = sl_s_sym_eval(start->next, NULL);
    sl_d_array_push(params, (sl_d_obj_t*)param_value);
    return sig;
  }
  
  char sig[80];
  bool is_keyword = true;
  sl_s_sym_t* part = start;
  while(part != NULL) {
    if(is_keyword) {
      strcpy(sig, part->value);
    } else {
      sl_d_sym_t* param = sl_s_sym_eval(part, NULL);
      sl_d_array_push(params, (sl_d_obj_t*)param);
    }
    part = part->next;
    is_keyword = !is_keyword;
  }
  
  // DEBUG("sig = '%s'", sig);
  sl_d_sym_t *sym = sl_d_sym_new(sig);//cstring
  return sym;
}

// This evaluations a method defined by an sl_s_message and returns an
// sl_d_message like "def: method".
static inline sl_d_message_t* sl_i_message_eval_def(sl_s_message_t* msg, void* scope){
  // TODO: Hook these into bootstrap
  static sl_d_sym_t* method_to_sym = NULL;
  
  if(msg->hint) { return msg->hint; }
  
  if(method_to_sym == NULL) { method_to_sym = sl_d_sym_new("method:to:"); }
  
  sl_s_sym_t*     def_sym    = msg->head;
  sl_s_message_t* def_params = def_sym->next;
  sl_s_block_t*   def_block  = def_params->next;
  
  sl_d_array_t* params = sl_d_array_new();
  sl_d_sym_t* sig = sl_i_message_signature_params(def_params->head, params);
  sl_d_block_t* block = sl_s_block_eval(def_block, scope);
  
  // TODO: Maybe abstract methods into just simple blocks?
  sl_d_method_t* method = sl_d_method_new();
  method->signature = sig;
  method->params = params;
  method->block = block;
  
  sl_d_array_t* args = sl_d_array_new();
  sl_d_array_push(args, (sl_d_obj_t*)method->signature);
  sl_d_array_push(args, (sl_d_obj_t*)method);
  
  
  sl_d_message_t* d_msg = sl_d_message_new();
  d_msg->signature = method_to_sym;
  d_msg->arguments = args;
  
  msg->hint = d_msg;
  return d_msg;
}

static inline void* sl_s_message_eval(sl_s_message_t* m, void* scope) {
  // TODO: Hook these into bootstrap
  static sl_d_sym_t* def_sym = NULL;
  if(def_sym == NULL) { def_sym = sl_d_sym_new("def:"); }
  static sl_d_sym_t* assign_sym = NULL;
  if(assign_sym == NULL) { assign_sym = sl_d_sym_new("="); }
  
  sl_d_sym_t* sym = sl_s_sym_eval(m->head, scope);
  if(sym == def_sym) {
    return sl_i_message_eval_def(m, scope);
  } else if(sym == assign_sym) {
    SENTINEL("TODO");
  } else {
    SENTINEL("TODO");
  }
  return SL_D_NULL;
  
error:
  return SL_D_NULL;
}

static inline void* sl_s_def_eval(sl_s_def_t* s, void* scope) {
  DEBUG("here");
  return NULL;
}

static inline void* sl_s_int_eval(sl_s_int_t* s, void* scope) {
  if(s->hint) { return s->hint; }
  
  sl_d_int_t* i = sl_d_int_new();
  i->value = s->value;
  s->hint = i;
  return i;
}

void* sl_s_eval(void* _s, void* scope) {
  // Cast it into the base struct
  sl_s_base_t* s = _s;
  
  sl_syntax_type type = s->type;
  // DEBUG("node = %p, node->type: %d", _s, (int)type);
  
  switch(type) {
  case SL_SYNTAX_EXPR:
    return sl_s_expr_eval((sl_s_expr_t*)s, scope);
  // case SL_SYNTAX_SYM:
  //   return sl_s_sym_eval((sl_s_sym_t*)s, scope);
  // case SL_SYNTAX_DEF:
  //   return sl_s_def_eval((sl_s_def_t*)s, scope);
  case SL_SYNTAX_INT:
    return sl_s_int_eval((sl_s_int_t*)s, scope);
  case SL_SYNTAX_MESSAGE:
    return sl_s_message_eval((sl_s_message_t*)s, scope);
  default:
    SENTINEL("Unknown expression type: %d", (int)type);
  }
  return NULL;
error:
  return NULL;
}
