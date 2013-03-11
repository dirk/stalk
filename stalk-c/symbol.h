#ifndef SYMBOL_H
#define SYMBOL_H

// Not exposed outside of symbol.c
// static inline short int sl_char_to_sym_int(const char _c);
// short int sl_str_to_sym_int(char *str);

sl_sym_id sl_i_str_to_sym_id(char *str);
unsigned int sl_i_fnv1a(char *str, int len);

#endif
