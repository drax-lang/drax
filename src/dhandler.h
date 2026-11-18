#ifndef __DHHANDLER
#define __DHHANDLER

#define HASH_VAR_TABLE_SIZE 80

#include "dstructs.h"
#include "dtypes.h"

typedef struct drax_string drax_string;

typedef struct d_vm d_vm;

/* Global envinronment */

typedef struct drax_generic_var_node {
  size_t key;
  drax_value value;
  struct drax_generic_var_node* next;
} drax_generic_var_node;

typedef struct d_generic_var_table {
    int size;
    drax_generic_var_node** array;
} d_generic_var_table;

/* Local envinronment */

/**
 * Local slots are stored inline (no allocation per variable).
 * A slot with key == 0 is unused.
 */
typedef struct d_local_var_node {
  size_t key;
  drax_value value;
} d_local_var_node;

typedef struct d_local_var_table {
  int count;
  int limit;
  d_local_var_node* array;
} d_local_var_table;

/* Function envinronment */
typedef struct drax_fun_node {
  size_t key;
  size_t args;
  drax_value value;
} drax_fun_node;

typedef struct d_fun_table {
  int count;
  int limit;
  drax_fun_node* pairs;
} d_fun_table;

/**
 * Module envinronment
 */

typedef struct d_mod_table {
  int count;
  int limit;
  drax_value* modules;
  /**
   * bit (c & 63) is set when a module name starts with the character c.
   * A name lookup (done for every variable and function) that starts with
   * another character is rejected without walking the modules.
   */
  uint64_t first_chars;
} d_mod_table;

/* 
 * Variable definitions
 */

size_t fnv1a_hash(const char* key, int len);

/**
 * The hashes of a name are computed in one pass over the characters, a
 * lookup that tries several tables hashes the name once (see lookup_name):
 *
 *   fnv     the key of an entry (all the tables)
 *   bucket  the slot of a global table (fnv is not used for it)
 */
#define D_FNV_OFFSET 2166136261u

static inline uint32_t d_fnv_step(uint32_t h, char c) {
  return (h ^ (d_byte_def) c) * 16777619u;
}

static inline uint32_t d_bucket_step(uint32_t h, char c) {
  return (uint32_t) c + 31u * h;
}

/* false when no module has a name that starts with 'c' (see get_mod_table) */
static inline int d_mod_may_have(const d_mod_table* t, char c) {
  return (int) ((t->first_chars >> (c & 63)) & 1);
}

d_generic_var_table* new_var_table();

void free_var_table(d_vm* vm, d_generic_var_table* t);

void put_var_table(d_generic_var_table* t, char* name, drax_value value);

void put_var_table_h(d_generic_var_table* t, uint32_t bucket, size_t fnv, drax_value value);

int get_var_table(d_generic_var_table* t, char* name, drax_value* value);

/* the same with the hashes of the name already computed */
int get_var_table_h(d_generic_var_table* t, uint32_t bucket, size_t fnv, drax_value* value);

void del_var_table(d_generic_var_table* t, char* name);

/**
 * Local variable definitions.
 */

d_local_var_table* new_local_table();

int ensure_local_table_capacity(d_local_var_table* t, int extra);


/**
 * Function definitions.
 */

d_fun_table* new_fun_table();

void put_fun_table(d_fun_table* t, drax_value value);

drax_value get_fun_table(d_fun_table* t, char* key, uint8_t arity);

/**
 * Module definitions.
 */

d_mod_table* new_mod_table();

void put_mod_table(d_mod_table* t, drax_value value);

int get_mod_table(d_mod_table* t, char* name, drax_value* value);

#endif
