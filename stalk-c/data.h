#ifndef DATA_H
#define DATA_H

#import "deps/uthash/src/uthash.h"

// FNV hash of symbol string
typedef unsigned int sl_sym_id;
// Incremented for each object created
typedef unsigned int sl_obj_id;

typedef unsigned char sl_data_type;
#define SL_DATA_OBJ   (sl_data_type)0
#define SL_DATA_SYM   (sl_data_type)1
#define SL_DATA_SCOPE (sl_data_type)2

#define SL_DATA_TYPE   sl_data_type type;
#define SL_OBJ_ID      sl_obj_id id;
#define SL_OBJ_PARENT  void* parent;
#define SL_OBJ_METHODS sl_i_sym_item_t* values;
#define SL_OBJ_VALUES  sl_i_sym_item_t* methods;
#define SL_OBJ_DEFN    SL_DATA_TYPE \
                       SL_OBJ_ID \
                       SL_OBJ_PARENT \
                       SL_OBJ_METHODS \
                       SL_OBJ_VALUES

typedef unsigned int sl_scope_item_id;

// Lookup a value in a hash by its symbol id.
typedef struct sl_i_sym_item {
  sl_sym_id id;
  //char *name;
  void* value;
  UT_hash_handle hh;
} sl_i_sym_item_t;

typedef struct sl_d_obj {
  SL_OBJ_DEFN;
} sl_d_obj_t;

typedef struct sl_d_sym {
  SL_OBJ_DEFN;
  sl_sym_id sym_id;
  char *value;//non-null-terminated string of length .length
  unsigned char length;//length of value
  UT_hash_handle hh;
} sl_d_sym_t;



typedef struct sl_i_scope_item {
  sl_scope_item_id id;// Computed using sl_d_scope_hash_name(name)
  char *name;
  UT_hash_handle hh;
} sl_i_scope_item_t;

typedef struct sl_d_scope {
  SL_DATA_TYPE
  SL_OBJ_ID
  sl_d_obj_t *parent;
  // sl_i_scope_item_t *locals;
  sl_i_sym_item_t* locals;
} sl_scope_t;

sl_d_sym_t* sl_d_sym_new(char* name);
char* sl_i_sym_value_to_cstring(sl_d_sym_t* s);

#endif
