#include <stdio.h>

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

// OBJECTS --------------------------------------------------------------------

static sl_d_string_t* type_str_object;
static sl_d_sym_t*    type_sym;
SL_I_METHOD_F(object_type_primitive) {//args: self, params
  return (sl_d_obj_t*)type_str_object;
}

static sl_d_sym_t* object_sym;
static sl_d_sym_t* new_sym;
SL_I_METHOD_F(object_new) {
  return sl_d_obj_new();
}

static sl_d_sym_t* return_sym;
SL_I_METHOD_F(object_return) {//args: self, params
  sl_d_obj_t* obj = (sl_d_obj_t*)sl_d_array_index(params, 0);
  if(obj == NULL) {
    char* msgs[1] = {"Missing argument 1 to return:"};
    return (sl_d_obj_t*)sl_d_exception_new(1, msgs);
  }
  // DEBUG("ret obj val  = %d", ((sl_d_int_t*)obj)->value);
  return (sl_d_obj_t*)sl_i_return_new(obj);
}

static sl_d_sym_t* slot_set_sym;
SL_I_METHOD_F(object_slot_set) {//args: self, params
  // arg 1: symbol of slot to set
  sl_d_sym_t* sym = (sl_d_sym_t*)sl_d_array_index(params, 0);
  if(sym == NULL || sym->type != SL_DATA_SYM) {
    char buff[4];
    sprintf(buff, "%d", sym->type);
    char* msgs[2] = {"Argument 1 must be a symbol; currently = ", buff};
    return (sl_d_obj_t*)sl_d_exception_new(2, msgs);
  }
  // arg 2: object to set to slot
  sl_d_obj_t* obj = (sl_d_obj_t*)sl_d_array_index(params, 1);
  if(obj == NULL) {
    char* msgs[1] = {"Missing argument 2"};
    return (sl_d_obj_t*)sl_d_exception_new(1, msgs);
  }
  // DEBUG("setting slot %s = %p", sym->value, obj);
  sl_d_obj_set_slot(self, sym, (sl_d_obj_t*)obj);
  return (sl_d_obj_t*)obj;
}

static sl_d_sym_t* methodto_sym;
SL_I_METHOD_F(object_methodto) {//args: self, params
  sl_d_sym_t* sym = (sl_d_sym_t*)sl_d_array_index(params, 0);
  if(sym == NULL || sym->type != SL_DATA_SYM) {
    char buff[4];
    sprintf(buff, "%d", sym->type);
    char* msgs[2] = {"Argument 1 must be a symbol; currently = ", buff};
    return (sl_d_obj_t*)sl_d_exception_new(2, msgs);
  }
  sl_d_method_t* meth = (sl_d_method_t*)sl_d_array_index(params, 1);
  if(meth == NULL || meth->type != SL_DATA_METHOD) {
    char* msgs[1] = {"Argument 2 must be a method"};
    return (sl_d_obj_t*)sl_d_exception_new(1, msgs);
  }
  // DEBUG("set slot %p %p (%s) %p", self, sym, sym->value, meth);
  sl_d_obj_set_slot(self, sym, (sl_d_obj_t*)meth);
  return (sl_d_obj_t*)SL_D_NULL;
}


// STRINGS --------------------------------------------------------------------

static sl_d_sym_t* println_sym;
SL_I_METHOD_F(string_println) {//args: self, params
  sl_d_string_t* str = (sl_d_string_t*)self;
  printf("%s\n", str->value);
  return (sl_d_obj_t*)SL_D_NULL;
}

static sl_d_sym_t* add_sym;
SL_I_METHOD_F(string_add) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  if(other == NULL) {
    char* msgs[1] = {"Missing argument 1"};
    return (sl_d_obj_t*)sl_d_exception_new(1, msgs);
  }
  if(other->type != SL_DATA_STRING) {
    char* msgs[1] = {"Argument 1 must be a string"};
    return (sl_d_obj_t*)sl_d_exception_new(1, msgs);
  }
  sl_d_string_t* _self  = (sl_d_string_t*)self;
  sl_d_string_t* _other = (sl_d_string_t*)other;
  char* new_string;
  unsigned int self_len = strlen(_self->value);
  unsigned int other_len = strlen(_other->value);
  // Initialize new string
  new_string = malloc(sizeof(char) * (self_len + other_len + 1));
  // Copy the strings and add the trailing null byte
  strncpy(new_string, _self->value, self_len);
  strncpy(&new_string[self_len], _other->value, other_len);
  new_string[self_len + other_len] = '\0';
  // Construct return object
  sl_d_obj_t* ret = (sl_d_obj_t*)sl_d_string_new(new_string);
  free(new_string);// sl_d_string_new calls strdup()
  return ret;
}

// INTEGERS -------------------------------------------------------------------

static sl_d_sym_t* string_sym;
SL_I_METHOD_F(int_string) {//args: self, params
  sl_d_int_t* _self = (sl_d_int_t*)self;
  char buff[20];
  sprintf(buff, "%d", _self->value);
  // Construct return object
  sl_d_obj_t* ret = (sl_d_obj_t*)sl_d_string_new(buff);
  return ret;
}

static inline sl_d_obj_t* guard_int(sl_d_obj_t* val) {
  if(val == NULL) {
    char* msgs[1] = {"Missing argument 1"};
    return (sl_d_obj_t*)sl_d_exception_new(1, msgs);
  }
  if(val->type != SL_DATA_INT) {
    char* msgs[1] = {"Argument 1 must be a string"};
    return (sl_d_obj_t*)sl_d_exception_new(1, msgs);
  }
  return NULL;
}

static sl_d_sym_t* mod_sym;
SL_I_METHOD_F(int_mod) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  return (sl_d_obj_t*)sl_d_int_new(_self->value % _other->value);
}

