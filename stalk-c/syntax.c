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
  s->keyword = false;
  s->hint = NULL;
  return s;
}
void sl_s_message_unshift(sl_s_message_t* message, sl_s_base_t* s) {
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
  s->message_hint = NULL;
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

sl_s_keyword_t* sl_s_keyword_new() {
  sl_s_keyword_t* s = sl_s_base_gen_new(SL_SYNTAX_KEYWORD, sizeof(sl_s_keyword_t));
  s->value = NULL;//cstring
  s->sym = NULL;//sl_d_sym_t
  return s;
}

// EVALUATION -----------------------------------------------------------------

static int __depth = 0;

static inline void* sl_s_message_eval(sl_s_message_t* m, void* scope);
static inline void* sl_s_messages_eval(
  sl_d_obj_t* target,
  sl_s_message_t* messages,
  void* scope
) {
  __depth += 1;
  fprintf(stderr, "%*s sl_s_messages_eval\n", __depth, "");
  
  sl_d_obj_t* prev_target;
  sl_s_message_t* message = messages;
  sl_d_obj_t* ret = NULL;
  sl_d_obj_retain(target);
  while(message != NULL) {
    ret = sl_s_message_eval(message, scope);
    if(ret->type == SL_DATA_EXCEPTION) {
      __depth -= 1;
      // Pass the exception back up the chain.
      return ret;
    }
    if(ret->type != SL_DATA_MESSAGE) {
      // Plain object without a target besides the scope.
      if(target == scope) {
        target = ret;
      } else {
        SENTINEL("Unexpected non-message (type = %d)", ret->type);
      }
      
    } else {
      // If it is a message:
      prev_target = target;// Save the previous target to be released.
      // Send the message.
      target = sl_d_obj_send(target, (sl_d_message_t*)ret);
      // If we got a RETURN back then pass it up.
      if(target->type == SL_DATA_RETURN) {
        sl_d_obj_release(prev_target);
        __depth -= 1;
        return target;
      }
      // Release the previous target.
      sl_d_obj_release(prev_target);
    }
    // Move on to the next message.
    message = message->next;
  }
  __depth -= 1;
  return target;
error:
  __depth -= 1;
  return NULL;
}

void* sl_s_expr_eval(sl_s_expr_t* expr, void* scope) {
  __depth += 1;
  fprintf(stderr, "%*s sl_s_expr_eval\n", __depth, "");
  
  sl_d_obj_t* ret = NULL;
  
  sl_d_scope_t* target = (sl_d_scope_t*)scope;
  
  while(expr != NULL) {
    // DEBUG("%d expr eval", __depth);
    
    if(target != NULL) {
      // DEBUG("expr target %p type = %d", target, target->type);
    } else {
      // DEBUG("expr target null");
    }
    ret = sl_s_messages_eval((sl_d_obj_t*)target, (sl_s_message_t*)expr->head, scope);
    // DEBUG("ret type = %d", ret->type);
    if(ret != NULL && ret->type == SL_DATA_RETURN) {
      sl_i_return_t* _ret = (sl_i_return_t*)ret;
      __depth -= 1;
      return _ret->value;
    }
    if(ret != NULL && expr->next == NULL) {
      __depth -= 1;
      return ret;
    }
    
    expr = expr->next;
  }
  __depth -= 1;
  return ret;
}

static inline void* sl_s_sym_eval(sl_s_sym_t* s, void* scope) {
  sl_d_sym_t* sym = sl_i_sym_hint(s);
  if(s->literal) {
    return sym;
  } else {
    if(s->message_hint) {
      sl_d_message_t* msg = s->message_hint;
      return sl_d_obj_send(scope, msg);
    }
    
    // Make an identifier-message if it's not a literal.
    // Empty arguments
    sl_d_array_t* args = sl_d_array_new();
    // Set up the message
    sl_d_message_t* msg = sl_d_message_new();
    msg->signature = sym;
    msg->arguments = args;
    
    s->message_hint = msg;
    
    return sl_d_obj_send(scope, msg);
  }
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
    // Operator
    sl_d_sym_t* sig = sl_i_sym_hint(start);
    sl_s_sym_t* param_1 = start->next;
    sl_s_sym_t* param_2 = param_1->next;
    
    sl_d_sym_t* param_ident = sl_i_sym_hint(param_1);
    sl_d_array_push(params, (sl_d_obj_t*)param_ident);
    sl_d_sym_t* param_value = sl_i_sym_hint(param_2);
    sl_d_array_push(params, (sl_d_obj_t*)param_value);
    return sig;
  } else
  if(start->assign) {
    // Assignment
    sl_d_sym_t* sig = sl_i_sym_hint(start);
    
    sl_d_sym_t* param_value = sl_i_sym_hint(start->next);
    sl_d_array_push(params, (sl_d_obj_t*)param_value);
    return sig;
  } else
  if(start->keyword) {
    // Keyword
    char sig[80];
    bool is_keyword = true;
    sl_s_sym_t* part = start;
    while(part != NULL) {
      if(is_keyword) {
        strcpy(sig, part->value);
      } else {
        sl_d_sym_t* param = sl_i_sym_hint(part);
        sl_d_array_push(params, (sl_d_obj_t*)param);
      }
      part = part->next;
      is_keyword = !is_keyword;
    }
  
    // DEBUG("sig = '%s'", sig);
    sl_d_sym_t *sym = sl_d_sym_new(sig);//cstring
    return sym;
  } else {
    // Plain (just an identifier)
    sl_d_sym_t* sig = sl_i_sym_hint(start);
    return sig;
  }
  
}

