#ifndef DATA_H
#define DATA_H

typedef unsigned char sl_data_type;
typedef int sl_sym_id;

#define SL_DATA_SYM (unsigned int)0

#define SL_DATA_TYPE sl_data_type type;

typedef struct sl_d_sym {
  SL_DATA_TYPE
    sl_sym_id id;
  char *value;
} sl_d_sym_t;

#endif
