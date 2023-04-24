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

#define dg16_byte(f) \
    (f->ip += 2, (uint16_t)((f->ip[-2] << 8) | f->ip[-1]))

#define VMCase(t) case t:

/* Trace messages */
#define TRACE_DESCRIPTION_LINE "  line: %d "

/* Common messages */
#define MSG_NAME_IS_NOT_DEFINED    "'%s' is not defined."
#define MSG_NUMBER_OF_INVALID_ARGS "Number of invalid arguments, expected %d arguments."
#define MSG_BAD_AGR_ARITH_OP       "Bad argument in arithmetic expression."

typedef struct const_value_array{
  int limit;
  int count;
  drax_value* values;
} const_value_array;

typedef struct dt_envs {
  d_fun_table* native;
  d_fun_table* functions;
  d_global_var_table* global;
  d_local_var_table* local; // Local definitions inside functions
} dt_envs;

typedef struct  dcall_stack {
  d_instructions** values;
  int count;
  int size;
} dcall_stack;

typedef struct d_vm {
  // uint8_t ip
  dt_envs* envs;
  drax_value* ip;
  drax_value* stack;
  dcall_stack* call_stack;
  int stack_count;
  int stack_size;
  d_instructions* instructions; // global instructions
  d_instructions* active_instr; // active instructions
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

void append_drax_value(d_vm* vm, drax_byte* d_byte, drax_value byte, int line);

int add_drax_value(d_vm* vm, drax_byte* d_byte, drax_value value);

/* VM */
d_vm* createVM();

const_value_array* new_const_value_array();

void push(d_vm* vm, drax_value v);

drax_value pop(d_vm* vm);

void __reset__(d_vm* vm);

void __run__(d_vm* curr, int inter_mode);

#endif
