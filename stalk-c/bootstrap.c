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

// SYMBOLS --------------------------------------------------------------------

static sl_d_sym_t* length_sym;

// GUARDS ---------------------------------------------------------------------

static inline sl_d_obj_t* guard_block(sl_d_obj_t* val, int index) {
  if(val->type == SL_DATA_EXCEPTION) { return val; }
  if(val == NULL || val == SL_D_NULL) {
    char buff[4];
    sprintf(buff, "%d", index);
    char* msgs[2] = {"Missing argument ", buff};
    return (sl_d_obj_t*)sl_d_exception_new(2, msgs);
  }
  if(val->type != SL_DATA_BLOCK) {
    char buff[4];
    sprintf(buff, "%d", index);
    char* msgs[3] = {"Argument ", buff, " must be a block"};
    return (sl_d_obj_t*)sl_d_exception_new(3, msgs);
  }
  return NULL;
}

static inline sl_d_obj_t* guard_obj(sl_d_obj_t* val, int index) {
  if(val->type == SL_DATA_EXCEPTION) { return val; }
  if(val == NULL) {
    char buff[4];
    sprintf(buff, "%d", index);
    char* msgs[2] = {"Missing argument ", buff};
    return (sl_d_obj_t*)sl_d_exception_new(2, msgs);
  }
  return NULL;
}