// Compile a linked-list of sl_s_sym_t's into an sl_d_sym_t.
static inline sl_d_sym_t* sl_i_message_signature_compile(sl_s_sym_t* sym) {
  char sig[80];
  int len = 0;
  int sym_len;
  while(sym != NULL) {
    sym_len = strlen(sym->value);
    strncpy(&sig[len], sym->value, sym_len);
    len += sym_len;
    sym = sym->next;
  }
  sig[len] = '\0';
  
  sl_d_sym_t* sym_d = sl_d_sym_new(sig);
  return sym_d;
}

// Extract a def message from the tree.
static inline void* sl_i_message_def_extract(sl_s_message_t* def_msg) {
  sl_s_message_t* next = def_msg->next;
  // Extract it from the tree.
  def_msg->next = next->next;
  
  sl_s_message_t* def_next = def_msg->next;
  if(def_next != NULL) {
    def_next->prev = def_msg;
  }
  
  next->next = NULL;
  next->prev = NULL;
  return next;
}

// Extract a sl_d_sym_t/exception.
static inline sl_d_obj_t* sl_i_message_param_extract(sl_s_base_t* param) {
  __depth += 1;
  
  fprintf(stderr, "%*s sl_i_message_param_extract type: %d\n", __depth, "", param->type);
  
  if(param->type == 1) {
    sl_s_sym_t* ps = (sl_s_sym_t*)param;
    // fprintf(stderr, "%*s sl_i_message_param_extract value: %s\n", __depth, "", ps->value);
    __depth -= 1;
    return (sl_d_obj_t*)sl_i_sym_hint(ps);
  } else {
    __depth -= 1;
    char buff[4];
    sprintf(buff, "%d", param->type);
    char* msgs[2] = {"Unexpected syntax type in message param: ", buff};
    return sl_d_exception_new(2, msgs);
    
  }
  
  
  __depth -= 1;
  return (sl_d_obj_t*)SL_D_NULL;
}

