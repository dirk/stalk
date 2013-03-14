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

void sl_i_bootstrap() {
  sl_d_obj_t* object = sl_d_obj_new();
  
  // TODO: Bootstrap this language.
}
