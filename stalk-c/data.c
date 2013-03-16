#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#include "debug.h"

#include "stalk.h"
#include "syntax.h"
#include "data.h"
#include "symbol.h"

static sl_obj_id sl_i_obj_id_counter = 0;
static pthread_mutex_t sl_i_obj_id_mutex = PTHREAD_MUTEX_INITIALIZER;

static sl_d_sym_t* sl_i_sym_table = NULL;
static pthread_rwlock_t sl_i_sym_table_lock = PTHREAD_RWLOCK_INITIALIZER;

// data.h #defines aliases SL_D_NULL and D_NULL to sl_d_null.
// Throw in "extern sl_d_null_t* sl_d_null;" into any source files that need
// this (or just do "SL_DATA_EXTERN;").
// 
// Global immutable null object.
sl_d_obj_t* sl_d_null = NULL;
// Global immutable root objects.
sl_d_obj_t* sl_i_root_object = NULL;
sl_d_obj_t* sl_i_root_string = NULL;
sl_d_obj_t* sl_i_root_int    = NULL;
sl_d_obj_t* sl_i_root_float  = NULL;

// OBJECTS --------------------------------------------------------------------

sl_d_obj_t* sl_d_obj_new() {
  sl_d_obj_t* obj;
  obj = sl_d_gen_obj_new(SL_DATA_OBJ, sizeof(sl_d_obj_t));
  obj->parent = sl_i_root_object;
  return obj;
}

// Generic object creation with given type and size.
// Sets type, id, parent, refcount, methods, and values attributes.
void* sl_d_gen_obj_new(sl_data_type type, size_t size) {
  sl_d_obj_t* obj = malloc(size);
  assert(obj != NULL);
  obj->type     = type;
  obj->id       = sl_obj_next_id();
  obj->parent   = sl_i_root_object;
  obj->refcount = 1;
  obj->slots    = NULL;
  SL_GC_DEBUG("NEW     %-4d %5d %s", obj->refcount, obj->id, sl_d_type_string_padded(obj->type));
  return obj;
}

// Thread-safe object id generation.
sl_obj_id sl_obj_next_id() {
  sl_obj_id id;
  assert(pthread_mutex_lock(&sl_i_obj_id_mutex) == 0);
  id = sl_i_obj_id_counter;
  sl_i_obj_id_counter += 1;
  assert(pthread_mutex_unlock(&sl_i_obj_id_mutex) == 0);
  return id;
}


char* sl_d_type_string_padded(int type) {
  switch(type) {
    case SL_DATA_OBJ:
      return "OBJ      ";
    case SL_DATA_SYM:
      return "SYM      ";
    case SL_DATA_SCOPE:
      return "SCOPE    ";
    case SL_DATA_ARRAY:
      return "ARRAY    ";
    case SL_DATA_METHOD:
      return "METHOD   ";
    case SL_DATA_INT:
      return "INT      ";
    case SL_DATA_MESSAGE:
      return "MESSAGE  ";
    case SL_DATA_BLOCK:
      return "BLOCK    ";
    case SL_DATA_NULL:
      return "NULL     ";
    case SL_DATA_EXCEPTION:
      return "EXCEPTION";
    case SL_DATA_STRING:
      return "STRING   ";
    case SL_DATA_RETURN:
      return "RETURN   ";
    default:
      LOG_ERR("Shouldn't reach here");
      return "         ";
  }
}

// OBJECT MEMORY MANAGEMENT ---------------------------------------------------

// Predefining before retain-release
static inline void sl_d_obj_free(sl_d_obj_t* obj);

void sl_d_obj_retain(sl_d_obj_t* obj) {
  obj->refcount += 1;
  SL_GC_DEBUG("RETAIN  %-4d %5d %s", obj->refcount, obj->id, sl_d_type_string_padded(obj->type));
}
bool sl_d_obj_release(sl_d_obj_t* obj) {
  obj->refcount -= 1;
  SL_GC_DEBUG("RELEASE %-4d %5d %s", obj->refcount, obj->id, sl_d_type_string_padded(obj->type));
  if(obj->refcount != 0) {
    return false;
  }
  sl_d_obj_free(obj);
  return true;
}

