#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "debug.h"

#include "stalk.h"
#include "syntax.h"
#include "data.h"
#include "symbol.h"


#include "deps/uthash/src/utarray.h"

sl_sym_id sl_i_str_to_sym_id(char *str) {
  // TODO: Add len parameter to reduce strlen() calls.
  int len = strlen(str);
  return sl_i_fnv1a(str, len);
}

inline unsigned int sl_i_fnv1a(char *str, int len) {
  unsigned int h = 2166136261;
  for(int i = 0; i < len; i++) {
    h = (h ^ str[i]) * 16777619;
  }
  return h;
}

char* sl_i_sym_value_to_cstring(sl_d_sym_t* s) {
  LOG_WARN("Dangerous: make sure to free the return pointer");
  char* buffer = malloc(sizeof(char) * (s->length + 1));
  sprintf(buffer, "%*s", s->length, s->value);
  return buffer;
}

bool sl_i_sym_eq(sl_d_sym_t* a, sl_d_sym_t* b) {
  return (a->id == b->id) ? true : false;
  // Lololol we don't need the below since syms all have uniq ids
  /*
  if(a->length != b->length) { return false; }
  unsigned char l = a->length;
  for(unsigned char i = 0; i < l; i++) {
    if(a->value[i] != b->value[i]) {
      return false;
    }
  }
  return true;
  */
}
