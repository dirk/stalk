#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

#include "stalk.h"
#include "syntax.h"
#include "data.h"
#include "symbol.h"

// void test(char *s) {
//   // printf("\"%s\" = %d\n", s, sl_str_to_sym_int(s));
// }

SL_I_METHOD_F(bootstrap_test) { //args: self, params
  DEBUG("testing!");
  return NULL;
}

void bootstrap(sl_d_scope_t* scope) {
  sl_d_sym_t* test_sym = sl_d_sym_new("test");
  sl_d_method_t* test_method = sl_d_method_new();
  
  test_method->hint = *bootstrap_test;
  test_method->signature = test_sym;
  
  sl_d_obj_set_method((sl_d_obj_t*)scope, test_sym, (sl_d_obj_t*)test_method);
}

void test() {
  sl_s_expr_t* s = sl_s_expr_new();
  sl_s_sym_t* sym = sl_s_sym_new();
  sym->value = malloc(sizeof(char) * 5);
  strcpy(sym->value, "test");
  sl_d_scope_t* root = sl_d_scope_new();
  bootstrap(root);
  sl_d_sym_t* test_sym = sl_d_sym_new("test");
  
  // sl_s_eval(sym, scope);
  // sl_s_eval(sym, scope);
  // sym->hint = NULL;
  // sl_s_eval(sym, scope);
  
  
  DL_PREPEND(s->head, (sl_s_base_t*)sym);
  sl_s_eval(s, root);
  
  sl_d_obj_free((sl_d_obj_t*)test_sym);
  sl_d_scope_free(root);
  sl_s_sym_free(sym);
  sl_s_expr_free(s);
}



int main(int argc, char *argv[]) {
  //test("a");
  //test("aa");
  //test("ba");
  //test("zz");
  //test(":");
  //test("a:");
  //test("");
  //test(":");
  
  test();
  
  return 0;
}