// Free that doesn't give a shit about the reference count.
static inline void sl_d_obj_free(sl_d_obj_t* obj) {
  // Clear out slots.
  if(obj->slots != NULL) {
    sl_i_sym_item_t* current_item;
    sl_i_sym_item_t* tmp_item;
    // sl_d_obj_t* current_obj;
    HASH_ITER(hh, obj->slots, current_item, tmp_item) {
      HASH_DEL(obj->slots, current_item);
      if(current_item->value != NULL) {
        sl_d_obj_release((sl_d_obj_t*)current_item->value);
      }
      free(current_item);
    }
  }
  
  // Then call the internal free to finish the rest of the job.
  switch(obj->type) {
  case SL_DATA_ARRAY:
    sl_d_array_free((sl_d_array_t*)obj);
    break;
  case SL_DATA_SYM:
    sl_d_sym_free((sl_d_sym_t*)obj);
    break;
  case SL_DATA_INT:
    sl_d_int_free((sl_d_int_t*)obj);
    break;
  case SL_DATA_OBJ:
    free(obj);
    break;
  case SL_DATA_STRING:
    sl_d_string_free((sl_d_string_t*)obj);
    break;
  case SL_DATA_METHOD:
    sl_d_method_free((sl_d_method_t*)obj);
    break;
  case SL_DATA_SCOPE:
    sl_d_scope_free((sl_d_scope_t*)obj);
    break;
  default:
    LOG_ERR("Unexpected data type: %d", obj->type);
  }
}

// OBJECT SLOTS ---------------------------------------------------------------

sl_d_obj_t* sl_d_obj_get(sl_d_obj_t* obj, sl_d_sym_t* slot) {
  sl_d_obj_t* value = sl_d_obj_get_slot(obj, slot);
  if(value == NULL) {
    if(obj->parent == NULL) {
      return SL_D_NULL;
    } else {
      value = sl_d_obj_get(obj->parent, slot);
    }
  }
  if(value == NULL) { return SL_D_NULL; }
  
  // DEBUG("got slot %s", slot->value);
  
  return value;
}
void sl_d_obj_set_slot(sl_d_obj_t* obj, sl_d_sym_t* name, sl_d_obj_t* val) {
  // DEBUG("set slot on %p: %s = %p", obj, name->value, val);
  sl_i_sym_item_t* check_item;
  sl_sym_id id = name->sym_id;
  HASH_FIND_INT(obj->slots, &id, check_item);
  if(check_item == NULL) {
    sl_i_sym_item_t* item = malloc(sizeof(sl_i_sym_item_t));
    item->id = id;
    item->value = val;
    HASH_ADD_INT(obj->slots, id, item);
  } else {
    sl_d_obj_t* old_val = check_item->value;
    check_item->value = val;
    sl_d_obj_release(old_val);
  }
  // Retain the value now that it's stored in this object.
  sl_d_obj_retain(val);
}
sl_d_obj_t* sl_d_obj_get_slot(sl_d_obj_t* obj, sl_d_sym_t* name) {
  // DEBUG("get slot on %p: %s", obj, name->value);
  sl_i_sym_item_t* val;
  sl_sym_id id = name->sym_id;
  HASH_FIND_INT(obj->slots, &id, val);
  if(val == NULL) {
    return NULL;
  } else {
    return val->value;
  }
}

// EXCEPTIONS -----------------------------------------------------------------

sl_d_exception_t* sl_d_exception_new(int count, char** strings) {
  static sl_d_sym_t* message_sym = NULL;
  if(message_sym == NULL) { message_sym = sl_d_sym_new("message"); }
  
  // Calculate the lengths
  int lengths[count];
  int total_length = 0;
  int i = 0;
  for(i = 0; i < count; i++) {
    int length = strlen(strings[i]);
    lengths[i] = length;
    total_length += length;
  }
  total_length += 1;
  // Concatenate the strings
  char buff[total_length];
  buff[0] = '\0';
  for(i = 0; i < count; i++) {
    strncat(buff, strings[i], lengths[i]);
  }
  // Add trailing newline and null byte
  // buff[total_length - 2] = '\0';
  buff[total_length - 1] = '\0';
  
  sl_d_exception_t* e = sl_d_gen_obj_new(
    SL_DATA_EXCEPTION, sizeof(sl_d_exception_t)
  );
  e->traceback = sl_d_traceback_new();
  // Set message string on slot
  sl_d_string_t* s = sl_d_string_new(buff);
  sl_d_obj_set_slot(
    (sl_d_obj_t*)e, message_sym, (sl_d_obj_t*)s
  );
  
  //LOG_ERR("Exception: %s", buff);
  
  return e;
}

