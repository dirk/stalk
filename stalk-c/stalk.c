#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stalk.h"
#include "syntax.h"
#include "data.h"
#include "symbol.h"

// void test(char *s) {
//   // printf("\"%s\" = %d\n", s, sl_str_to_sym_int(s));
// }

void test() {
  sl_s_expr_t* s = sl_s_expr_new();
  sl_s_sym_t* sym = sl_s_sym_new();
  sym->value = "test";
  
  void* scope = malloc(sizeof(NULL));
  
  // sl_s_eval(s, scope);
  sl_s_eval(sym, scope);
  sl_s_eval(sym, scope);
  sym->hint = NULL;
  sl_s_eval(sym, scope);
  
  free(scope);
  
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
