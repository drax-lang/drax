#include <stdlib.h>
#include <string.h>

#include "dstructs.h"
#include "dhandler.h"
#include "dtypes.h"
#include "ddefs.h"

static size_t gen_hash_idx(const char* key, int len) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < len; i++) {
    hash ^= (d_byte_def) key[i];
    hash *= 16777619;
  }
  return hash;
}

/* Hash table with simple layer */

d_var_table* new_var_table() {
  d_var_table* t = (d_var_table*) malloc(sizeof(d_var_table));
  t->pairs = NULL;
  t->count = 0;
  t->limit = 0;

  return t;
}

void free_table(d_vm* vm, d_var_table* t) {
  UNUSED(vm);
  free(t->pairs);
  // free(t);
  t = new_var_table();
}

void put_var_table(d_var_table* t, char* key, drax_value value) {
  if (t->count >= t->limit) {
    t->limit = t->limit == 0 ? 8 : t->limit * 2;
    t->pairs = realloc(t->pairs, sizeof(drax_value) * t->limit);
  }

  t->pairs[t->count -1].key = gen_hash_idx(key, strlen(key));
  t->pairs[t->count -1].value = value;
  t->count++;
}