static inline sl_d_obj_t* guard_int(sl_d_obj_t* val, int index) {
  if(val->type == SL_DATA_EXCEPTION) { return val; }
  if(val == NULL || val == SL_D_NULL) {
    char buff[4];
    sprintf(buff, "%d", index);
    char* msgs[2] = {"Missing argument ", buff};
    return (sl_d_obj_t*)sl_d_exception_new(2, msgs);
  }
  if(val->type != SL_DATA_INT) {
    char buff[4];
    sprintf(buff, "%d", index);
    char buff2[8];
    sprintf(buff2, "%d", val->type);
    char* msgs[5] = {
      "Argument ", buff, " must be an integer (currently ",
      buff2, ")"
    };
    return (sl_d_obj_t*)sl_d_exception_new(5, msgs);
  }
  return NULL;
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

static sl_d_sym_t* while_do_sym;
SL_I_METHOD_F(object_while_do) {//args: self, params
  // Check condition (arg 1)
  sl_d_obj_t* _cond = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* cond_valid = guard_block(_cond, 1);
  if(cond_valid != NULL) { return cond_valid; }
  // Check block (arg 2)
  sl_d_obj_t* _block = (sl_d_obj_t*)sl_d_array_index(params, 1);
  sl_d_obj_t* block_valid = guard_block(_block, 2);
  if(block_valid != NULL) { return block_valid; }
  
  sl_d_block_t* cond = (sl_d_block_t*)_cond;
  sl_d_block_t* block = (sl_d_block_t*)_block;
  
  sl_d_obj_t* cond_ret = sl_d_block_call_shallow(
    // block, params
    cond, NULL
  );
  sl_d_obj_t* block_ret;
  while(SL_D_TRUTHY(cond_ret)) {
    block_ret = sl_d_block_call_shallow(block, NULL);
    if(block_ret->type == SL_DATA_EXCEPTION) {
      return block_ret;
    }
    cond_ret = sl_d_block_call_shallow(cond, NULL);
  }
  if(cond_ret->type == SL_DATA_EXCEPTION) { return cond_ret; }
  return SL_D_NULL;
}

static sl_d_sym_t* if_then_sym;
SL_I_METHOD_F(object_if_then) {
  sl_d_obj_t* cond = sl_d_array_index(params, 0);
  sl_d_obj_t* cond_valid = guard_obj(cond, 1);
  if(cond_valid != NULL) { return cond_valid; }
  // Check block (arg 2)
  sl_d_obj_t* _block = (sl_d_obj_t*)sl_d_array_index(params, 1);
  sl_d_obj_t* block_valid = guard_block(_block, 2);
  if(block_valid != NULL) { return block_valid; }
  
  sl_d_obj_t* cond_ret;
  if(cond->type == SL_DATA_BLOCK) {
    cond_ret = sl_d_block_call_shallow((sl_d_block_t*)cond, NULL);
  } else {
    cond_ret = cond;
  }
  
  sl_d_block_t* block = (sl_d_block_t*)_block;
  if(SL_D_TRUTHY(cond)) {
    return sl_d_block_call_shallow(block, NULL);
  } else {
    return SL_D_NULL;
  }
}

static sl_d_sym_t* if_then_else_sym;
SL_I_METHOD_F(object_if_then_else) {
  sl_d_obj_t* cond = sl_d_array_index(params, 0);
  sl_d_obj_t* cond_valid = guard_obj(cond, 1);
  if(cond_valid != NULL) { return cond_valid; }
  // Check block (arg 2)
  sl_d_obj_t* _block = (sl_d_obj_t*)sl_d_array_index(params, 1);
  sl_d_obj_t* block_valid = guard_block(_block, 2);
  if(block_valid != NULL) { return block_valid; }
  // Check block (arg e)
  sl_d_obj_t* _block_else = (sl_d_obj_t*)sl_d_array_index(params, 2);
  sl_d_obj_t* block_else_valid = guard_block(_block, 3);
  if(block_else_valid != NULL) { return block_else_valid; }
  
  sl_d_obj_t* cond_ret;
  if(cond->type == SL_DATA_BLOCK) {
    cond_ret = sl_d_block_call_shallow((sl_d_block_t*)cond, NULL);
  } else {
    cond_ret = cond;
  }
  if(cond_ret->type == SL_DATA_EXCEPTION) {
    return cond_ret;
  }
  
  sl_d_block_t* block = (sl_d_block_t*)_block;
  sl_d_block_t* block_else = (sl_d_block_t*)_block_else;
  if(SL_D_TRUTHY(cond)) {
    return sl_d_block_call_shallow(block, NULL);
  } else {
    return sl_d_block_call_shallow(block_else, NULL);
  }
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

static sl_d_sym_t* print_sym;
SL_I_METHOD_F(string_print) {//args: self, params
  sl_d_string_t* str = (sl_d_string_t*)self;
  printf("%s", str->value);
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
  unsigned int self_len = _self->length;
  unsigned int other_len = _other->length;
  // Initialize new string
  new_string = malloc(sizeof(char) * (self_len + other_len + 1));
  // Copy the strings and add the trailing null byte
  strncpy(new_string, _self->value, self_len);
  strncpy(&new_string[self_len], _other->value, other_len);
  new_string[self_len + other_len] = '\0';
  // Construct return object
  sl_d_string_t* ret = sl_d_string_new_empty();
  ret->value = new_string;
  ret->length = (self_len + other_len);
  return (sl_d_obj_t*)ret;
}

SL_I_METHOD_F(string_length) {//args: self, params
  sl_d_string_t* str = (sl_d_string_t*)self;
  return (sl_d_obj_t*)sl_d_int_new(str->length);
}

static sl_d_sym_t* from_to_sym;
SL_I_METHOD_F(string_from_to) {//args: self, params
  sl_d_obj_t* _from = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* from_valid = guard_int(_from, 1);
  if(from_valid != NULL) { return from_valid; }
  sl_d_obj_t* _to = (sl_d_obj_t*)sl_d_array_index(params, 1);
  sl_d_obj_t* to_valid = guard_int(_to, 1);
  if(to_valid != NULL) { return to_valid; }
  
  int from = ((sl_d_int_t*)_from)->value;
  int to   = ((sl_d_int_t*)_to)->value;
  sl_d_string_t* str = (sl_d_string_t*)self;
  
  if(from <= str->length) {
    if(to < from) {
      char* msgs[1] = {"Argument 2 must be greater than argument 1"};
      return (sl_d_obj_t*)sl_d_exception_new(1, msgs);
    }
    int len = (to - from);
    char* buff = malloc(sizeof(char) * (len + 1));
    strncpy(buff, &str->value[from], len);
    buff[len] = '\0';
    sl_d_string_t* ret = sl_d_string_new(buff);
    free(buff);
    // sl_d_obj_release(_from);
    // sl_d_obj_release(_to);
    return (sl_d_obj_t*)ret;
  } else {
    return SL_D_NULL;
  }
  return (sl_d_obj_t*)sl_d_int_new(str->length);
}

static sl_d_sym_t* from_sym;
SL_I_METHOD_F(string_from) {//args: self, params
  sl_d_obj_t* _from = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* from_valid = guard_int(_from, 1);
  if(from_valid != NULL) { return from_valid; }
  
  int from = ((sl_d_int_t*)_from)->value;
  sl_d_string_t* str = (sl_d_string_t*)self;
  
  if(from <= str->length) {
    sl_d_obj_t* ret = (sl_d_obj_t*)sl_d_string_new(&str->value[from]);
    // sl_d_obj_release(_from);
    return ret;
  } else {
    return SL_D_NULL;
  }
}

// INTEGERS -------------------------------------------------------------------

static sl_d_sym_t* string_sym;
SL_I_METHOD_F(int_string) {//args: self, params
  sl_d_int_t* _self = (sl_d_int_t*)self;
  char buff[20];
  sprintf(buff, "%d", _self->value);
  // Construct return object
  sl_d_obj_t* ret = (sl_d_obj_t*)sl_d_string_new(buff);
  // And release the self since no longer needed
  // sl_d_obj_release(self);
  return ret;
}

static sl_d_sym_t* mod_sym;
SL_I_METHOD_F(int_mod) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other, 1);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  sl_d_obj_t* ret = (sl_d_obj_t*)sl_d_int_new(_self->value % _other->value);
  // sl_d_obj_release(self); sl_d_obj_release(other);
  return ret;
}

