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
  free(t);
  t = new_var_table();
}

void put_var_table(d_var_table* t, char* key, drax_value value) {
  if (t->count >= t->limit) {
    t->limit = t->limit == 0 ? 8 : t->limit * 2;
    t->pairs = realloc(t->pairs, sizeof(drax_value) * t->limit);
  }

  t->pairs[t->count].key = gen_hash_idx(key, strlen(key));
  t->pairs[t->count].value = value;
  t->count++;
}

drax_value get_var_table(d_var_table* t, char* key) {
  size_t hs = gen_hash_idx(key, strlen(key));
  for (int i = 0; i < t->count; i++) {
    if (t->pairs[i].key == hs) {
      return t->pairs[i].value;
    }
  }

  return 0;
}

/**
 * function definitions
*/

d_fun_table* new_fun_table() {
  d_fun_table* t = (d_fun_table*) malloc(sizeof(d_fun_table));
  t->pairs = NULL;
  t->count = 0;
  t->limit = 0;

  return t;
}

void free_fun_table(d_vm* vm, d_fun_table* t) {
  UNUSED(vm);
  free(t->pairs);
  free(t);
  t = new_fun_table();
}

void put_fun_table(d_fun_table* t, drax_value value) {
  if (t->count >= t->limit) {
    t->limit = t->limit == 0 ? 8 : t->limit * 2;
    t->pairs = realloc(t->pairs, sizeof(drax_fun_store) * t->limit);
  }

  drax_function* f = CAST_FUNCTION(value);
  t->pairs[t->count].key = gen_hash_idx(f->name, strlen(f->name));
  t->pairs[t->count].value = value;
  t->pairs[t->count].args = f->arity;
  t->count++;
}

drax_value get_fun_table(d_fun_table* t, char* key, uint8_t arity) {
  size_t hs = gen_hash_idx(key, strlen(key));
  for (int i = 0; i < t->count; i++) {
    if (t->pairs[i].key == hs && t->pairs[i].args == arity) {
      return t->pairs[i].value;
    }
  }

  return 0;
}
