/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */
 
#ifndef __DDEFS
#define __DDEFS

#include <stdint.h>

#define UNUSED(x) (void)(x)

#define MAX_INSTRUCTIONS 80

#define STACK_OP_SIZE   256

#define MAX_STACK_SIZE (MAX_INSTRUCTIONS * STACK_OP_SIZE)

#define MAX_LOCAL_INSTRUCTIONS (MAX_INSTRUCTIONS * STACK_OP_SIZE)

#define CALL_STACK_SIZE (MAX_INSTRUCTIONS * STACK_OP_SIZE)

/**
 * Maximum depth of non tail calls (body recursion).
 *
 * The value stack, the local stack and the call stack grow on demand,
 * like the stack of an Erlang process. This limit only exists to give a
 * clean error instead of exhausting the memory of the machine.
 *
 * Tail calls never count towards this limit.
 * Override at build time with: make DWN_CCFLAGS=-DMAX_CALL_DEPTH=<n>
 */
#ifndef MAX_CALL_DEPTH
  #define MAX_CALL_DEPTH 10000000
#endif

/**
 * Lists and frames can be nested as deep as the program wants (a list built
 * by recursion, data that comes from outside). Comparing (==, !=) and
 * printing walk them with the C stack, so they stop at this depth: the
 * comparison is an error and the printing writes "..." for what is deeper.
 * Override at build time with: make DWN_CCFLAGS=-DMAX_VALUE_DEPTH=<n>
 */
#ifndef MAX_VALUE_DEPTH
  #define MAX_VALUE_DEPTH 1000
#endif

/**
 * Native functions that call Drax functions (List.map, List.filter,
 * List.tap) run a nested VM loop on the C stack. This is the maximum
 * number of those loops nested inside each other (recursion that goes
 * through a native function), it keeps the C stack safe.
 */
#ifndef MAX_NATIVE_NESTING
  #define MAX_NATIVE_NESTING 5000
#endif

/**
 * The garbage collector runs automatically when the memory allocated
 * since the last cycle reaches a limit. After each cycle the limit is
 * twice the memory in use (and at least GC_INITIAL_THRESHOLD).
 */
/**
 * What the allocator of the system keeps for each block (its header and the
 * alignment). It is counted for the blocks of a function, which has many small
 * ones, so the collector sees the memory that is really in use.
 */
#define D_MALLOC_OVERHEAD 16

/* the blocks of a function: the function, its code, the values and the lines */
#define D_FUNCTION_BLOCKS 4

#ifndef GC_INITIAL_THRESHOLD
  #define GC_INITIAL_THRESHOLD (8 * 1024 * 1024)
#endif

#define MAX_SIZE_TABLE 0.85

#define MODE_T_MAX ((mode_t)-1)

typedef uint8_t d_byte_def;

/**
 * Global Macros
 */

#define args_fail_required_size(a, b, msg) \
   if (a != b) { { raise_drax_error(vm, msg); return 0; } };

#define args_fail_required_bigger_than(a, b) if (a < b) { return 0; };

#define args_fail_required_less_than(a, b) if (a > b) { return 0; };

#define match_dfunction(n, v, f, vm, a, s) if ((strcmp(n, v) == 0)) { return f(vm, a, s); }

/* Make String as Return */
#define MSR(g, c)  \
    return DS_VAL(copy_dstring(g, c, strlen(c)));

#define DX_SUCESS_FN(v) *v = 1;

#define DX_ERROR_FN(v) *v = 0;

#define return_if_is_not_function(v, s) \
  if (!IS_FUNCTION(v)) { \
    DX_ERROR_FN(s); \
    return DS_VAL(new_derror(vm, (char *) "Expected function/1 as argument")); \
  }

#define return_if_is_not_string(v, s) \
  if (!IS_STRING(v)) { \
    DX_ERROR_FN(s); \
    return DS_VAL(new_derror(vm, (char *) "Expected string as argument")); \
  }

#define return_if_is_not_number(v, s) \
  if (!IS_NUMBER(v)) { \
    DX_ERROR_FN(s); \
    return DS_VAL(new_derror(vm, (char *) "Expected number as argument")); \
  }

#define return_if_is_not_frame(v, s) \
  if (!IS_FRAME(v)) { \
    DX_ERROR_FN(s); \
    return DS_VAL(new_derror(vm, (char *) "Expected frame as argument")); \
  }

#define return_if_is_not_list(v, s) \
  if (!IS_LIST(v)) { \
    DX_ERROR_FN(s); \
    return DS_VAL(new_derror(vm, (char *) "Expected list as argument")); \
  }

#define return_if_is_not_module(v, s) \
  if (!IS_MODULE(v)) { \
    DX_ERROR_FN(s); \
    return DS_VAL(new_derror(vm, (char *) "Expected module as argument")); \
  }

#define return_if_is_not_tid(v, s) \
  if (!IS_TID(v)) { \
    DX_ERROR_FN(s); \
    return DS_VAL(new_derror(vm, (char *) "Expected tid as argument")); \
  }  

#endif