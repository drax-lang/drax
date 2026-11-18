/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __DVM
#define __DVM

#include <stdbool.h>
#include <stdarg.h>
#include "dtypes.h"
#include "dhandler.h"
#include "dstructs.h"

#define is_call_fn(v) ((v->act != BACT_CALL_OP) && (v->act != BACT_CORE_OP))

#ifdef _AST_INSPECT
 #define DEBUG(x) (x)
#else
 #define DEBUG(x)
#endif

#ifdef _AST_INSPECT_OP
 #define DEBUG_OP(x) (x)
#else
 #define DEBUG_OP(x)
#endif

/**
 * The interpreter loop (__exec__) keeps the instruction pointer in the
 * local variable 'ip', not in vm->ip: it is written to vm->ip (SYNC_IP) before
 * anything outside of the loop can read it (calls, errors, the garbage
 * collector) and read again (RELOAD_IP) when that can change it.
 */
#ifdef _COMP_GOTO
  #define VMDispatch(vm) goto *dispatch_table[*(ip++)];

  #define VMcond(f) 

  #define VMCase(t) do_##t:

  #define NEXT_OP(vm) \
      goto *dispatch_table[*(ip++)]

  #define VMDefault() do_default:
#else
  #define VMDispatch(vm) while(true)

  #define VMcond(f) \
      switch (*(ip++))

  #define VMCase(t) case t:

  #define NEXT_OP(vm) break
#endif

#define AS_NUMBER(v) draxvalue_to_num(v)
#define AS_VALUE(v)  num_to_draxvalue(v)

#define GET_VALUE(vm) *(vm->ip++)
#define GET_NUMBER(vm) AS_NUMBER(GET_VALUE(vm))

#define CURR_CALLSTACK_SIZE(vm) vm->call_stack->count

#define CURR_FRAME(vm) (&vm->call_stack->frames[vm->call_stack->count - 1])

/* Trace messages */
#define TRACE_DESCRIPTION_LINE      "  line: %d"
#define TRACE_DESCRIPTION_LINE_PATH "  %s:%d"

/* Common messages */
#define MSG_STACK_OVERFLOW          "stack overflow: the maximum call depth was exceeded."
#define MSG_NATIVE_OVERFLOW         "stack overflow: too many nested calls through native functions (List.map, List.filter, List.tap)."
#define MSG_NAME_IS_NOT_DEFINED     "'%s' is not defined."
#define MSG_NUMBER_OF_INVALID_ARGS  "Number of invalid arguments, expected %d arguments."
#define MSG_BAD_AGR_ARITH_OP        "Bad argument in arithmetic expression."
#define MSG_FUNCTION_IS_NOT_DEFINED "error: function '%s/%d' is not defined\n"

#define VM_STATUS_STOPED   0 /* prepared to work */
#define VM_STATUS_WORKING  1 /* working */
#define VM_STATUS_FINISHED 2 /* finished, prepared to get result */

/**
 * Validators for arguments
*/
#define dvalidate_type(vm, fun, v, msg)  \
  if (!fun(v)) { raise_drax_error(vm, msg); return 0; }

#define dvalidate_string(vm, v, msg) dvalidate_type(vm, IS_STRING, v, msg)

#define dvalidate_number(vm, v, msg) dvalidate_type(vm, IS_NUMBER, v, msg)

#define dvalidate_list(vm, v, msg) dvalidate_type(vm, IS_LIST, v, msg)

typedef struct dt_envs {
  d_mod_table* modules;
  d_fun_table* native;
  d_generic_var_table* global;
  d_local_var_table* local; /* Local definitions inside functions */
} dt_envs;

/**
 * A call frame keeps the state of the caller (instr, ip, env_global) and
 * where the callee data begins on the value stack and on the local stack.
 *
 * stack_base/local_base are used by OP_RETURN to drop everything the
 * callee left behind, and by tail calls to reuse the frame.
 */
typedef struct d_frame {
  d_instructions* instr;
  drax_value* ip;
  d_generic_var_table* env_global;
  int stack_base;
  int local_base;
  drax_value callee;              /* function running in this frame (kept alive by the GC) */
} d_frame;