// This evaluations a method defined by an sl_s_message and returns an
// sl_d_message like "def: method".
static inline sl_d_obj_t* sl_i_message_eval_def(sl_s_message_t* msg, void* scope){
  // TODO: Hook these into bootstrap
  static sl_d_sym_t* method_to_sym = NULL;
  
  if(msg->hint) { return msg->hint; }
  
  if(method_to_sym == NULL) { method_to_sym = sl_d_sym_new("method:to:"); }
  
  __depth += 1;
  
  sl_s_message_t* def_msg = msg;
  // sl_s_sym_t*     def_sym = (sl_s_sym_t*)msg->head;
  
  fprintf(stderr, "%*s sl_s_messages_eval_def\n", __depth, "");
  
  sl_d_array_t* params_d = sl_d_array_new();
  
  sl_s_message_t* next = sl_i_message_def_extract(def_msg);
  if(next == NULL) {
    char* msgs[1] = {"Missing message in definition"};
    return sl_d_exception_new(1, msgs);
  }
  
  sl_s_sym_t* current_sig;
  sl_s_sym_t* prev_sig = NULL;
  sl_s_sym_t* sig = NULL;
  
  while(next) {
    if(next->head->type == SL_SYNTAX_SYM) {
      // The head of the message should be a syntax symbol.
      current_sig = (sl_s_sym_t*)next->head;
      current_sig->prev = prev_sig;
      // If there was a previous signature parsed then set the current one to
      // follow the previous.
      if(prev_sig != NULL) {
        prev_sig->next = current_sig;
      // Otherwise it's the first signature being parsed.
      } else {
        sig = current_sig;
        prev_sig = current_sig;
      }
      // Extract the param symbol from the current sig.
      sl_d_obj_t* param = sl_i_message_param_extract(current_sig->next);
      // Then remove it from the current sig.
      current_sig->next = NULL;
      // Ran into an error processing the signature.
      if(param->type == SL_DATA_EXCEPTION) {
        return param;
      }
      // Push the param symbol on to the params array.
      sl_d_array_push(params_d, param);
      if(current_sig->operator) {
        goto parse_block;
      } else if(current_sig->assign) {
        char* msgs[1] = {"Not implemented"};
        __depth -= 1;
        return sl_d_exception_new(1, msgs);
      } else {
        // Go to next message.
      }
    } else if(next->head->type == SL_SYNTAX_BLOCK) {
      goto parse_block;
    } else {
      char buff[4];
      sprintf(buff, "%d", next->head->type);
      char* msgs[3] = {"Unexpected syntax type ", buff, " in definition"};
      __depth -= 1;
      return sl_d_exception_new(3, msgs);
    }
    
    next = sl_i_message_def_extract(def_msg);
  }
  if(next == NULL) {
    char* msgs[1] = {"Missing block in definition"};
    __depth -= 1;
    return sl_d_exception_new(1, msgs);
  }
  
  
parse_block:
  if(sig == NULL) {
    char* msgs[1] = {"Missing message signature in definition"};
    __depth -= 1;
    return sl_d_exception_new(1, msgs);
  }
  // fprintf(stderr, "%*s next->head->type: %d\n", __depth + 1, "", next->head->type);
  sl_d_block_t* block_d = (sl_d_block_t*)sl_s_block_eval((sl_s_block_t*)next->head, scope);
  
  // fprintf(stderr, "%*s sym: %s\n", __depth + 1, "", sig->value);
  
  // Compile the linked-list of signatures into a single symbol.
  sl_d_sym_t* sig_d = sl_i_message_signature_compile(sig);
  
  
  sl_d_method_t* method = sl_d_method_new();
  method->signature = sig_d;
  method->params = params_d;
  method->block = block_d;
  
  sl_d_array_t* args = sl_d_array_new();
  sl_d_array_push(args, (sl_d_obj_t*)method->signature);
  sl_d_array_push(args, (sl_d_obj_t*)method);
  
  sl_d_message_t* d_msg = sl_d_message_new();
  d_msg->signature = method_to_sym;
  d_msg->arguments = args;
  
  msg->hint = d_msg;
  
  __depth -= 1;
  return (sl_d_obj_t*)d_msg;
}