static sl_d_sym_t* add_sym;
SL_I_METHOD_F(int_add) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  return (sl_d_obj_t*)sl_d_int_new(_self->value + _other->value);
}

static sl_d_sym_t* mul_sym;
SL_I_METHOD_F(int_mul) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  return (sl_d_obj_t*)sl_d_int_new(_self->value * _other->value);
}

static sl_d_sym_t* div_sym;
SL_I_METHOD_F(int_div) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  return (sl_d_obj_t*)sl_d_int_new(_self->value / _other->value);
}

void sl_i_bootstrap() {
  // Set the statics.
  type_sym = sl_d_sym_new("type");
  println_sym = sl_d_sym_new("println");
  methodto_sym = sl_d_sym_new("method:to:");
  return_sym = sl_d_sym_new("return:");
  slot_set_sym = sl_d_sym_new("=");
  add_sym = sl_d_sym_new("+");
  string_sym = sl_d_sym_new("string");
  mod_sym = sl_d_sym_new("%");
  mul_sym = sl_d_sym_new("*");
  div_sym = sl_d_sym_new("/");
  object_sym = sl_d_sym_new("object");
  new_sym = sl_d_sym_new("new");
  
  
  // ROOT ---------------------------------------------------------------------
  
  // root object
  sl_d_obj_t* root = sl_d_obj_new();
  sl_i_root_object = root;
  // Just to make sure it isn't circular.
  root->parent = NULL;
  
  // `new` -> object
  sl_d_method_t* new_method = sl_d_method_new();
  new_method->hint = *object_new;
  new_method->signature = new_sym;
  sl_d_obj_set_slot(root, new_sym, (sl_d_obj_t*)new_method);
  
  // `object type` -> string
  sl_d_method_t* type_method = sl_d_method_new();
  type_method->hint = *object_type_primitive;
  type_method->signature = type_sym;
  sl_d_obj_set_slot(root, type_sym, (sl_d_obj_t*)type_method);
  
  // `object = sym val` -> val
  sl_d_method_t* slot_set_method = sl_d_method_new();
  slot_set_method->hint = *object_slot_set;
  slot_set_method->signature = slot_set_sym;
  sl_d_obj_set_slot(root, slot_set_sym, (sl_d_obj_t*)slot_set_method);
  
  // `object method: sym to: method` -> string
  sl_d_method_t* methodto_method = sl_d_method_new();
  methodto_method->hint = *object_methodto;
  methodto_method->signature = methodto_sym;
  sl_d_obj_set_slot(root, methodto_sym, (sl_d_obj_t*)methodto_method);
  
  // `object return: object` -> internal return object
  sl_d_method_t* return_method = sl_d_method_new();
  return_method->hint = *object_return;
  return_method->signature = return_sym;
  sl_d_obj_set_slot(root, return_sym, (sl_d_obj_t*)return_method);
  
  // STRING -------------------------------------------------------------------
  
  // root string
  sl_d_obj_t* string = sl_d_obj_new();
  assert(string->parent == sl_i_root_object);
  sl_i_root_string = string;
  
  // `println` -> null
  sl_d_method_t* println_method = sl_d_method_new();
  println_method->hint = *string_add;
  println_method->signature = add_sym;
  sl_d_obj_set_slot(string, add_sym, (sl_d_obj_t*)println_method);
  
  // `+ string` -> new string
  sl_d_method_t* string_add_method = sl_d_method_new();
  string_add_method->hint = *string_println;
  string_add_method->signature = println_sym;
  sl_d_obj_set_slot(string, println_sym, (sl_d_obj_t*)string_add_method);
  
  // INTEGER ------------------------------------------------------------------
  
  // root int
  sl_d_obj_t* root_int = sl_d_obj_new();
  assert(root_int->parent == sl_i_root_object);
  sl_i_root_int = root_int;
  
  // `string` -> new string
  sl_d_method_t* int_string_method = sl_d_method_new();
  int_string_method->hint = *int_string;
  int_string_method->signature = string_sym;
  sl_d_obj_set_slot(root_int, string_sym, (sl_d_obj_t*)int_string_method);
  
  // `%` -> int
  sl_d_method_t* int_mod_method = sl_d_method_new();
  int_mod_method->hint = *int_mod;
  int_mod_method->signature = mod_sym;
  sl_d_obj_set_slot(root_int, mod_sym, (sl_d_obj_t*)int_mod_method);
  
  // `+` -> int
  sl_d_method_t* int_add_method = sl_d_method_new();
  int_add_method->hint = *int_add;
  int_add_method->signature = add_sym;
  sl_d_obj_set_slot(root_int, add_sym, (sl_d_obj_t*)int_add_method);
  
  // `*` -> int
  sl_d_method_t* int_mul_method = sl_d_method_new();
  int_mul_method->hint = *int_mul;
  int_mul_method->signature = mul_sym;
  sl_d_obj_set_slot(root_int, mul_sym, (sl_d_obj_t*)int_mul_method);
  
  // `/` -> int
  sl_d_method_t* int_div_method = sl_d_method_new();
  int_div_method->hint = *int_div;
  int_div_method->signature = div_sym;
  sl_d_obj_set_slot(root_int, div_sym, (sl_d_obj_t*)int_div_method);
  
  // MORE STATICS (Dependent on sl_i_root_object, sl_i_root_string, etc.) -----
  
  type_str_object = sl_d_string_new("object");
  // And updating those already defined.
  type_sym->parent = sl_i_root_object;
  println_sym->parent = sl_i_root_object;
  methodto_sym->parent = sl_i_root_object;
  
  SL_GC_DEBUG("BOOTSTRAP DONE");
}
