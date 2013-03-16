#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "debug.h"

#include "stalk.h"
#include "syntax.h"
#include "data.h"
#include "symbol.h"
#include "bootstrap.h"



#include "parse.tab.h"
#include "stalk.yy.h"


// void test(char *s) {
//   // printf("\"%s\" = %d\n", s, sl_str_to_sym_int(s));
// }

SL_I_METHOD_F(bootstrap_test) { //args: self, params
  DEBUG("testing!");
  return NULL;
}
/*
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
  
  
  sl_s_expr_unshift(s, (sl_s_base_t*)sym);
  sl_s_eval(s, root);
  
  sl_d_obj_release((sl_d_obj_t*)test_sym);
  sl_d_scope_free(root);
  sl_s_sym_free(sym);
  sl_s_expr_free(s);
}
*/

extern int yyparse();
extern int yydebug;

unsigned int file_size(const char* filename) {
  struct stat s;
  if(stat(filename, &s) != 0) {
    fprintf(stderr, "'stat' failed on '%s': %s\n", filename, strerror(errno));
    exit(1);
  }
  return s.st_size;
}
char* file_read(const char* filename, unsigned int s) {
  // unsigned int s = file_size(filename);
  
  char* data = malloc(s + 1);
  if(data == NULL) {
    fprintf(stderr, "Out of memory");
    exit(1);
  }
  
  FILE* f = fopen(filename, "r");
  if(!f) {
    fprintf(stderr, "'fopen' failed on '%s': %s\n", filename, strerror(errno));
    exit(1);
  }
  size_t sr = fread(data, sizeof(char), s, f);
  if(sr != s) {
    fprintf(
      stderr, "Only read %d bytes from '%s', expected %d",
      (int)sr, filename, s
    );
    exit(1);
  }
  int status = fclose(f);
  if(status != 0) {
    fprintf(stderr, "'fclose' failed on '%s': %s\n", filename, strerror(errno));
    exit(1);
  }
  
  return data;
}

char* sl_read_source_file(const char* filename) {
  unsigned int size = file_size(filename);
  char* source = file_read(filename, size);
  source = realloc(source, size + 2);
  if(source == NULL) {
    fprintf(stderr, "Out of memory");
    exit(1);
  }
  source[size] = '\n';
  source[size + 1] = '\0';
  return source;
}

int main(int argc, char *argv[]) {
  if(argc != 2) {
    fprintf(stderr, "Usage: stalk file.sl\n");
    exit(0);
  }
  
  // Bootstrap the primitive data types
  sl_d_bootstrap();
  // Bootstrap the interpreter
  sl_i_bootstrap();
  
  
  
  char* filename = argv[1];
  char* source = sl_read_source_file(filename);
  
  sl_s_expr_t* head;
  yydebug = 0;
  
  
  yyscan_t scanner;
  YY_BUFFER_STATE buffer;
  yylex_init(&scanner);
  buffer = yy_scan_string(source, scanner);
  
  yyparse(&head, scanner, filename);
  
  // sl_parse(scanner, head);
  
  yy_delete_buffer(buffer, scanner);
  yylex_destroy(scanner);
  
  
  sl_d_scope_t* root = sl_d_scope_new();
  
  sl_d_obj_t* ret = sl_s_eval(head, root);
  if(ret->type == SL_DATA_EXCEPTION) {
    sl_i_exception_print((sl_d_exception_t*)ret);
  }
  
  
  return 0;
  
  
  
  return 0;
}