sl_i_traceback_t* sl_d_traceback_new() {
  sl_i_traceback_t* t = malloc(sizeof(sl_i_traceback_t));
  t->head = NULL;
  t->tail = NULL;
  return t;
}
sl_i_traceback_frame_t* sl_d_traceback_frame_new() {
  sl_i_traceback_frame_t* tf = malloc(sizeof(sl_i_traceback_frame_t));
  tf->prev = NULL;
  tf->next = NULL;
  tf->signature = NULL;
  tf->source = "<none>";
  tf->line = 0;
  return tf;
}
void sl_d_traceback_push_frame(
  sl_i_traceback_t* t, sl_i_traceback_frame_t* tf
) {
  if(t->head == NULL) { t->head = tf; }
  sl_i_traceback_frame_t* prev = t->tail;
  if(prev != NULL) {
    prev->next = tf;
    tf->prev   = prev;
  }
  t->tail = tf;
}

void sl_i_traceback_print(sl_i_traceback_t* t) {
  fprintf(stderr, "  Traceback:\n");
  sl_i_traceback_frame_t* tf = t->head;
  while(tf != NULL) {
    fprintf(
      stderr, "    %s at %s:%d\n",
      tf->signature->value, tf->source, tf->line
    );
    tf = tf->next;
  }
  
}

void sl_i_exception_print(sl_d_exception_t* e) {
  static sl_d_sym_t* message_sym = NULL;
  if(message_sym == NULL) { message_sym = sl_d_sym_new("message"); }
  
  fprintf(stderr, "Exception:\n");
  sl_d_string_t* message = (sl_d_string_t*)sl_d_obj_get_slot(
    (sl_d_obj_t*)e, message_sym
  );
  if(message != NULL) {
    fprintf(stderr, "  Message: %s\n", message->value);
  } else {
    fprintf(stderr, "  No message\n");
  }
  if(e->traceback != NULL) {
    sl_i_traceback_print(e->traceback);
  }
}

// MESSAGE SENDING ------------------------------------------------------------

sl_d_obj_t* sl_d_obj_send(sl_d_obj_t* target, sl_d_message_t* msg) {
  sl_d_sym_t* sig;
  sl_d_obj_t* slot = sl_d_obj_get(target, msg->signature);
  
  if(slot == SL_D_NULL) {
    goto not_found;
  }
  if(slot->type == SL_DATA_METHOD) {
    sl_d_method_t* method = (sl_d_method_t*)slot;
    if(method->hint != NULL) {
      return method->hint(target, msg->arguments);
    } else {
      return sl_d_method_call(method, target, msg->arguments);
    }
  } else {
    // Just a regular value from a slot
    return slot;
  }
  
  return (sl_d_obj_t*)SL_D_NULL;
  
not_found:
  sig = msg->signature;
  char* msgs[2] = {"Slot not found: ", sig->value};
  sl_d_exception_t* e = sl_d_exception_new(2, msgs);
  /*sl_i_traceback_frame_t* tf = sl_d_traceback_frame_new();
  tf->signature = sig;
  sl_d_traceback_push_frame(e->traceback, tf);*/
  return (sl_d_obj_t*)e;
}

// RETURN ---------------------------------------------------------------------

sl_i_return_t* sl_i_return_new(sl_d_obj_t* value) {
  sl_i_return_t* r;
  r = sl_d_gen_obj_new(SL_DATA_RETURN, sizeof(sl_i_return_t));
  r->value = value;
  return r;
}
sl_d_obj_t* sl_i_return_unwrap(sl_d_obj_t* ret) {
  if(ret->type == SL_DATA_RETURN) {
    return ((sl_i_return_t*)ret)->value;
  }
  return ret;
}

// METHOD ---------------------------------------------------------------------

sl_d_method_t* sl_d_method_new() {
  sl_d_method_t* m;
  m = sl_d_gen_obj_new(SL_DATA_METHOD, sizeof(sl_d_method_t));
  m->signature = NULL;
  m->params = NULL;
  m->block = NULL;
  m->hint = NULL;
  return m;
}

