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

// #define aliases SL_D_NULL and D_NULL to sl_d_null.
// Throw in "extern sl_d_null_t* sl_d_null;" into any source files that need
// this.
sl_d_obj_t* sl_d_null = NULL;

// OBJECTS --------------------------------------------------------------------

sl_d_obj_t* sl_d_obj_new() {
  sl_d_obj_t* obj;
  obj = sl_d_gen_obj_new(SL_DATA_OBJ, sizeof(sl_d_obj_t));
  return obj;
}

// Generic object creation with given type and size.
// Sets type, id, parent, refcount, methods, and values attributes.
void* sl_d_gen_obj_new(sl_data_type type, size_t size) {
  sl_d_obj_t* o = malloc(size);
  assert(o != NULL);
  o->type     = type;
  o->id       = sl_obj_next_id();
  o->parent   = NULL;
  o->refcount = 1;
  o->slots    = NULL;
  return o;
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

// OBJECT MEMORY MANAGEMENT ---------------------------------------------------

// Predefining before retain-release
static inline void sl_d_obj_free(sl_d_obj_t* obj);

void sl_d_obj_retain(sl_d_obj_t* obj) {
  obj->refcount += 1;
}
bool sl_d_obj_release(sl_d_obj_t* obj) {
  obj->refcount -= 1;
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
        sl_d_obj_free((sl_d_obj_t*)current_item->value);
      }
      free(current_item);
    }
  }
  
  // Then call the internal free to finish the rest of the job.
  switch(obj->id) {
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
  default:
    LOG_ERR("Unexpected data type: %d", obj->id);
  }
}

// MESSAGE SENDING ------------------------------------------------------------

sl_d_obj_t* sl_d_exception_new(int count, char** strings) {
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
  
  sl_d_obj_t* e = sl_d_obj_new();
  e->type = SL_DATA_EXCEPTION;
  // Set message string on slot
  sl_d_string_t* s = sl_d_string_new(buff);
  sl_d_obj_set_slot(e, sl_d_sym_new("message"), (sl_d_obj_t*)s);
  
  DEBUG("exception: %s", buff);
  
  return e;
}

sl_d_obj_t* sl_d_obj_send(sl_d_obj_t* target, sl_d_message_t* msg) {
  sl_d_method_t* method = (sl_d_method_t*)sl_d_obj_get_slot(target, msg->signature);
  if(method == NULL || method->type != SL_DATA_METHOD) {
    sl_d_sym_t* sig = msg->signature;
    char* msgs[2] = {"Method not found: ", sig->value};
    sl_d_obj_t* e = sl_d_exception_new(2, msgs);
    return e;
  }
  DEBUG("method = %p", method);
  
  return (sl_d_obj_t*)SL_D_NULL;
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

// STRING ---------------------------------------------------------------------

sl_d_string_t* sl_d_string_new(char* value) {
  sl_d_string_t* s = sl_d_gen_obj_new(SL_DATA_STRING, sizeof(sl_d_string_t));
  s->value = strdup(value);
  return s;
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

void sl_d_obj_set_slot(sl_d_obj_t* obj, sl_d_sym_t* name, sl_d_obj_t* val) {
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
    sl_d_obj_free(old_val);
  }
  // Retain the value now that it's stored in this object.
  sl_d_obj_retain(val);
}
sl_d_obj_t* sl_d_obj_get_slot(sl_d_obj_t* obj, sl_d_sym_t* name) {
  sl_i_sym_item_t* val;
  sl_sym_id id = name->sym_id;
  HASH_FIND_INT(obj->slots, &id, val);
  if(val == NULL) {
    return NULL;
  } else {
    return val->value;
  }
}

// BLOCK ----------------------------------------------------------------------

sl_d_block_t* sl_d_block_new() {
  sl_d_block_t* b = sl_d_gen_obj_new(SL_DATA_BLOCK, sizeof(sl_d_block_t));
  b->expr = NULL;
  b->closure = NULL;
  b->params = NULL;
  return b;
}


// ARRAY ----------------------------------------------------------------------

UT_icd sl_i_array_icd = {sizeof(sl_d_obj_t*), NULL, NULL, NULL};

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
  sl_d_obj_retain(obj);
  utarray_push_back(arr->objs, &obj);
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
  // LOG_WARN("This function should not normally be called!");
  assert(pthread_rwlock_wrlock(&sl_i_sym_table_lock) == 0);
  HASH_DEL(sl_i_sym_table, sym);
  pthread_rwlock_unlock(&sl_i_sym_table_lock);
  free(sym->value);
  free(sym);
}

// INTEGER --------------------------------------------------------------------

sl_d_int_t* sl_d_int_new() {
  sl_d_int_t* i;
  i = sl_d_gen_obj_new(SL_DATA_INT, sizeof(sl_d_int_t));
  return i;
}
void sl_d_int_free(sl_d_int_t* i) {
  free(i);
}

