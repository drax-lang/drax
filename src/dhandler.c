#include <stdlib.h>
#include <string.h>

#include "dstructs.h"
#include "dhandler.h"
#include "dtypes.h"
#include "ddefs.h"

static unsigned long generate_hash(const char* str, size_t size) {
  unsigned int hashval = 0;
  for (int i = 0; str[i] != '\0'; i++) {
      hashval = str[i] + 31 * hashval;
  }
  return hashval % size;
}

/* Return 64-bit FNV-1a hash
 * Generate hash key
 */
size_t fnv1a_hash(const char* key, int len) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < len; i++) {
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
    for (int i = 0; i < HASH_VAR_TABLE_SIZE; i++) {
        t->array[i] = NULL;
    }

  return t;
}

void free_var_table(d_vm* vm, d_generic_var_table* t) {
  UNUSED(vm);
    for (int i = 0; i < t->size; i++) {
        struct drax_generic_var_node* current = t->array[i];
        while (current != NULL) {
            struct drax_generic_var_node* next = current->next;
            // free(current->value);
            free(current);
            current = next;
        }
    }
    free(t->array);
    free(t);
}

void put_var_table(d_generic_var_table* t, char* name, drax_value value) {
    int index = generate_hash(name, t->size);
    drax_generic_var_node* node = malloc(sizeof(drax_generic_var_node));
    node->key = fnv1a_hash(name, strlen(name));
    node->value = value;
    node->next = t->array[index];
    t->array[index] = node;
}

int get_var_table(d_generic_var_table* t, char* name, drax_value* value) {
    int index = generate_hash(name, t->size);
    size_t key = fnv1a_hash(name, strlen(name));

    drax_generic_var_node* current = t->array[index];
    while (current != NULL) {
        if (current->key == key) {
            *value = current->value;
            return 1;
        }
        current = current->next;
    }

    return 0;
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

void free_fun_table(d_vm* vm, d_fun_table* t) {
  UNUSED(vm);
  free(t->pairs);
  free(t);
  t = new_fun_table();
}

void put_fun_table(d_fun_table* t, drax_value value) {
  #define set_key_and_args_2_fn(f) \
    t->pairs[t->count].key = fnv1a_hash(f->name, strlen(f->name)); \
    t->pairs[t->count].args = f->arity;

  if (t->count >= t->limit) {
    t->limit = t->limit == 0 ? 8 : t->limit * 2;
    t->pairs = realloc(t->pairs, sizeof(drax_fun_node) * t->limit);
  }

  if (IS_FUNCTION(value)) {
    drax_function* f = CAST_FUNCTION(value);
    set_key_and_args_2_fn(f);
  } else {
    drax_os_native* f = CAST_NATIVE(value);
    set_key_and_args_2_fn(f);
  }

  t->pairs[t->count].value = value;
  t->count++;
}

drax_value get_fun_table(d_fun_table* t, char* key, uint8_t arity) {
  size_t hs = fnv1a_hash(key, strlen(key));
  for (int i = 0; i < t->count; i++) {
    if (t->pairs[i].key == hs && t->pairs[i].args == arity) {
      return t->pairs[i].value;
    }
  }

  return 0;
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
  t->array[t->count++] = node;
}

drax_value get_local_table(d_local_var_table* t, int local_range, char* name, drax_value* value) {
  size_t key = fnv1a_hash(name, strlen(name));
  int limit = t->count - local_range;
  
  for (int i = t->count; i > limit; i--) {
    if (t->array[i -1]->key == key) {
      *value = t->array[i -1]->value;
      return 1;
    }
  }
  
  return 0;
}