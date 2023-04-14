/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __DVM
#define __DVM

#include <stdbool.h>
#include "dtypes.h"
#include "dhandler.h"

#define is_call_fn(v) ((v->act != BACT_CALL_OP) && (v->act != BACT_CORE_OP))

#define VMDispatch while(true)

#define VMcond(f) \
     switch (*(f->ip++))

#define VMCase(t) case t:

typedef struct const_value_array{
  int limit;
  int count;
  drax_value* values;
} const_value_array;

typedef struct dt_envs {
  d_var_table* globals;
  d_var_table* strings;
  d_var_table* dynamic;
} dt_envs;

typedef struct d_instructions {
    drax_value* values;
    int instr_count;
    int instr_size;
    // stack trace
} d_instructions;

typedef struct d_vm {
  // uint8_t ip
  dt_envs* envs;
  drax_value* ip;
  drax_value* stack;
  int stack_count;
  int stack_size;
  d_instructions* instructions;
} d_vm;

typedef struct value_array{
  int limit;
  int count;
  drax_value* values;
} value_array;

typedef struct drax_byte {
  int count;
  int limit;
  drax_value* code;
  int* lines;
  value_array constants;
} drax_byte;

drax_byte* new_drax_value();

/* Value drax */
void free_drax_value(d_vm* vm, drax_byte* d_byte);

void append_drax_value(d_vm* vm, drax_byte* d_byte, drax_value byte, int line);

int add_drax_value(d_vm* vm, drax_byte* d_byte, drax_value value);

/* VM */
d_vm* createVM();

const_value_array* new_const_value_array();

int put_const_value_array(const_value_array* a, drax_value v);

void free_const_value_array(const_value_array* a);

void push(d_vm* vm, drax_value v);

drax_value pop(d_vm* vm);

void __run__(d_vm* curr, int inter_mode);

#endif