static sl_d_sym_t* add_sym;
SL_I_METHOD_F(int_add) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other, 1);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  sl_d_obj_t* ret = (sl_d_obj_t*)sl_d_int_new(_self->value + _other->value);
  
  sl_d_obj_release(self); sl_d_obj_release(other);
  
  return ret;
}

static sl_d_sym_t* sub_sym;
SL_I_METHOD_F(int_sub) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other, 1);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  sl_d_obj_t* ret = (sl_d_obj_t*)sl_d_int_new(_self->value - _other->value);
  
  sl_d_obj_release(self); sl_d_obj_release(other);
  
  return ret;
}

static sl_d_sym_t* mul_sym;
SL_I_METHOD_F(int_mul) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other, 1);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  sl_d_obj_t* ret = (sl_d_obj_t*)sl_d_int_new(_self->value * _other->value);
  
  sl_d_obj_release(self); sl_d_obj_release(other);
  
  return ret;
}

static sl_d_sym_t* div_sym;
SL_I_METHOD_F(int_div) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other, 1);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  sl_d_obj_t* ret = (sl_d_obj_t*)sl_d_int_new(_self->value / _other->value);
  
  sl_d_obj_release(self); sl_d_obj_release(other);
  
  return ret;
}

static sl_d_sym_t* lt_sym;
SL_I_METHOD_F(int_lt) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other, 1);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  if(_self->value < _other->value) {
    // sl_d_obj_release(self); sl_d_obj_release(other);
    return SL_D_TRUE;
  } else {
    // sl_d_obj_release(self); sl_d_obj_release(other);
    return SL_D_FALSE;
  }
}

static sl_d_sym_t* gt_sym;
SL_I_METHOD_F(int_gt) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other, 1);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  if(_self->value > _other->value) {
    // sl_d_obj_release(self); sl_d_obj_release(other);
    return SL_D_TRUE;
  } else {
    // sl_d_obj_release(self); sl_d_obj_release(other);
    return SL_D_FALSE;
  }
}

static sl_d_sym_t* cmp_sym;
SL_I_METHOD_F(int_cmp) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other, 1);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _self  = (sl_d_int_t*)self;
  sl_d_int_t* _other = (sl_d_int_t*)other;
  if(_self->value == _other->value) {
    // sl_d_obj_release(self); sl_d_obj_release(other);
    return SL_D_TRUE;
  } else {
    // sl_d_obj_release(self); sl_d_obj_release(other);
    return SL_D_FALSE;
  }
}

// ARRAYS ---------------------------------------------------------------------

static sl_d_sym_t* array_colon_sym;
SL_I_METHOD_F(array_new_length) {//args: self, params
  sl_d_obj_t* other = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* valid = guard_int(other, 1);
  if(valid != NULL) { return valid; }
  
  sl_d_int_t* _other = (sl_d_int_t*)other;
  sl_d_array_t* arr = sl_d_array_new_length(_other->value);
  return (sl_d_obj_t*)arr;
}