sl_d_obj_t* sl_d_method_call(
  sl_d_method_t* method,
  sl_d_obj_t* self,
  sl_d_array_t* args
) {
  // TODO: Hook these into bootstrap
  static sl_d_sym_t* self_sym = NULL;
  if(self_sym == NULL) { self_sym = sl_d_sym_new("self"); }
  
  static sl_d_array_t* block_params = NULL;
  if(block_params == NULL) { block_params = sl_d_array_new(); }
  
  sl_d_scope_t* scope = sl_d_scope_new();
  
  sl_i_array_item_t* param_item = sl_d_array_first_item(method->params);
  sl_i_array_item_t* arg_item   = sl_d_array_first_item(args);
  int i = 1;
  while(param_item != NULL) {
    if(arg_item == NULL) {
      char buff[4];
      sprintf(buff, "%d", i);
      char* msgs[4] = {
        "Missing argument ", buff,
        " for method ", method->signature->value
      };
      return (sl_d_obj_t*)sl_d_exception_new(4, msgs);
    }
    sl_d_obj_set_slot(
      (sl_d_obj_t*)scope,
      (sl_d_sym_t*)param_item->value,
      arg_item->value
    );
    param_item = sl_d_array_next_item(method->params, param_item);
    arg_item   = sl_d_array_next_item(args, arg_item);
    i += 1;
  }
  sl_d_obj_set_slot((sl_d_obj_t*)scope, self_sym, self);
  // Call the block itself
  sl_d_obj_t* ret = sl_d_block_call(method->block, scope, block_params);
  // Clean up
  sl_d_obj_release((sl_d_obj_t*)scope);
  // sl_d_obj_release((sl_d_obj_t*)args);
  return ret;
}
void sl_d_method_free(sl_d_method_t* m) {
  if(m->params != NULL) {
    sl_d_obj_free((sl_d_obj_t*)m->params);
  }
  DEBUG("TODO BLOCK FREE");
  free(m);
}

// MESSAGE --------------------------------------------------------------------

sl_d_message_t* sl_d_message_new() {
  sl_d_message_t* m;
  m = sl_d_gen_obj_new(SL_DATA_MESSAGE, sizeof(sl_d_message_t));
  m->signature = NULL;
  m->arguments = NULL;
  return m;
}
void sl_d_message_empty(sl_d_message_t* m) {
  if(m->arguments != NULL) {
    sl_d_obj_release((sl_d_obj_t*)m->arguments);
  }
}
void sl_d_message_free(sl_d_message_t* m) {
  sl_d_message_empty(m);
  free(m);
}

// STRING ---------------------------------------------------------------------

sl_d_string_t* sl_d_string_new(char* value) {
  sl_d_string_t* s = sl_d_gen_obj_new(SL_DATA_STRING, sizeof(sl_d_string_t));
  s->parent = sl_i_root_string;
  s->value = strdup(value);
  return s;
}
void sl_d_string_free(sl_d_string_t* s) {
  free(s->value);
  free(s);
}

// SCOPE ----------------------------------------------------------------------

sl_d_scope_t* sl_d_scope_new() {
  sl_d_scope_t* s;
  s = sl_d_gen_obj_new(SL_DATA_SCOPE, sizeof(sl_d_scope_t));
  return s;
}
void sl_d_scope_free(sl_d_scope_t* s) {
  free(s);
}

// BLOCK ----------------------------------------------------------------------

sl_d_block_t* sl_d_block_new() {
  sl_d_block_t* b = sl_d_gen_obj_new(SL_DATA_BLOCK, sizeof(sl_d_block_t));
  b->expr = NULL;
  b->closure = NULL;
  b->params = NULL;
  return b;
}
sl_d_obj_t* sl_d_block_call(
  sl_d_block_t* block,
  sl_d_scope_t* scope,
  sl_d_array_t* params
) {
  static sl_d_sym_t* closure_sym = NULL;
  if(closure_sym == NULL) { closure_sym = sl_d_sym_new("closure"); }
  
  scope->parent = block->closure;
  sl_d_obj_set_slot((sl_d_obj_t*)scope, closure_sym, (sl_d_obj_t*)block->closure);
  return sl_s_expr_eval(block->expr, scope);
}

// ARRAY ----------------------------------------------------------------------

// UT_icd sl_i_array_icd = {sizeof(sl_d_obj_t*), NULL, NULL, NULL};
UT_icd sl_i_array_icd = {sizeof(sl_i_array_item_t*), NULL, NULL, NULL};