/**
 * A frame started by a protected call (f?(...)). A failure inside the frame
 * ends it and the call evaluates to an Error value, see recover_protected.
 * The entries are kept apart from the frames, so a normal call pays nothing
 * for them (the frame stays small).
 */
typedef struct d_prot {
  int frame; /* index of the frame on the call stack */
  int loop;  /* depth of the VM loop that started the call */
} d_prot;

typedef struct dcall_stack {
  d_frame* frames;
  int count;
  int size;
} dcall_stack;

typedef struct dgc_meta {
  int n_cycles;
  int n_free_structs;
  size_t bytes;      /* bytes allocated by structs since the last cycle (estimate) */
  size_t next_gc;    /* a cycle runs when 'bytes' reaches this value */
  drax_value* gray;  /* mark stack */
  int gray_count;
  int gray_cap;
} dgc_meta;

/**
 * The names used by the instructions (variables, functions with their
 * arity) are hashed once, when the compiler creates them. The hashes are
 * kept in a header right before the characters, so a name is looked up
 * without reading it again (see dpool_strndup and lookup_name).
 */
#define D_NAME_MAGIC 0xD4A7A11Eu

typedef struct d_name_hdr {
  uint32_t magic;
  int slash;     /* position of the last '/' (the arity), -1 when there is none */
  uint32_t fk;   /* FNV hash of the whole name */
  uint32_t bk;   /* bucket hash of the whole name */
  uint32_t fb;   /* FNV hash of the name before the last '/' */
  uint32_t bb;   /* bucket hash of the name before the last '/' */
} d_name_hdr;

/**
 * Strings created by the compiler (names used as operands of
 * instructions). They live until the VM is destroyed.
 */
typedef struct d_str_pool {
  char** items;
  int count;
  int cap;
} d_str_pool;

typedef struct d_vm {
  /**
   * vid: VM Id
   * 
   * -2     import
   * -1     main vm
   * other  import 
   */
  int vid;

  dt_envs* envs;
  drax_value* ip;
  drax_value* stack;
  drax_value* exported;
  dcall_stack* call_stack;
  int stack_count;
  int stack_size;
  d_instructions* instructions; /* global instructions */
  d_instructions* active_instr; /* active instructions */
  d_struct* d_ls;
  int pstatus;
  int pipeID;
  int max_call_depth; /* see MAX_CALL_DEPTH and DRAX_MAX_CALL_DEPTH */
  int native_nesting; /* nested VM loops started by native functions */
  dgc_meta* gc_meta;
  d_generic_var_table* imported_files;
  d_str_pool* str_pool;

  /* Error handling, see docs/error-handling.md */
  d_prot* prot_stack;  /* protected frames, innermost last */
  int protect_count;   /* entries of prot_stack */
  int prot_cap;
  int soft_depth;      /* protected calls of natives being executed */
  int loop_depth;      /* nested __start__ loops (natives, imports) */
  int pending_error;   /* an error was raised and is waiting for its handler */
  char* pending_msg;   /* message of the pending error (heap) */
} d_vm;

/* VM */
d_vm* createMainVM();

void __destroy__(d_vm* vm);

char* dpool_add(d_vm* vm, char* s);

char* dpool_strndup(d_vm* vm, const char* s, size_t n);

void free_instructions(d_instructions* instr);

d_vm* ligth_based_createVM(d_vm* vm_base, int vid, int clone_gc, int new_global_scope);

void push(d_vm* vm, drax_value v);

drax_value pop(d_vm* vm);

void raise_drax_error(d_vm* vm, const char* format, ...);

void zero_new_local_range(d_vm* vm, int range);

void do_call_function_no_validation(d_vm* vm, drax_value f);

dt_envs* new_environment(int ignore_natives, int ignore_local, int ignore_global);

void callstack_push(d_vm* vm, d_instructions* target, d_generic_var_table* module_env);

int callstack_pop(d_vm* vm);

int execute_d_function(d_vm* vm, drax_value a, drax_value v);

void __reset__(d_vm* vm);

int __start__(d_vm* vm, int inter_mode);

int __run__(d_vm* curr, int inter_mode);

int __irun__(d_vm* curr, int inter_mode);

#endif
