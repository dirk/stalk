#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import <math.h>

#import "symbol.h"

// Covers A-Z, a-z, 0-9, :
#define SYMBOL_CHAR_MAX 63
//#define SYMBOL_LENGTH_MAX 2
// Size of symbol table = 64 * 16 = 1024

static inline short int sl_char_to_sym_int(const char _c) {
  static const short int colon = ':';
  if(_c == colon) { return 63; }
  short int c = ((short int)_c - (short int)'0');
  
  //static const short int zero = 0;//'0' - '0'
  static const short int nine = '9' - '0';
  static const short int A = 'A' - '0';
  static const short int Z = 'Z' - '0';
  static const short int a = 'a' - '0';
  static const short int z = 'z' - '0';
  
  if(c <= nine) {
    return c;
  } else if(c <= Z) {
    return c - A;
  } else if(c <= z) {
    return c - a;
  }
  
  /*
  if(c >= A && c <= Z) {
    r = c - A;// up to 25
  } else if(c >= a && c <= z) {
    r = c - a + 26;// up to 51
  } else if(c >= zero && c <= nine) {
    r = c - zero + 52;// up to 62
  } else if(c == colon) {
    r = 63;// up to 63
  }
  */
  return 0;
  // printf("%c = %d\n", _c, r);
  // return r;
}

short int sl_str_to_sym_int(char *str) {
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
