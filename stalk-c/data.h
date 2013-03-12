#ifndef DATA_H
#define DATA_H

#include "deps/uthash/src/uthash.h"
#include "deps/uthash/src/utarray.h"

// VALUES ---------------------------------------------------------------------

// FNV hash of symbol string
typedef unsigned int sl_sym_id;
// Incremented for each object created
typedef unsigned int sl_obj_id;

typedef unsigned char sl_data_type;
#define SL_DATA_OBJ    (sl_data_type)0
#define SL_DATA_SYM    (sl_data_type)1
#define SL_DATA_SCOPE  (sl_data_type)2
#define SL_DATA_ARRAY  (sl_data_type)3
#define SL_DATA_METHOD (sl_data_type)4
#define SL_DATA_INT    (sl_data_type)5

#define SL_DATA_TYPE    sl_data_type type;
#define SL_OBJ_ID       sl_obj_id id;
#define SL_OBJ_PARENT   void* parent;
#define SL_OBJ_REFCOUNT unsigned int refcount;
#define SL_OBJ_METHODS  sl_i_sym_item_t* values;
#define SL_OBJ_VALUES   sl_i_sym_item_t* methods;
#define SL_OBJ_HEADER   SL_DATA_TYPE \
                        SL_OBJ_ID \
                        SL_OBJ_PARENT \
                        SL_OBJ_REFCOUNT \
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
  SL_OBJ_HEADER;
} sl_d_obj_t;

typedef struct sl_d_int {
  SL_OBJ_HEADER;
  int value;
} sl_d_int_t;

typedef struct sl_d_sym {
  SL_OBJ_HEADER;
  sl_sym_id sym_id;
  char *value;//non-null-terminated string of length .length
  unsigned char length;//length of value
  UT_hash_handle hh;
} sl_d_sym_t;

typedef struct sl_d_block {
  SL_OBJ_HEADER;
  sl_s_expr_t* expr;
  sl_s_list_t* params;
} sl_d_block_t;

typedef struct sl_d_array {
  SL_OBJ_HEADER;
  UT_array* objs;
} sl_d_array_t;

/*typedef struct sl_i_scope_item {
  sl_scope_item_id id;// Computed using sl_d_scope_hash_name(name)
  char *name;
  UT_hash_handle hh;
} sl_i_scope_item_t;*/

typedef struct sl_d_scope {
  SL_OBJ_HEADER;
  // Locals of a scope are defined in .values and .methods.
  // .parent: Parent scope
  // 
  // sl_i_scope_item_t *locals;
  // sl_i_sym_item_t* locals;
} sl_d_scope_t;

#define SL_I_METHOD_F(NAME) sl_d_obj_t* NAME( \
  sl_d_obj_t* self, \
  sl_d_array_t* params)

// Primitive function pointer
typedef SL_I_METHOD_F((*sl_i_method_f));
/*typedef sl_d_obj_t* (*sl_i_method_f)(
  sl_d_obj_t* self,
  sl_d_array_t* params
);*/

typedef struct sl_d_method {
  SL_OBJ_HEADER;
  sl_d_sym_t* signature;
  sl_d_scope_t* closure;
  sl_d_block_t* block;
  // Optional function pointer, currently used for just primitives but in the
  // future hopefully also for JITing.
  sl_i_method_f hint;
} sl_d_method_t;




// FUNCTIONS ------------------------------------------------------------------

sl_d_obj_t* sl_d_obj_new();
bool sl_d_obj_free(sl_d_obj_t* obj);
void sl_d_obj_set_value(sl_d_obj_t* obj, sl_d_sym_t* name, sl_d_obj_t* val);
void sl_d_obj_set_method(sl_d_obj_t* obj, sl_d_sym_t* name, sl_d_obj_t* val);
sl_d_obj_t* sl_d_obj_get_value(sl_d_obj_t* obj, sl_d_sym_t* name);
sl_d_obj_t* sl_d_obj_get_method(sl_d_obj_t* obj, sl_d_sym_t* name);

sl_d_sym_t* sl_d_sym_new(char* name);
void sl_d_sym_free(sl_d_sym_t* sym);

sl_d_method_t* sl_d_method_new();

sl_d_scope_t* sl_d_scope_new();
void sl_d_scope_free(sl_d_scope_t* s);

sl_d_array_t* sl_d_array_new();
void sl_d_array_free(sl_d_array_t* arr);

sl_d_int_t* sl_d_int_new();
void sl_d_int_free(sl_d_int_t* arr);


#endif
