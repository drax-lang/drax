#ifndef __DHHANDLER
#define __DHHANDLER

#include "dstructs.h"
#include "dtypes.h"

typedef struct drax_string drax_string;
typedef struct d_vm d_vm;

/* Runtime hash function (Match definition) */

typedef struct d_fmdef_pair {
  drax_string* key;
  uint8_t arity;
  drax_value value;
} d_fmdef_pair;

typedef struct d_fun_table {
  int count;
  int limit;
  d_fmdef_pair* pairs;
} d_fun_table;

typedef struct drax_pairs {
  size_t key;
  drax_value value;
} drax_pairs;

/* Global envinronment */
typedef struct d_var_table {
  int count;
  int limit;
  drax_pairs* pairs;
} d_var_table;

/* impl. */

d_var_table* new_var_table();

void free_table(d_vm* vm, d_var_table* t);

void put_var_table(d_var_table* t, char* key, drax_value value);

drax_value get_var_table(d_var_table* t, char* key);

#endif
