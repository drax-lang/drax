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

#define AS_NUMBER(v) draxvalue_to_num(v)
#define AS_VALUE(v)  num_to_draxvalue(v)

#define GET_VALUE(vm) *(vm->ip++)
#define GET_NUMBER(vm) AS_NUMBER(GET_VALUE(vm))

#define CURR_CALLSTACK_SIZE(vm) vm->call_stack->count

/* Trace messages */
#define TRACE_DESCRIPTION_LINE "  line: %d "

/* Common messages */
#define MSG_NAME_IS_NOT_DEFINED    "'%s' is not defined."
#define MSG_NUMBER_OF_INVALID_ARGS "Number of invalid arguments, expected %d arguments."
#define MSG_BAD_AGR_ARITH_OP       "Bad argument in arithmetic expression."

typedef struct dt_envs {
  d_mod_table* modules;
  d_fun_table* native;
  d_fun_table* functions;
  d_generic_var_table* global;
  d_local_var_table* local; /* Local definitions inside functions */
} dt_envs;

typedef struct  dcall_stack {
  d_instructions** values;
  int count;
  int size;
} dcall_stack;

typedef struct d_vm {
  dt_envs* envs;
  drax_value* ip;
  drax_value* stack;
  dcall_stack* call_stack;
  int stack_count;
  int stack_size;
  d_instructions* instructions; /* global instructions */
  d_instructions* active_instr; /* active instructions */
  d_struct* d_ls;
} d_vm;

/* VM */
d_vm* createVM();

void push(d_vm* vm, drax_value v);

drax_value pop(d_vm* vm);

void __reset__(d_vm* vm);

void __run__(d_vm* curr, int inter_mode);

#endif
