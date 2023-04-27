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

typedef struct d_local_var_node {
  size_t key;
  drax_value value;
} d_local_var_node;

typedef struct d_local_var_table {
  int count;
  int limit;
  d_local_var_node** array;
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

/* 
 * Variable definitions
 */

size_t fnv1a_hash(const char* key, int len);

d_generic_var_table* new_var_table();

void free_var_table(d_vm* vm, d_generic_var_table* t);

void put_var_table(d_generic_var_table* t, char* name, drax_value value);

int get_var_table(d_generic_var_table* t, char* name, drax_value* value);

/**
 * Local variable definitions.
 */

d_local_var_table* new_local_table();

void put_local_table(d_local_var_table* t, char* name, drax_value value);

int get_local_table(d_local_var_table* t, int local_range, char* name, drax_value* value);


/**
 * Function definitions.
 */

d_fun_table* new_fun_table();

void free_fun_table(d_vm* vm, d_fun_table* t);

void put_fun_table(d_fun_table* t, drax_value value);

drax_value get_fun_table(d_fun_table* t, char* key, uint8_t arity);

#endif
