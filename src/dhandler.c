#include <stdlib.h>
#include <string.h>

#include "dstructs.h"
#include "dhandler.h"
#include "dtypes.h"
#include "ddefs.h"

static unsigned long generate_hash(const char* str, size_t size) {
  unsigned int hashval = 0;
  int i;
  for (i = 0; str[i] != '\0'; i++) {
      hashval = str[i] + 31 * hashval;
  }
  return hashval % size;
}

/**
 * Return 64-bit FNV-1a hash
 * Generate hash key
 */
size_t fnv1a_hash(const char* key, int len) {
  uint32_t hash = 2166136261u;
  int i;
  for (i = 0; i < len; i++) {
    hash ^= (d_byte_def) key[i];
    hash *= 16777619;
  }
  return hash;
}

/**
 * Closed hashing implementation
 */
d_generic_var_table* new_var_table() {
  d_generic_var_table* t = (d_generic_var_table*) malloc(sizeof(d_generic_var_table));
  t->size = HASH_VAR_TABLE_SIZE;
  t->array = malloc(HASH_VAR_TABLE_SIZE * sizeof(struct node*));
  int i;
  for (i = 0; i < HASH_VAR_TABLE_SIZE; i++) {
    t->array[i] = NULL;
  }

  return t;
}

void free_var_table(d_vm* vm, d_generic_var_table* t) {
  UNUSED(vm);
  int i;
  for (i = 0; i < t->size; i++) {
    struct drax_generic_var_node* current = t->array[i];
    while (current != NULL) {
      struct drax_generic_var_node* next = current->next;
      /* free(current->value); */
      free(current);
      current = next;
    }
  }
  free(t->array);
  free(t);
}

static drax_generic_var_node*
get_elem_on_var_table(d_generic_var_table* t, int i, size_t k) {
  drax_generic_var_node* current = t->array[i];
  while (current != NULL) {
    if (current->key == k) {
      return current;
    }
    current = current->next;
  }

  return NULL;
}

int get_var_table(d_generic_var_table* t, char* name, drax_value* value) {
  int i = generate_hash(name, t->size);
  size_t k = fnv1a_hash(name, strlen(name));

  drax_generic_var_node* node = get_elem_on_var_table(t, i, k);
  if (node == NULL) return 0;

  *value = node->value;
  return 1;
}

void put_var_table(d_generic_var_table* t, char* name, drax_value value) {
  int i = generate_hash(name, t->size);
  size_t k = fnv1a_hash(name, strlen(name));

  drax_generic_var_node* node = get_elem_on_var_table(t, i, k);
  
  if (node == NULL) {
    node = (drax_generic_var_node*) malloc(sizeof(drax_generic_var_node));
    node->key = k;
    node->next = t->array[i];
    t->array[i] = node;
  }
  node->value = value;
}

/**
 * starting helper definitions for functions
 */

d_fun_table* new_fun_table() {
  d_fun_table* t = (d_fun_table*) malloc(sizeof(d_fun_table));
  t->pairs = NULL;
  t->count = 0;
  t->limit = 0;

  return t;
}

static drax_fun_node* 
get_elem_on_fun_table(d_fun_table* t, size_t h, uint8_t a) {
  int i;
  for (i = 0; i < t->count; i++) {
    if (t->pairs[i].key == h && t->pairs[i].args == a) {
      return &t->pairs[i];
    }
  }

  return NULL;
}

drax_value get_fun_table(d_fun_table* t, char* key, uint8_t arity) {
  size_t hs = fnv1a_hash(key, strlen(key));
  drax_fun_node* elem = get_elem_on_fun_table(t, hs, arity);

  if (elem == NULL) return 0;

  return elem->value;
}

void put_fun_table(d_fun_table* t, drax_value value) {
  if (t->count >= t->limit) {
    t->limit = t->limit == 0 ? 8 : t->limit * 2;
    t->pairs = realloc(t->pairs, sizeof(drax_fun_node) * t->limit);
  }
  
  size_t hs;
  int arity;

  if (IS_FUNCTION(value)) {
    drax_function* f = CAST_FUNCTION(value);
    hs = fnv1a_hash(f->name, strlen(f->name));
    arity = f->arity;
  } else {
    drax_os_native* f = CAST_NATIVE(value);
    hs = fnv1a_hash(f->name, strlen(f->name));
    arity = f->arity;
  }

  drax_fun_node* elem = get_elem_on_fun_table(t, hs, arity);

  if (elem != NULL) {
    elem->value = value;
    return;
  }

  t->pairs[t->count].key = hs;
  t->pairs[t->count].args = arity;
  t->pairs[t->count].value = value;
  t->count++;
}

/**
 * starting helper definitions for local definitions
 */

d_local_var_table* new_local_table() {
  d_local_var_table* t = (d_local_var_table*) malloc(sizeof(d_local_var_table));
  t->count = 0;
  t->limit = MAX_LOCAL_INSTRUCTIONS;
  t->array = (d_local_var_node**) malloc(sizeof(d_local_var_node*) * MAX_LOCAL_INSTRUCTIONS);

  return t;
}

void put_local_table(d_local_var_table* t, char* name, drax_value value) {
  if (t->count >= t->limit) {
    t->limit = t->limit + MAX_LOCAL_INSTRUCTIONS;
    t->array = (d_local_var_node**) realloc(t->array, sizeof(d_local_var_node*) * t->limit);
  }

  d_local_var_node* node = malloc(sizeof(d_local_var_node));
  node->key = fnv1a_hash(name, strlen(name));
  node->value = value;

  int next_index = t->count - 1;
  /**
   * check which is the next unused index.
   * 
   * x           => used
   * 0           => not used
   * 
   * t->array    =>  |x|x|x|x|x|x|0|0|0|x|
   * next_index  =>  (10 -1)
   * 
   * while verification will stop at the first 0
   * next_index  =>  (8)
   */
  while (next_index >= 0 && t->array[next_index] != 0) {
    next_index--;
  }

  t->array[next_index] = node;
}

int get_local_table(d_local_var_table* t, int local_range, char* name, drax_value* value) {
  if (t->count <= 0) return 0;

  size_t key = fnv1a_hash(name, strlen(name));
  int init = t->count - local_range;
  init = init < 0 ? 0 : init;

  int i;
  for (i = init; i < t->count; i++) {
    if ((t->array[i] != 0) && (t->array[i]->key == key)) {
      *value = t->array[i]->value;
      return 1;
    }
  }
  
  return 0;
}

/**
 * Modules implementation
 */

#define MOD_TABLE_INIT_SIZE 8

d_mod_table* new_mod_table() {

  d_mod_table* t = (d_mod_table*) malloc(sizeof(d_mod_table));
  t->limit = MOD_TABLE_INIT_SIZE;
  t->count = 0;
  t->modules = (drax_value*) malloc(sizeof(drax_value) * MOD_TABLE_INIT_SIZE);

  return t;
}

void put_mod_table(d_mod_table* t, drax_value value) {
  if (t->count >= t->limit) {
    t->limit = t->limit + MOD_TABLE_INIT_SIZE;
    t->modules = (drax_value*) realloc(t->modules, sizeof(drax_value) * t->limit);
  }

  t->modules[t->count++] = value;
}

int get_mod_table(d_mod_table* t, char* name, drax_value* value) {
  int i;
  for (i = 0; i < t->count; i++) {
    drax_native_module* m = CAST_MODULE(t->modules[i]);
    if (strcmp(m->name, name) == 0) {
      *value = DS_VAL(t->modules[i]);
      return 1;
    }
  }

  return 0;
} 
