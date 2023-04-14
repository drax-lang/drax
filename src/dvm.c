#include <stdlib.h>
#include <stdio.h>
#include "dvm.h"
#include "dtypes.h"
#include "dinspect.h"

/**
 * Helpers
*/
#define AS_NUMBER(v) draxvalue_to_num(v)
#define AS_VALUE(v)  num_to_draxvalue(v)

#define GET_VALUE(vm) *(vm->ip++)
#define GET_NUMBER(vm) AS_NUMBER(GET_VALUE(vm))

// validation only number
#define binary_op(vm, op) \
        double b = AS_NUMBER(pop(vm)); \
        double a = AS_NUMBER(pop(vm)); \
        push(vm, AS_VALUE(a op b)); 


/* Value Handler */

static void write_value_array(d_vm* vm, value_array* array, drax_value value) {
  UNUSED(vm);
  if (array->limit < array->count + 1) {
    int old_cap = array->limit;
    array->limit = EXPAND_CAPACITY(old_cap);
    array->values = realloc(array->values, sizeof(drax_value) * array->limit);
  }
  
  array->values[array->count] = value;
  array->count++;
}

static void init_value_array(value_array* array) {
  array->values = NULL;
  array->limit = 0;
  array->count = 0;
}

static void free_value_array(d_vm* vm, value_array* array) {
  UNUSED(vm);
  free(array->values);
  init_value_array(array);
}

/**
 * Remove value from pending bytes and put on main stack
 */

int add_drax_value(d_vm* vm, drax_byte* d_byte, drax_value value) {
  push(vm, value);
  write_value_array(vm, &d_byte->constants, value);
  pop(vm);
  return d_byte->constants.count - 1;
}

/* Byte Handler */
drax_byte* new_drax_value() {
  drax_byte* d_byte = (drax_byte*) malloc(sizeof(drax_byte));
  d_byte->count = 0;
  d_byte->limit = 0;
  d_byte->code = NULL;
  d_byte->lines = NULL;

  init_value_array(&d_byte->constants);
  return d_byte;
}

void free_drax_value(d_vm* vm, drax_byte* d_byte) {
  // FREE_ARRAY(gBSM, d_byte_def, d_byte->code, d_byte->limit);
  // FREE_ARRAY(gBSM, int, d_byte->lines, d_byte->limit);
  // free_value_array(gBSM, &d_byte->constants);
  d_byte = new_drax_value();
}

void append_drax_value(d_vm* vm, drax_byte* d_byte, drax_value byte, int line) {
  if (d_byte->limit < d_byte->count + 1) {
    int old_cap = d_byte->limit;
    d_byte->limit = EXPAND_CAPACITY(old_cap);
    d_byte->code = (drax_value*) realloc(d_byte->code, sizeof(drax_value) * d_byte->limit);
    d_byte->lines = (int*) realloc(d_byte->code, sizeof(int) * d_byte->limit);
  }

  d_byte->code[d_byte->count] = byte;
  d_byte->lines[d_byte->count] = line;
  d_byte->count++;
}

/* VM/Compilers Helpers */

int put_const_value_array(const_value_array* a, drax_value v) {
  a->count++;
  if (a->count >= a->limit) {
    a->limit = EXPAND_CAPACITY(a->limit);
    a->values =  (drax_value*) realloc(a->values, sizeof(drax_value) * a->limit);
  }

  a->values[a->count -1] = v;
  return a->count -1;
}

void free_const_value_array(const_value_array* a) {
  free(a->values);
  free(a);
}

/* VM Impl. */


void push(d_vm* vm, drax_value v) {
  *vm->stack = v;
  vm->stack++;
}

drax_value pop(d_vm* vm) {
  vm->stack--;
  return *vm->stack;
}

static void print_d_struct(drax_value value) {
  switch (DRAX_STYPEOF(value)) {
    case DS_LIST:
      // print_list(CAST_LIST(value));
      break;

    case DS_FUNCTION:
      break;

    case DS_NATIVE:
      printf("<function>");
      break;
    case DS_STRING:
      printf("%s", CAST_STRING(value)->chars);
      break;
    case DS_ERROR:
      printf("error");
      break;
  }
  putchar('\n');
}