static inline void* sl_i_message_eval_keyword(sl_s_message_t* m, void* scope) {
  sl_d_array_t* args = sl_d_array_new();
  
  __depth += 1;
  fprintf(stderr, "%*s sl_i_message_eval_keyword\n", __depth, "");
  
  if(m->hint != NULL && m->hint_args != NULL) {
    sl_d_message_t* msg = (sl_d_message_t*)m->hint;
    int i = 0;
    sl_s_base_t* arg = (sl_s_base_t*)m->hint_args;
    while(arg != NULL) {
      sl_d_array_index_set(msg->arguments, i, sl_s_eval(arg, scope));
      arg = arg->next;
      i  += 1;
    }
    __depth -= 1;
    return msg;
  }
  
  // Computing the signature
  char sig[80];
  int len = 0;
  int sym_len;
  // Building up the linked list of arguments to be cached.
  sl_s_base_t*    arg_head = NULL;
  sl_s_base_t*    arg_curr;
  sl_s_base_t*    arg_prev = NULL;
  // Current message
  sl_s_message_t* cm = m;
  // While the current message isn't null and it's a keyword message:
  while(cm != NULL && cm->keyword == true) {
    sl_s_sym_t* kw = (sl_s_sym_t*)cm->head;
    sl_s_base_t* arg = kw->next;
    // Push the keyword onto the signature.
    sym_len = strlen(kw->value);
    strncpy(&sig[len], kw->value, sym_len);
    len += sym_len;
    
    sl_d_obj_t* arg_value = sl_s_eval(arg, scope);
    // Push the value onto the array of arguments
    sl_d_array_push(args, arg_value);
    // Then cache the pointer to the arg sl_s_base_t
    arg_curr = arg;
    arg_curr->prev = arg_prev;
    if(arg_prev != NULL) {
      arg_prev->next = arg_curr;
    } else {
      arg_head = arg;
      arg_prev = arg_curr;
    }
    // Move on to the next message.
    cm = cm->next;
  }
  // Convert the char* sig into a symbol.
  sig[len] = '\0';
  sl_d_sym_t *sym = sl_d_sym_new(sig);//cstring
  // Create the message.
  sl_d_message_t* msg = sl_d_message_new();
  msg->signature = sym;
  msg->arguments = args;
  
  // Make the next of the main message be the next matched non-keyword message
  // so that the sl_s_messages_eval() will skip over this message's children.
  m->next = cm;
  
  // Set the hints to speed things up in the future.
  m->hint = msg;
  m->hint_args = arg_head;
  
  __depth -= 1;
  return msg;
}

static inline void* sl_i_message_eval_plain(sl_s_message_t* m, void* scope) {
  fprintf(stderr, "%*s sl_i_message_eval_plain\n", __depth + 1, "");
  
  __depth += 1;
  
  sl_s_sym_t* head = (sl_s_sym_t*)m->head;
  if(head->keyword) {
    void* ret = sl_i_message_eval_keyword(m, scope);
    __depth -= 1;
    return ret;
  } else {
    if(m->hint != NULL) { return m->hint; }
    
    // Just a plain identifier-message
    // Empty arguments
    sl_d_array_t* args = sl_d_array_new();
    // Set up the message
    sl_d_message_t* msg = sl_d_message_new();
    msg->signature = sl_i_sym_hint(head);
    msg->arguments = args;
    
    m->hint = msg;
    
    __depth -= 1;
    return msg;
  }
}

static inline void* sl_i_message_eval_assign(sl_s_message_t* m, void* scope) {
  fprintf(stderr, "%*s sl_i_message_eval_assign\n", __depth + 1, "");
  
  sl_s_sym_t*  assign_s = (sl_s_sym_t*)m->head;
  DEBUG("assign type = %d", assign_s->type);
  DEBUG("assign next = %p", assign_s->next);
  sl_s_sym_t*  slot_s   = (sl_s_sym_t*)assign_s->next;
  DEBUG("slot type   = %d", slot_s->type);
  sl_s_expr_t* val_s    = (sl_s_expr_t*)slot_s->next;
  
  sl_d_obj_t*  slot_val   = sl_s_eval(val_s, scope);
  
  if(m->hint != NULL) {
    // Used the cached message object and just update the second argument.
    sl_d_message_t* msg = m->hint;
    sl_d_obj_t* ret = sl_d_array_index_set(msg->arguments, 1, slot_val);
    if(ret->type == SL_DATA_EXCEPTION) {
      return ret;
    }
    return m->hint;
  }
  
  sl_d_sym_t*  assign_sym = sl_i_sym_hint(assign_s);
  sl_d_sym_t*  slot_sym   = sl_i_sym_hint(slot_s);
  
  sl_d_message_t* msg = sl_d_message_new();
  msg->signature = assign_sym;
  sl_d_array_t* args = sl_d_array_new();
  sl_d_array_push(args, (sl_d_obj_t*)slot_sym);
  sl_d_array_push(args, slot_val);
  msg->arguments = args;
  
  m->hint = msg;
  return msg;
}

