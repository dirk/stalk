#include "debug.h"

#include "stalk.h"
#include "syntax.h"
#include "data.h"
#include "symbol.h"
#include "bootstrap.h"

SL_DATA_EXTERN;





void sl_d_bootstrap() {
  sl_d_obj_t* null = sl_d_gen_obj_new(SL_DATA_NULL, sizeof(sl_d_obj_t));
  sl_d_null = null;
}


static sl_d_string_t* type_str_object;
static sl_d_sym_t*    type_sym;
SL_I_METHOD_F(object_type_primitive) {//args: self, params
  return (sl_d_obj_t*)type_str_object;
}

static sl_d_sym_t* println_sym;
SL_I_METHOD_F(string_println) {//args: self, params
  sl_d_string_t* str = (sl_d_string_t*)self;
  printf("%s\n", str->value);
  return (sl_d_obj_t*)SL_D_NULL;
}


void sl_i_bootstrap() {
  // Set the statics.
  type_sym = sl_d_sym_new("type");
  println_sym = sl_d_sym_new("println");
  
  // ROOT ---------------------------------------------------------------------
  
  // root object
  sl_d_obj_t* root = sl_d_obj_new();
  sl_i_root_object = root;
  // Just to make sure it isn't circular.
  root->parent = NULL;
  // `object type` -> string
  sl_d_method_t* type_method = sl_d_method_new();
  type_method->hint = *object_type_primitive;
  type_method->signature = type_sym;
  sl_d_obj_set_slot(root, type_sym, (sl_d_obj_t*)type_method);
  
  // STRING -------------------------------------------------------------------
  
  // root string
  sl_d_obj_t* string = sl_d_obj_new();
  assert(string->parent == sl_i_root_object);
  sl_i_root_string = string;
  // `string println` -> null
  sl_d_method_t* println_method = sl_d_method_new();
  println_method->hint = *string_println;
  println_method->signature = println_sym;
  sl_d_obj_set_slot(string, println_sym, (sl_d_obj_t*)println_method);
  
  // MORE STATICS (Dependent on sl_i_root_object, sl_i_root_string, etc.) -----
  type_str_object = sl_d_string_new("object");
  // And updating those already defined.
  type_sym->parent = sl_i_root_object;
  println_sym->parent = sl_i_root_object;
}
