#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

sl_d_sym_t* sl_d_sym_new(char* name) {
  sl_d_sym_t* s;
  sl_sym_id sym_id = sl_i_str_to_sym_id(name);
  assert(pthread_rwlock_rdlock(&sl_i_sym_table_lock) == 0);
  HASH_FIND_INT(sl_i_sym_table, &sym_id, s);
  pthread_rwlock_unlock(&sl_i_sym_table_lock);
  if(s != NULL) {
    printf("sym table hit\n");
    return s;
  }
  printf("sym table miss\n");
  s = malloc(sizeof(sl_d_sym_t));
  assert(s != NULL);
  s->type = SL_DATA_SYM;
  s->id = sl_obj_next_id();
  int len = strlen(name) + 1;
  s->value = malloc(sizeof(char) * len);
  strncpy(s->value, name, len); // dest, src
  s->sym_id = sym_id;
  assert(pthread_rwlock_wrlock(&sl_i_sym_table_lock) == 0);
  HASH_ADD_INT(sl_i_sym_table, sym_id, s);
  pthread_rwlock_unlock(&sl_i_sym_table_lock);
  return s;
}
