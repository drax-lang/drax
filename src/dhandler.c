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

/**
 * Convert string to hash.
 */
static size_t string_to_id(const char* key, int len) {
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
d_var_table* new_var_table() {
  d_var_table* t = (d_var_table*) malloc(sizeof(d_var_table));
  t->size = HASH_VAR_TABLE_SIZE;
    t->array = malloc(HASH_VAR_TABLE_SIZE * sizeof(struct node*));
    for (int i = 0; i < HASH_VAR_TABLE_SIZE; i++) {
        t->array[i] = NULL;
    }

  return t;
}

void free_table(d_vm* vm, d_var_table* t) {
  UNUSED(vm);
    for (int i = 0; i < t->size; i++) {
        struct drax_node* current = t->array[i];
        while (current != NULL) {
            struct drax_node* next = current->next;
            // free(current->value);
            free(current);
            current = next;
        }
    }
    free(t->array);
    free(t);
}

void put_var_table(d_var_table* t, char* name, drax_value value) {
    int index = generate_hash(name, t->size);
    drax_node* node = malloc(sizeof(drax_node));
    node->key = string_to_id(name, strlen(name));
    node->value = value;
    node->next = t->array[index];
    t->array[index] = node;
}

drax_value get_var_table(d_var_table* t, char* name) {
    int index = generate_hash(name, t->size);
    size_t key = string_to_id(name, strlen(name));

    drax_node* current = t->array[index];
    while (current != NULL) {
        if (current->key == key) {
            return current->value;
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
  if (t->count >= t->limit) {
    t->limit = t->limit == 0 ? 8 : t->limit * 2;
    t->pairs = realloc(t->pairs, sizeof(drax_fun_store) * t->limit);
  }

  drax_function* f = CAST_FUNCTION(value);
  t->pairs[t->count].key = string_to_id(f->name, strlen(f->name));
  t->pairs[t->count].value = value;
  t->pairs[t->count].args = f->arity;
  t->count++;
}

drax_value get_fun_table(d_fun_table* t, char* key, uint8_t arity) {
  size_t hs = string_to_id(key, strlen(key));
  for (int i = 0; i < t->count; i++) {
    if (t->pairs[i].key == hs && t->pairs[i].args == arity) {
      return t->pairs[i].value;
    }
  }

  return 0;
}