sl_d_array_t* sl_d_array_new() {
  sl_d_array_t* arr;
  arr = sl_d_gen_obj_new(SL_DATA_ARRAY, sizeof(sl_d_array_t));
  utarray_new(arr->objs, &sl_i_array_icd);
  return arr;
}
void sl_d_array_free(sl_d_array_t* arr) {
  sl_d_obj_t* obj;
  for(
    obj = (sl_d_obj_t*)utarray_front(arr->objs);
    obj != NULL;
    obj = (sl_d_obj_t*)utarray_next(arr->objs, obj)
  ) {
    sl_d_obj_release(obj);
  }
  utarray_free(arr->objs);
  free(arr);
}
void sl_d_array_push(sl_d_array_t* arr, sl_d_obj_t* obj) {
  sl_i_array_item_t* item = malloc(sizeof(sl_i_array_item_t));
  item->value = obj;
  sl_d_obj_retain(obj);
  utarray_push_back(arr->objs, item);
}
sl_d_obj_t* sl_d_array_index(sl_d_array_t* arr, int i) {
  sl_i_array_item_t* item = sl_d_array_index_item(arr, i);
  if(item == NULL) {
    char buff[12];
    sprintf(buff, "%d", i);
    char* msgs[2] = {"No item at index ", buff};
    return (sl_d_obj_t*)sl_d_exception_new(2, msgs);
  } else {
    return item->value;
  }
}
sl_i_array_item_t* sl_d_array_index_item(sl_d_array_t* arr, int i) {
  int len = utarray_len(arr->objs);
  if(i >= len || i < 0) {
    return NULL;
  }
  return (sl_i_array_item_t*)utarray_eltptr(arr->objs, i);
}
sl_i_array_item_t* sl_d_array_first_item(sl_d_array_t* arr) {
  return (sl_i_array_item_t*)utarray_front(arr->objs);
}
sl_i_array_item_t* sl_d_array_next_item(sl_d_array_t* arr, sl_i_array_item_t* i) {
  return (sl_i_array_item_t*)utarray_next(arr->objs, i);
}
sl_d_obj_t* sl_d_array_index_set(sl_d_array_t* arr, int i, sl_d_obj_t* obj) {
  sl_i_array_item_t* item = sl_d_array_index_item(arr, i);
  if(item == NULL) {
    char buff[12];
    sprintf(buff, "%d", i);
    char* msgs[2] = {"No item at index ", buff};
    return (sl_d_obj_t*)sl_d_exception_new(2, msgs);
  } else {
    sl_d_obj_t* old_val = item->value;
    item->value = obj;
    sl_d_obj_release(old_val);
    sl_d_obj_retain(obj);
    return obj;
  }
}

// SYMBOL ---------------------------------------------------------------------

sl_d_sym_t* sl_d_sym_new(char* name) {
  sl_d_sym_t* s;
  int len = strlen(name);
  sl_sym_id sym_id = sl_i_str_to_sym_id(name, len);
  assert(pthread_rwlock_rdlock(&sl_i_sym_table_lock) == 0);
    HASH_FIND_INT(sl_i_sym_table, &sym_id, s);
  pthread_rwlock_unlock(&sl_i_sym_table_lock);
  if(s != NULL) {
    // DEBUG("sym table hit: %s", name);
    return s;
  }
  // DEBUG("sym table miss: %s", name);
  
  s = (sl_d_sym_t*)sl_d_gen_obj_new(SL_DATA_SYM, sizeof(sl_d_sym_t));
  // value
  // s->value = malloc(sizeof(char) * (len + 1));
  // strncpy(s->value, name, len + 1); // dest, src
  s->value = strdup(name);
  // sym_id
  s->sym_id = sym_id;
  // hh
  assert(pthread_rwlock_wrlock(&sl_i_sym_table_lock) == 0);
    sl_d_sym_t* s_check;
    HASH_FIND_INT(sl_i_sym_table, &sym_id, s_check);
    if(s_check != NULL) {
      // DEBUG("sym table dup: %s", name);
      // sl_d_sym_free without thread safety, since we already have write-lock.
      free(s->value);
      free(s);
      pthread_rwlock_unlock(&sl_i_sym_table_lock);
      return s_check;
    } else {
      HASH_ADD_INT(sl_i_sym_table, sym_id, s);
      pthread_rwlock_unlock(&sl_i_sym_table_lock);
      return s;
    }
  //pthread
  LOG_ERR("Should never reach here");
  return s;
}
void sl_d_sym_free(sl_d_sym_t* sym) {
  LOG_WARN("This function should not normally be called!");
  assert(pthread_rwlock_wrlock(&sl_i_sym_table_lock) == 0);
  HASH_DEL(sl_i_sym_table, sym);
  pthread_rwlock_unlock(&sl_i_sym_table_lock);
  free(sym->value);
  free(sym);
}

// INTEGER --------------------------------------------------------------------

sl_d_int_t* sl_d_int_new(int value) {
  sl_d_int_t* i;
  i = sl_d_gen_obj_new(SL_DATA_INT, sizeof(sl_d_int_t));
  i->parent = sl_i_root_int;
  i->value = value;
  return i;
}
void sl_d_int_free(sl_d_int_t* i) {
  free(i);
}

// FLOAT ----------------------------------------------------------------------

sl_d_float_t* sl_d_float_new(float value) {
  sl_d_float_t* i;
  i = sl_d_gen_obj_new(SL_DATA_INT, sizeof(sl_d_float_t));
  i->parent = sl_i_root_float;
  i->value = value;
  return i;
}
void sl_d_float_free(sl_d_float_t* i) {
  free(i);
}