static void __start__(d_vm* vm, int inter_mode) {
  VMDispatch {
    VMcond(vm) {
      VMCase(OP_CONST) {
        push(vm, GET_VALUE(vm));
        break;
      }
      VMCase(OP_NIL) {
        break;
      }
      VMCase(OP_TRUE) {
        break;
      }
      VMCase(OP_FALSE) {
        break;
      }
      VMCase(OP_LIST) {
        break;
      }
      VMCase(OP_POP) {
        break;
      }
      VMCase(OP_PUSH) {
        drax_value v = GET_VALUE(vm);
        // printf("push[%g)\n", AS_NUMBER(v));
        push(vm, v);
        break;
      }
      VMCase(OP_VAR) {
        drax_value v = pop(vm);
        char* k = (char*) GET_VALUE(vm);
        put_var_table(vm->envs->dynamic, k, v);
        // printf("var\n");
        // printf("k: %s\n", k);
        // printf("v: %g\n", AS_NUMBER(v));
        break;
      }
      VMCase(OP_EQUAL) {
        break;
      }
      VMCase(OP_GREATER) {
        break;
      }
      VMCase(OP_LESS) {
        break;
      }
      VMCase(OP_CONCAT) {
        break;
      }
      VMCase(OP_ADD) {
        binary_op(vm, +);
        break;
      }
      VMCase(OP_SUB) {
        binary_op(vm, -);
        break;
      }
      VMCase(OP_MUL) {
        binary_op(vm, *);
        break;
      }
      VMCase(OP_DIV) {
        // check division by zero
        binary_op(vm, /);
        break;
      }
      VMCase(OP_NOT) {
        break;
      }
      VMCase(OP_PRINT) {
        drax_value v = pop(vm);

        if(IS_STRUCT(v)) {
          print_d_struct(v);
          // printf("%g\n", AS_(v));
        }

        if(IS_NUMBER(v)) {
          printf("%g\n", AS_NUMBER(v));
        }
        break;
      }
      VMCase(OP_JMP) {
        break;
      }
      VMCase(OP_JMF) {
        break;
      }
      VMCase(OP_LOOP) {
        break;
      }
      VMCase(OP_CALL) {
        break;
      }
      VMCase(OP_FUN) {
        break;
      }
      VMCase(OP_RETURN) {
        break;
      }
      VMCase(OP_EXIT) {
        return;
      }
      default: {
        printf("runtime error.\n");
        break;
      }
    }
  }
  
}

/* Conatructor */

static dt_envs* new_environment() {
  dt_envs* e = (dt_envs*) malloc(sizeof(dt_envs));
  e->globals = new_var_table();
  e->strings = new_var_table();
  e->dynamic = new_var_table();
  return e;
}

d_vm* createVM() {
  d_vm* vm = (d_vm*) malloc(sizeof(d_vm));
  vm->instructions = (d_instructions*) malloc(sizeof(d_instructions));
  vm->instructions->values = (drax_value*) malloc(sizeof(drax_value) * MAX_INSTRUCTIONS);
  vm->instructions->instr_size = MAX_INSTRUCTIONS;
  vm->instructions->instr_count = 0;

  vm->stack = (drax_value*) malloc(sizeof(drax_value) * MAX_STACK_SIZE);
  vm->stack_size = MAX_STACK_SIZE;
  vm->stack_count = 0;
  vm->envs = new_environment();

  return vm;
}

/* Run functions */

static void __init__(d_vm* vm) {
  vm->ip = vm->instructions->values;
}

static void __free__(d_vm* vm) {
  /* TEMP */
  free(vm->instructions->values);
  free(vm->instructions);
  // free(vm->envs);
  vm->instructions = (d_instructions*) malloc(sizeof(d_instructions));
  vm->instructions->values = (drax_value*) malloc(sizeof(drax_value) * MAX_INSTRUCTIONS);
}

void __run__(d_vm* vm, int inter_mode) {
 __init__(vm);
 __start__(vm, inter_mode);
 __free__(vm);
}
