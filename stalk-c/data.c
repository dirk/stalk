#include <pthread.h>
#include <stdio.h>
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

// Thread-safe object id generation.
sl_obj_id sl_obj_next_id() {
  sl_obj_id id;
  assert(pthread_mutex_lock(&sl_i_obj_id_mutex) == 0);
  id = sl_i_obj_id_counter;
  sl_i_obj_id_counter += 1;
  assert(pthread_mutex_unlock(&sl_i_obj_id_mutex) == 0);
  return id;
}

// Generic object creation with given type and size.
// Sets type, id, parent, refcount, methods, and values attributes.
void* sl_d_gen_obj_new(void* _o, sl_data_type type, size_t size) {
  sl_d_obj_t* o = malloc(size);
  assert(o != NULL);
  o->type = type;
  o->id = sl_obj_next_id();
  o->parent = NULL;
  o->refcount = 1;
  o->methods = NULL;
  o->values = NULL;
  return o;
}

sl_d_obj_t* sl_d_obj_new() {
  sl_d_obj_t* obj;
  obj = sl_d_gen_obj_new(obj, SL_DATA_OBJ, sizeof(sl_d_obj_t));
  return obj;
}

// Free that doesn't give a shit about the reference count.
static inline void sl_d_obj_force_free(sl_d_obj_t* obj) {
  // Do any specific frees
  switch(obj->id) {
  case SL_DATA_ARRAY:
    sl_d_array_free((sl_d_array_t*)obj);
    break;
  case SL_DATA_SYM:
    sl_d_sym_free((sl_d_sym_t*)obj);
    break;
  default:
    LOG_ERR("Unexpected data type: %d", obj->id);
  }
  
  free(obj);
}

bool sl_d_obj_free(sl_d_obj_t* obj) {
  obj->refcount -= 1;
  if(obj->refcount != 0) {
    return false;
  }
  
  // Clear out values and locals (and call sl_d_obj_free on each).
  if(obj->values != NULL) {
    sl_i_sym_item_t* current_item;
    sl_i_sym_item_t* tmp_item;
    // sl_d_obj_t* current_obj;
    HASH_ITER(hh, obj->values, current_item, tmp_item) {
      HASH_DEL(obj->values, current_item);
      if(current_item->value != NULL) {
        sl_d_obj_free((sl_d_obj_t*)current_item->value);
      }
      free(current_item);
    }
  }
  if(obj->methods != NULL) {
    sl_i_sym_item_t* current_item;
    sl_i_sym_item_t* tmp_item;
    // sl_d_obj_t* current_obj;
    HASH_ITER(hh, obj->methods, current_item, tmp_item) {
      HASH_DEL(obj->methods, current_item);
      if(current_item->value != NULL) {
        sl_d_obj_free((sl_d_obj_t*)current_item->value);
      }
      free(current_item);
    }
  }
  
  
  // Then call the internal free to finish the rest of the job.
  sl_d_obj_force_free(obj);
  
  return true;
}

// METHOD ---------------------------------------------------------------------

sl_d_method_t* sl_d_method_new() {
  sl_d_method_t* m;
  m = sl_d_gen_obj_new(m, SL_DATA_METHOD, sizeof(sl_d_method_t));
  m->signature = NULL;
  m->closure = NULL;
  m->block = NULL;
  m->hint = NULL;
  return m;
}

// SCOPE ----------------------------------------------------------------------

sl_d_scope_t* sl_d_scope_new() {
  sl_d_scope_t* s;
  s = sl_d_gen_obj_new(s, SL_DATA_SCOPE, sizeof(sl_d_scope_t));
  return s;
}
void sl_d_scope_free(sl_d_scope_t* s) {
  free(s);
}

// TODO: De-duplicate this
void sl_d_obj_set_value(sl_d_obj_t* obj, sl_d_sym_t* name, sl_d_obj_t* val) {
  sl_i_sym_item_t* check_item;
  sl_sym_id id = name->sym_id;
  HASH_FIND_INT(obj->values, &id, check_item);
  if(check_item == NULL) {
    sl_i_sym_item_t* item = malloc(sizeof(sl_i_sym_item_t));
    item->id = id;
    item->value = val;
    HASH_ADD_INT(obj->values, id, item);
  } else {
    sl_d_obj_t* old_val = check_item->value;
    check_item->value = val;
    sl_d_obj_free(old_val);
  }
}
void sl_d_obj_set_method(sl_d_obj_t* obj, sl_d_sym_t* name, sl_d_obj_t* val) {
  sl_i_sym_item_t* check_item;
  sl_sym_id id = name->sym_id;
  HASH_FIND_INT(obj->methods, &id, check_item);
  if(check_item == NULL) {
    sl_i_sym_item_t* item = malloc(sizeof(sl_i_sym_item_t));
    item->id = id;
    item->value = val;
    HASH_ADD_INT(obj->methods, id, item);
  } else {
    sl_d_obj_t* old_val = check_item->value;
    check_item->value = val;
    sl_d_obj_free(old_val);
  }
}
sl_d_obj_t* sl_d_obj_get_value(sl_d_obj_t* obj, sl_d_sym_t* name) {
  sl_i_sym_item_t* val;
  sl_sym_id id = name->sym_id;
  HASH_FIND_INT(obj->values, &id, val);
  if(val == NULL) {
    return NULL;
  } else {
    return val->value;
  }
}
sl_d_obj_t* sl_d_obj_get_method(sl_d_obj_t* obj, sl_d_sym_t* name) {
  sl_i_sym_item_t* val;
  sl_sym_id id = name->sym_id;
  HASH_FIND_INT(obj->methods, &id, val);
  if(val == NULL) {
    return NULL;
  } else {
    return val->value;
  }
}


// ARRAY ----------------------------------------------------------------------

UT_icd sl_i_array_icd = {sizeof(sl_d_obj_t*), NULL, NULL, NULL};

sl_d_array_t* sl_d_array_new() {
  sl_d_array_t* arr;
  arr = sl_d_gen_obj_new(arr, SL_DATA_ARRAY, sizeof(sl_d_array_t));
  utarray_new(arr->objs, &sl_i_array_icd);
  return arr;
}
void sl_d_array_free(sl_d_array_t* arr) {
  utarray_free(arr->objs);
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
    DEBUG("sym table hit");
    return s;
  }
  DEBUG("sym table miss");
  
  s = (sl_d_sym_t*)sl_d_gen_obj_new(s, SL_DATA_SYM, sizeof(sl_d_sym_t));
  // value and length
  s->length = len;
  s->value = malloc(sizeof(char) * len);
  strncpy(s->value, name, len); // dest, src
  // sym_id
  s->sym_id = sym_id;
  // hh
  assert(pthread_rwlock_wrlock(&sl_i_sym_table_lock) == 0);
    sl_d_sym_t* s_check;
    HASH_FIND_INT(sl_i_sym_table, &sym_id, s_check);
    if(s_check != NULL) {
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
}