static sl_d_sym_t* index_sym;
SL_I_METHOD_F(array_index) {//args: self, params
  sl_d_obj_t* _index = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* index_valid = guard_int(_index, 1);
  if(index_valid != NULL) { return index_valid; }
  
  sl_d_int_t* index = (sl_d_int_t*)_index;
  sl_d_array_t* arr = (sl_d_array_t*)self;
  sl_i_array_item_t* item = sl_d_array_index_item(arr, index->value);
  if(item == NULL) {
    return SL_D_NULL;
  } else {
    return item->value;
  }
}

static sl_d_sym_t* index_to_sym;
SL_I_METHOD_F(array_to_index) {//args: self, params
  // Validations
  sl_d_obj_t* _index = (sl_d_obj_t*)sl_d_array_index(params, 0);
  sl_d_obj_t* index_valid = guard_int(_index, 1);
  if(index_valid != NULL) { return index_valid; }
  
  sl_d_int_t* index = (sl_d_int_t*)_index;
  sl_d_obj_t* obj   = sl_d_array_index(params, 1);
  sl_d_array_t* arr = (sl_d_array_t*)self;
  return sl_d_array_index_set(arr, index->value, obj);
}

SL_I_METHOD_F(array_length) {//args: self, params
  sl_d_array_t* arr = (sl_d_array_t*)self;
  int len = sl_d_array_length(arr);
  return (sl_d_obj_t*)sl_d_int_new(len);
}

// ----------------------------------------------------------------------------