static inline void* sl_i_message_eval_operator(sl_s_message_t* m, void* scope) {
  sl_s_sym_t*  operator_s = (sl_s_sym_t*)m->head;
  sl_s_base_t* val_s      = (sl_s_base_t*)operator_s->next;
  
  sl_d_obj_t*  val        = sl_s_eval(val_s, scope);
  
  fprintf(stderr, "%*s sl_i_message_eval_operator val->type: %d\n", __depth + 1, "", val->type);
  
  if(m->hint != NULL) {
    // Used the cached message object and just update the second argument.
    sl_d_message_t* msg = m->hint;
    sl_d_obj_t* ret = sl_d_array_index_set(msg->arguments, 0, val);
    if(ret->type == SL_DATA_EXCEPTION) {
      return ret;
    }
    return m->hint;
  }
  
  sl_d_sym_t*  operator_sym = sl_i_sym_hint(operator_s);
  
  sl_d_message_t* msg = sl_d_message_new();
  msg->signature = operator_sym;
  sl_d_array_t* args = sl_d_array_new();
  sl_d_array_push(args, val);
  msg->arguments = args;
  
  m->hint = msg;
  return msg;
}

static inline void* sl_s_message_eval(sl_s_message_t* m, void* scope) {
  // TODO: Hook these into bootstrap
  static sl_d_sym_t* def_sym = NULL;
  if(def_sym == NULL) { def_sym = sl_d_sym_new("def:"); }
  static sl_d_sym_t* assign_sym = NULL;
  if(assign_sym == NULL) { assign_sym = sl_d_sym_new("="); }
  
  if(m->head == NULL) {
    SENTINEL("Message must have a head");
  }
  sl_s_sym_t* head = (sl_s_sym_t*)m->head;
  if(head->type == SL_SYNTAX_SYM) {
    // Begins with a symbol
    sl_d_sym_t* sym = sl_i_sym_hint(head);
    if(sym == def_sym) {
      return sl_i_message_eval_def(m, scope);
    } else if(head->assign) {
      return sl_i_message_eval_assign(m, scope);
    } else if(head->operator) {
      return sl_i_message_eval_operator(m, scope);
    } else {
      return sl_i_message_eval_plain(m, scope);
    }
  } else if(head->type == SL_SYNTAX_EXPR) {
    return sl_s_expr_eval((sl_s_expr_t*)head, scope);
  } else if(head->type == SL_SYNTAX_STRING) {
    return sl_s_string_eval((sl_s_string_t*)head, scope);
  } else {
    DEBUG("Unrecognized message head type %d", head->type);
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
  sl_d_int_t* i = sl_d_int_new(s->value);
  s->hint = i;
  return i;
}
static inline void* sl_s_float_eval(sl_s_float_t* s, void* scope) {
  if(s->hint) { return s->hint; }
  sl_d_float_t* f = sl_d_float_new(s->value);
  s->hint = f;
  return f;
}

inline void* sl_s_string_eval(sl_s_string_t* s, void* scope) {
  if(s->hint) { return s->hint; }
  
  sl_d_string_t* str = sl_d_string_new(s->value);
  s->hint = str;
  
  return str;
}

void* sl_s_eval(void* _s, void* scope) {
  fprintf(stderr, "%*s sl_s_eval\n", __depth, "");
  
  // Cast it into the base struct
  sl_s_base_t* s = _s;
  
  sl_syntax_type type = s->type;
  // DEBUG("node = %p, node->type: %d", _s, (int)type);
  
  switch(type) {
  case SL_SYNTAX_EXPR:
    return sl_s_expr_eval((sl_s_expr_t*)s, scope);
  case SL_SYNTAX_STRING:
    return sl_s_string_eval((sl_s_string_t*)s, scope);
  case SL_SYNTAX_SYM:
    return sl_s_sym_eval((sl_s_sym_t*)s, scope);
  //   // DEBUG("there");
  //   _msg = sl_d_message_new();
  //   _msg->signature = sl_s_sym_eval((sl_s_sym_t*)s, scope);
  //   return _msg;
  //   return sl_s_sym_eval((sl_s_sym_t*)s, scope);
  // case SL_SYNTAX_DEF:
  //   return sl_s_def_eval((sl_s_def_t*)s, scope);
  case SL_SYNTAX_INT:
    return sl_s_int_eval((sl_s_int_t*)s, scope);
  case SL_SYNTAX_FLOAT:
    return sl_s_float_eval((sl_s_float_t*)s, scope);
  // case SL_SYNTAX_MESSAGE:
  //   return sl_s_message_eval((sl_s_message_t*)s, scope);
  default:
    SENTINEL("Unknown expression type: %d", (int)type);
  }
  return NULL;
error:
  return NULL;
}
