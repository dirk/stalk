#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import <math.h>

#import "stalk.h"
#import "data.h"
#import "symbol.h"


#import "deps/uthash/src/utarray.h"

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
