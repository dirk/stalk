#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "debug.h"

#import "stalk.h"
#import "data.h"
#import "symbol.h"

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

// New generic object with given type and size.
// Sets type, id, parent, methods, and values attributes.
void* sl_d_gen_obj_new(void* _o, sl_data_type type, size_t size) {
  sl_d_obj_t* o = malloc(size);
  assert(o != NULL);
  o->type = type;
  o->id = sl_obj_next_id();
  o->parent = NULL;
  o->methods = NULL;
  o->values = NULL;
  return o;
}

sl_d_sym_t* sl_d_sym_new(char* name) {
  sl_d_sym_t* s;
  sl_sym_id sym_id = sl_i_str_to_sym_id(name);
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
  int len = strlen(name);
  s->length = len;
  s->value = malloc(sizeof(char) * len);
  strncpy(s->value, name, len); // dest, src
  // sym_id
  s->sym_id = sym_id;
  // hh
  assert(pthread_rwlock_wrlock(&sl_i_sym_table_lock) == 0);
  HASH_ADD_INT(sl_i_sym_table, sym_id, s);
  pthread_rwlock_unlock(&sl_i_sym_table_lock);
  
  return s;
}
char* sl_i_sym_value_to_cstring(sl_d_sym_t* s) {
  LOG_WARN("Dangerous function");
  char* buffer = malloc(sizeof(char) * (s->length + 1));
  sprintf(buffer, "%*s", s->length, s->value);
  return buffer;
}