void sl_i_bootstrap() {
  // Set the statics.
  type_sym = sl_d_sym_new("type");
  println_sym = sl_d_sym_new("println");
  print_sym = sl_d_sym_new("print");
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
  array_colon_sym = sl_d_sym_new("array:");
  index_sym = sl_d_sym_new("index:");
  index_to_sym = sl_d_sym_new("index:to:");
  while_do_sym = sl_d_sym_new("while:do:");
  if_then_sym = sl_d_sym_new("if:then:");
  lt_sym = sl_d_sym_new("<");
  gt_sym = sl_d_sym_new(">");
  length_sym = sl_d_sym_new("length");
  if_then_else_sym = sl_d_sym_new("if:then:else:");
  from_sym = sl_d_sym_new("from:");
  from_to_sym = sl_d_sym_new("from:to:");
  sub_sym = sl_d_sym_new("-");
  cmp_sym = sl_d_sym_new("==");
  
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
  
  // `while: { } do: { }`
  sl_d_method_t* while_do_method = sl_d_method_new();
  while_do_method->hint = *object_while_do;
  while_do_method->signature = while_do_sym;
  sl_d_obj_set_slot(root, while_do_sym, (sl_d_obj_t*)while_do_method);
  
  // `if: ( ) then: { }`
  sl_d_method_t* if_then_method = sl_d_method_new();
  if_then_method->hint = *object_if_then;
  if_then_method->signature = if_then_sym;
  sl_d_obj_set_slot(root, if_then_sym, (sl_d_obj_t*)if_then_method);
  
  // `if: ( ) then: { } else: { }`
  sl_d_method_t* if_then_else_method = sl_d_method_new();
  if_then_else_method->hint = *object_if_then_else;
  if_then_else_method->signature = if_then_else_sym;
  sl_d_obj_set_slot(root, if_then_else_sym, (sl_d_obj_t*)if_then_else_method);
  
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
  
  // `object array: length` -> new array
  sl_d_method_t* array_method = sl_d_method_new();
  array_method->hint = *array_new_length;
  array_method->signature = array_colon_sym;
  sl_d_obj_set_slot(root, array_colon_sym, (sl_d_obj_t*)array_method);
  
  // BOOLEANS -----------------------------------------------------------------
  
  sl_d_false = sl_d_obj_new();
  sl_d_false->type = SL_DATA_BOOL;
  
  sl_d_true = sl_d_obj_new();
  sl_d_true->type = SL_DATA_BOOL;
  
  // STRING -------------------------------------------------------------------
  
  // root string
  sl_d_obj_t* string = sl_d_obj_new();
  assert(string->parent == sl_i_root_object);
  sl_i_root_string = string;
  
  // `println` -> null
  sl_d_method_t* println_method = sl_d_method_new();
  println_method->hint = *string_println;
  println_method->signature = println_sym;
  sl_d_obj_set_slot(string, println_sym, (sl_d_obj_t*)println_method);
  
  // `print` -> null
  sl_d_method_t* print_method = sl_d_method_new();
  print_method->hint = *string_print;
  print_method->signature = print_sym;
  sl_d_obj_set_slot(string, print_sym, (sl_d_obj_t*)print_method);
  
  // `+ string` -> new string
  sl_d_method_t* string_add_method = sl_d_method_new();
  string_add_method->hint = *string_add;
  string_add_method->signature = add_sym;
  sl_d_obj_set_slot(string, add_sym, (sl_d_obj_t*)string_add_method);
  
  // `length` -> int
  sl_d_method_t* string_length_method = sl_d_method_new();
  string_length_method->hint = *string_length;
  string_length_method->signature = length_sym;
  sl_d_obj_set_slot(string, length_sym, (sl_d_obj_t*)string_length_method);
  
  // `from:` -> string
  sl_d_method_t* string_from_method = sl_d_method_new();
  string_from_method->hint = *string_from;
  string_from_method->signature = from_sym;
  sl_d_obj_set_slot(string, from_sym, (sl_d_obj_t*)string_from_method);
  
  // `from:to:` -> string
  sl_d_method_t* string_from_to_method = sl_d_method_new();
  string_from_to_method->hint = *string_from_to;
  string_from_to_method->signature = from_to_sym;
  sl_d_obj_set_slot(string, from_to_sym, (sl_d_obj_t*)string_from_to_method);
  
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
  
  // `-` -> int
  sl_d_method_t* int_sub_method = sl_d_method_new();
  int_sub_method->hint = *int_sub;
  int_sub_method->signature = sub_sym;
  sl_d_obj_set_slot(root_int, sub_sym, (sl_d_obj_t*)int_sub_method);
  
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
  
  // `<` -> bool
  sl_d_method_t* int_lt_method = sl_d_method_new();
  int_lt_method->hint = *int_lt;
  int_lt_method->signature = lt_sym;
  sl_d_obj_set_slot(root_int, lt_sym, (sl_d_obj_t*)int_lt_method);
  
  // `>` -> bool
  sl_d_method_t* int_gt_method = sl_d_method_new();
  int_gt_method->hint = *int_gt;
  int_gt_method->signature = gt_sym;
  sl_d_obj_set_slot(root_int, gt_sym, (sl_d_obj_t*)int_gt_method);
  
  // `==` -> bool
  sl_d_method_t* int_cmp_method = sl_d_method_new();
  int_cmp_method->hint = *int_cmp;
  int_cmp_method->signature = cmp_sym;
  sl_d_obj_set_slot(root_int, cmp_sym, (sl_d_obj_t*)int_cmp_method);
  
  // ARRAY --------------------------------------------------------------------
  
  // root array
  sl_d_obj_t* root_array = sl_d_obj_new();
  assert(root_array->parent == sl_i_root_object);
  sl_i_root_array = root_array;
  
  // `index:` -> obj
  sl_d_method_t* array_index_method = sl_d_method_new();
  array_index_method->hint = *array_index;
  array_index_method->signature = index_sym;
  sl_d_obj_set_slot(root_array, index_sym, (sl_d_obj_t*)array_index_method);
  
  // `index:to:` -> obj
  sl_d_method_t* array_index_to_method = sl_d_method_new();
  array_index_to_method->hint = *array_to_index;
  array_index_to_method->signature = index_to_sym;
  sl_d_obj_set_slot(
    root_array, index_to_sym, (sl_d_obj_t*)array_index_to_method
  );
  
  // `index:` -> obj
  sl_d_method_t* array_length_method = sl_d_method_new();
  array_length_method->hint = *array_length;
  array_length_method->signature = length_sym;
  sl_d_obj_set_slot(root_array, length_sym, (sl_d_obj_t*)array_length_method);
  
  // MORE STATICS (Dependent on sl_i_root_object, sl_i_root_string, etc.) -----
  
  type_str_object = sl_d_string_new("object");
  // And updating those already defined.
  type_sym->parent = sl_i_root_object;
  println_sym->parent = sl_i_root_object;
  methodto_sym->parent = sl_i_root_object;
  
  SL_GC_DEBUG("BOOTSTRAP DONE");
}
