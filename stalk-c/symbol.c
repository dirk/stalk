#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import <math.h>

#import "stalk.h"
#import "symbol.h"
#import "data.h"

#import "deps/uthash/src/utarray.h"

// Covers A-Z, a-z, 0-9, :
#define SYMBOL_CHAR_MAX 63
// #define SYMBOL_LENGTH_MAX 2
UT_array* sl_sym_quick_ref[63 * 63];
// Size of symbol table = 63 * 63 = 3969

inline short int sl_char_to_sym_int(const char _c) __attribute__((
  pure,
  always_inline,
  nothrow
));
inline short int sl_char_to_sym_int(const char _c) {
  static const short int colon = ':';
  static const short int zero = 0;//'0' - '0'
  static const short int nine = '9' - '0';
  static const short int A = 'A' - '0';
  static const short int Z = 'Z' - '0';
  static const short int a = 'a' - '0';
  static const short int z = 'z' - '0';
  
  if(_c == colon) { return 63; }
  short int c = ((short int)_c - zero);
  if(c <= nine) {
    return c;
  } else if(c >= A && c <= Z) {
    return c - A;
  } else if(c >= a && c <= z) {
    return c - a;
  }
  return 0;
  // printf("%c = %d\n", _c, r);
  // return r;
}

inline short int sl_str_to_sym_int(char* str) __attribute__((
  pure
));
inline short int sl_str_to_sym_int(char* str) {
  if(str[0] == '\0') { return -1; }
  if(str[1] == '\0') { return sl_char_to_sym_int(str[0]); }
  return (
    sl_char_to_sym_int(str[0]) +
    (SYMBOL_CHAR_MAX * sl_char_to_sym_int(str[1]))
  );
  /*
  short int len = strlen(str);
  short int max = (len > SYMBOL_LENGTH_MAX) ? SYMBOL_LENGTH_MAX : len;
  short int acc = 0;
  for(short int i = 0; i < max; i++) {
    acc += pow(SYMBOL_CHAR_MAX,i) + sl_char_to_sym_int(str[i]);
  }
  return acc;
  */
}


