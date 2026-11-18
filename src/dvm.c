#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#include "ddefs.h"
#include "dvm.h"
#include "dbuiltin.h"
#include "dtypes.h"
#include "dgc.h"
#include "deval.h"

#include "dstring.h"

#include "dparser.h"
#include "dio.h"

#include "doutopcode.h"

/* see the macros of the loop on dvm.h */
/**
 * 'lbase' is the first local slot of the frame of the function that runs:
 * it is calculated again when the frame changes (calls, return) or when
 * the table of locals can grow (a call).
 */
#define SYNC_IP()   (vm->ip = ip)
#define RELOAD_IP() \
  do { \
    ip = vm->ip; \
    lbase = vm->call_stack->count > 0 ? &vm->envs->local->array[CURR_FRAME(vm)->local_base] : NULL; \
  } while (0)
#define OPERAND()   (*(ip++))
#define OPERAND16() (ip += 2, (uint16_t) ((ip[-2] << 8) | ip[-1]))

/**
 * Direct OP on stack
 */
#define binary_op(vm, op) \
  do { \
    drax_value v2 = vm->stack[vm->stack_count - 1]; \
    drax_value v1 = vm->stack[vm->stack_count - 2]; \
    if (IS_NUMBER(v1) && IS_NUMBER(v2)) { \
      vm->stack_count--; \
      vm->stack[vm->stack_count - 1] = AS_VALUE(AS_NUMBER(v1) op AS_NUMBER(v2)); \
    } else { \
      SYNC_IP(); \
      raise_operand_error(vm, v1, v2, MSG_BAD_AGR_ARITH_OP); \
      return 1; \
    } \
  } while (0)

/**
 * Functions of the error paths stay out of the interpreter loop: the loop
 * has only the code of the happy path (registers, instruction cache).
 */
#if defined(__GNUC__) || defined(__clang__)
  #define D_COLD __attribute__((noinline, cold))
#else
  #define D_COLD
#endif

#define STATUS_DCALL_ERROR            0
#define STATUS_DCALL_SUCCESS          1
#define STATUS_DCALL_SUCCESS_NO_CLEAR 2

#define _L_MSG_NOT_DEF "error: '%s' is not defined\n"

/* VM stack. */

/**
 * The value stack grows on demand (limited by the memory),
 * the same way the stack of an Erlang process does.
 */
static void grow_stack(d_vm* vm) {
  int new_size = vm->stack_size * 2;
  drax_value* ns = (drax_value*) realloc(vm->stack, sizeof(drax_value) * new_size);

  if (ns == NULL) {
    fprintf(stderr, "fatal: out of memory to grow the stack.\n");
    exit(1);
  }

  vm->stack = ns;
  vm->stack_size = new_size;
}

void push(d_vm* vm, drax_value v) {
  if (vm->stack_count >= vm->stack_size) grow_stack(vm);
  vm->stack[vm->stack_count++] = v;
}

/**
 * push is called for almost every instruction: inside this file it is
 * expanded in place, with the growth of the stack out of the way.
 */
#if defined(__GNUC__) || defined(__clang__)
  #define D_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
  #define D_UNLIKELY(x) (x)
#endif

static inline void push_inline(d_vm* vm, drax_value v) {
  if (D_UNLIKELY(vm->stack_count >= vm->stack_size)) grow_stack(vm);
  vm->stack[vm->stack_count++] = v;
}

#define push(vm, v) push_inline(vm, v)

drax_value pop(d_vm* vm) {
  if (vm->stack_count == 0) return 0;
  vm->stack_count--;
  return vm->stack[vm->stack_count];
}

static inline drax_value pop_inline(d_vm* vm) {
  if (vm->stack_count == 0) return 0;
  vm->stack_count--;
  return vm->stack[vm->stack_count];
}

#define pop(vm) pop_inline(vm)

static drax_value pop_times(d_vm* vm, int t) {
  vm->stack_count -= t;
  return vm->stack[vm->stack_count];
}

static drax_value peek(d_vm* vm, int distance) {
  if (vm->stack_count == 0) return 0;
  return vm->stack[vm->stack_count -1 - distance];
}

D_COLD static void grow_call_stack(d_vm* vm) {
  int new_size = vm->call_stack->size * 2;
  d_frame* nf = (d_frame*) realloc(vm->call_stack->frames, sizeof(d_frame) * new_size);

  if (nf == NULL) {
    fprintf(stderr, "fatal: out of memory to grow the call stack.\n");
    exit(1);
  }

  vm->call_stack->frames = nf;
  vm->call_stack->size = new_size;
}

/* a call pushes a frame: this is expanded in place by the calls of the VM loop */
static inline void callstack_push_inline(d_vm* vm, d_instructions* target, d_generic_var_table* module_env) {
  if (D_UNLIKELY(vm->call_stack->count >= vm->call_stack->size)) grow_call_stack(vm);

  d_frame* frame = &vm->call_stack->frames[vm->call_stack->count++];
  frame->instr = vm->active_instr;
  frame->ip = vm->ip;
  frame->env_global = vm->envs->global;
  frame->stack_base = vm->stack_count;
  frame->local_base = vm->envs->local->count;
  frame->callee = DRAX_NIL_VAL;

  vm->active_instr = target;
  vm->ip = target->values;
  
  if (module_env != NULL) {
    vm->envs->global = module_env;
  }
}

void callstack_push(d_vm* vm, d_instructions* target, d_generic_var_table* module_env) {
  callstack_push_inline(vm, target, module_env);
}

int callstack_pop(d_vm* vm) {
  if (vm->call_stack->count == 0) return 1;

  d_frame* frame = &vm->call_stack->frames[--vm->call_stack->count];

  vm->active_instr = frame->instr;
  vm->ip = frame->ip;
  vm->envs->global = frame->env_global;

  return 0;
}

/**
 * Raise Helpers
*/

/* deep recursion can have millions of frames, only the top ones are printed */
#define MAX_TRACE_LINES 20

/* returns 1 when a line was printed */
static int print_trace_step(d_vm* vm, int* last_line) {
  d_instructions* instr = vm->active_instr;
  int printed = 0;
  if (instr == NULL) return 0;

  int idx = vm->ip - instr->values - 1;
  
  if (idx < 0 || idx >= instr->instr_count) return 0;

  if (instr->lines[idx] != 0 && instr->lines[idx] != *last_line) {
    printed = 1;
    if (vm->active_instr->file != NULL) {
      printf(TRACE_DESCRIPTION_LINE_PATH, vm->active_instr->file, instr->lines[idx]);
      putchar('\n');
    } else {
      fprintf(stderr, TRACE_DESCRIPTION_LINE, instr->lines[idx]);
      putchar('\n');
    }
  }
  *last_line = instr->lines[idx];
  return printed;
}

static void trace_error(d_vm* vm) {
  int last_line = -1;

  int printed = 0;

  while (CURR_CALLSTACK_SIZE(vm) > 0) {
    if (printed < MAX_TRACE_LINES) {
      printed += print_trace_step(vm, &last_line);
    } else {
      printf("  ... %d more frame(s)\n", CURR_CALLSTACK_SIZE(vm));
      return; /* __reset__ clears the remaining frames */
    }

    callstack_pop(vm);
  }
  
  print_trace_step(vm, &last_line);
}

/**
 * Reports a runtime error. The error ends the execution (the callers
 * return 1), unless a protected call (f?(...)) is waiting for it: then
 * nothing is printed, the message is kept and the failure travels back
 * (return 1, the same way) to the protected call, which turns it into an
 * Error value, see recover_protected and OP_D_CALL_S.
 */
void raise_drax_error(d_vm* vm, const char* format, ...) {
  va_list args;
  va_start(args, format);

  if (vm->protect_count > 0 || vm->soft_depth > 0) {
    /* the first error is the cause, the ones raised while it travels back are ignored */
    if (!vm->pending_error) {
      va_list copy;
      size_t n;
      int len;

      va_copy(copy, args);
      len = vsnprintf(NULL, 0, format, copy);
      va_end(copy);
      if (len < 0) len = 0;

      free(vm->pending_msg);
      vm->pending_msg = (char*) malloc((size_t) len + 1);
      if (vm->pending_msg == NULL) {
        fprintf(stderr, "fatal: out of memory to report an error.\n");
        exit(1);
      }
      vsnprintf(vm->pending_msg, (size_t) len + 1, format, args);
      n = strlen(vm->pending_msg);
      while (n > 0 && vm->pending_msg[n - 1] == '\n') vm->pending_msg[--n] = '\0';
      vm->pending_error = 1;
    }
    va_end(args);
    return;
  }

  drax_print_error(format, args);
  putchar('\n');
  va_end(args);

  trace_error(vm);
  __reset__(vm);
}

/**
 * An Error used as an operand (1 + Error(...), a call of an Error) ends
 * the execution with its own message, not with a generic one.
 */
D_COLD static void raise_operand_error(d_vm* vm, drax_value a, drax_value b, const char* generic) {
  drax_value e = IS_ERROR(a) ? a : (IS_ERROR(b) ? b : 0);

  if (e != 0) {
    char buf[64];
    raise_drax_error(vm, "%s", derror_text(CAST_ERROR(e), buf, sizeof(buf)));
    return;
  }

  raise_drax_error(vm, "%s", generic);
}

/* the pending error as an Error value (the message is its payload) */
static drax_value take_pending_error(d_vm* vm) {
  drax_string* s = copy_dstring(vm, vm->pending_msg, (int) strlen(vm->pending_msg));
  vm->pending_error = 0;
  free(vm->pending_msg);
  vm->pending_msg = NULL;
  return DS_VAL(new_derror_value(vm, DS_VAL(s)));
}

/* an error nobody handled: reports it like raise_drax_error does without a handler */
static void flush_pending_error(d_vm* vm) {
  char* msg = vm->pending_msg;

  vm->pending_msg = NULL;
  vm->pending_error = 0;
  vm->protect_count = 0;
  vm->soft_depth = 0;
  raise_drax_error(vm, "%s", msg);
  free(msg);
}

/**
 * Where the VM was before a protected call of a native function (or of
 * something that is not a function), to go back to it if the call fails.
 */
typedef struct d_snapshot {
  d_instructions* instr;
  drax_value* ip;
  d_generic_var_table* env_global;
  int frames;
  int stack_count;
  int local_count;
  int protect_count;
} d_snapshot;

static void snapshot_take(d_vm* vm, d_snapshot* s) {
  s->instr = vm->active_instr;
  s->ip = vm->ip;
  s->env_global = vm->envs->global;
  s->frames = vm->call_stack->count;
  s->stack_count = vm->stack_count;
  s->local_count = vm->envs->local->count;
  s->protect_count = vm->protect_count;
}

static void snapshot_restore(d_vm* vm, d_snapshot* s) {
  vm->active_instr = s->instr;
  vm->ip = s->ip;
  vm->envs->global = s->env_global;
  vm->call_stack->count = s->frames;
  vm->stack_count = s->stack_count;
  vm->envs->local->count = s->local_count;
  vm->protect_count = s->protect_count;
}

/**
 * A failure came back to this loop. If the innermost protected frame was
 * started by this loop, the frame (and everything above it) is dropped and
 * the protected call evaluates to an Error. The state is the one the
 * caller had, taken from the frame, so no work is needed on the happy path.
 *
 * A protected frame of an outer loop (the failure crossed a native
 * function or an import) is recovered by that loop.
 */
static int recover_protected(d_vm* vm) {
  d_frame* fr;
  d_prot* p;

  if (vm->protect_count == 0) return 0;

  /* the last entry is the innermost protected frame */
  p = &vm->prot_stack[vm->protect_count - 1];
  if (p->loop != vm->loop_depth) return 0;

  fr = &vm->call_stack->frames[p->frame];
  vm->protect_count--;
  vm->stack_count = fr->stack_base;
  vm->envs->local->count = fr->local_base;
  vm->active_instr = fr->instr;
  vm->ip = fr->ip;
  vm->envs->global = fr->env_global;
  vm->call_stack->count = p->frame;

  push(vm, take_pending_error(vm));
  return 1;
}

/* the frame on the top of the call stack was started by a protected call */
static void protect_top_frame(d_vm* vm) {
  if (vm->protect_count >= vm->prot_cap) {
    int cap = vm->prot_cap == 0 ? 16 : vm->prot_cap * 2;
    d_prot* np = (d_prot*) realloc(vm->prot_stack, sizeof(d_prot) * cap);

    if (np == NULL) {
      fprintf(stderr, "fatal: out of memory to grow the protected calls.\n");
      exit(1);
    }

    vm->prot_stack = np;
    vm->prot_cap = cap;
  }

  vm->prot_stack[vm->protect_count].frame = vm->call_stack->count - 1;
  vm->prot_stack[vm->protect_count].loop = vm->loop_depth;
  vm->protect_count++;
}

/* too_deep is set when lists/frames are nested deeper than MAX_VALUE_DEPTH */
static bool values_equal(drax_value a, drax_value b, int depth, bool* too_deep) {
  #define DV_EQS_AND_RETURN(_tp, _t1, _t2)\
    _tp* _v1##_tp = (_tp*) _t1->elems;\
    _tp* _v2##_tp = (_tp*) _t2->elems;\
    for(i = 0; i < _t1->length; i++) {\
      if (_v1##_tp[i] != _v2##_tp[i]) {  return false; }\
    }

  /**
   * numbers follow IEEE 754: -0 == 0 and nan != nan,
   * so they are compared before the identity check.
   */
  if (IS_NUMBER(a) && IS_NUMBER(b)) {
    return CAST_NUMBER(a) == CAST_NUMBER(b);
  }

  /**
   * lists and frames are always compared element by element,
   * a list with nan is not equal to itself (like the nan inside it).
   */
  if(a == b && !(IS_LIST(a) || IS_FRAME(a))) {
    return true;
  }

  if(!(IS_STRUCT(a) && IS_STRUCT(b))) {
    return false;  
  }

  if(DRAX_STYPEOF(a) != DRAX_STYPEOF(b)) {
    return false;
  }

  int i;
  switch(DRAX_STYPEOF(a)) {
    case DS_STRING:
      return CAST_STRING(a)->hash == CAST_STRING(b)->hash;
      
    case DS_LIST: {
      drax_list* l1 = CAST_LIST(a);
      drax_list* l2 = CAST_LIST(b);

      if(l1->length != l2->length) { return false; }

      if (depth >= MAX_VALUE_DEPTH) { *too_deep = true; return false; }

      for(i = 0; i < l1->length; i++) {
        if(!values_equal(l1->elems[i], l2->elems[i], depth + 1, too_deep)) {
          return false;
        }
      }
      return true;
    }

    case DS_FRAME: {
      drax_frame* f1 = CAST_FRAME(a);
      drax_frame* f2 = CAST_FRAME(b);

      if(f1->length != f2->length) { return false; }

      if (depth >= MAX_VALUE_DEPTH) { *too_deep = true; return false; }

      for(i = 0; i < f1->length; i++) {
        if(f1->keys[i] != f2->keys[i] || 
          !values_equal(f1->values[i], f2->values[i], depth + 1, too_deep)) {
          return false;
        }
      }
      return true;
    }


    case DS_ERROR: {
      drax_error* e1 = CAST_ERROR(a);
      drax_error* e2 = CAST_ERROR(b);

      /* the text of an error built by the VM equals Error('the same text') */
      if (e1->chars != NULL || e2->chars != NULL) {
        const char* t1 = e1->chars != NULL ? e1->chars : (IS_STRING(e1->payload) ? CAST_STRING(e1->payload)->chars : NULL);
        const char* t2 = e2->chars != NULL ? e2->chars : (IS_STRING(e2->payload) ? CAST_STRING(e2->payload)->chars : NULL);
        return t1 != NULL && t2 != NULL && strcmp(t1, t2) == 0;
      }

      if (depth >= MAX_VALUE_DEPTH) { *too_deep = true; return false; }
      return values_equal(e1->payload, e2->payload, depth + 1, too_deep);
    }

    case DS_FUNCTION:
    case DS_NATIVE:
    case DS_MODULE:
      break;

    default:
      break;
  }

  return false;
}

/**
 * clear new range of local variables
 * 
 * x                => used
 * 0                => not used
 * 
 * before:
 *  local->count    =>  (6)
 *  local->array    =>  |x|x|x|x|x|x|
 *  range           =>  (4)
 * 
 * after:
 *  local->array    =>  |x|x|x|x|x|x|0|0|0|0|
 *  local->count    =>  (10)
 * 
 */
void zero_new_local_range(d_vm* vm, int range) {
  if (!ensure_local_table_capacity(vm->envs->local, range)) {
    fprintf(stderr, "fatal: out of memory to grow the local stack.\n");
    exit(1);
  }

  vm->envs->local->count += range;
  memset(&vm->envs->local->array[vm->envs->local->count - range], 0, sizeof(d_local_var_node) * range);
}

/**
 * The slots of the frame of a call. A slot with key 0 is not set, so only
 * the keys are cleared (the table grows by the function above).
 *
 * Invariant: the value of a slot that is not set is stale (it may point to a
 * freed object). Every scan of the locals (GC roots, captures, snapshots)
 * must check 'key != 0' before reading 'value'.
 */
static inline void zero_locals_inline(d_vm* vm, int range) {
  d_local_var_table* l = vm->envs->local;
  d_local_var_node* n;
  int i;

  if (D_UNLIKELY(l->count + range > l->limit)) {
    zero_new_local_range(vm, range);
    return;
  }

  n = &l->array[l->count];
  l->count += range;
  for (i = 0; i < range; i++) n[i].key = 0;
}

/**
 * Copies to 'base' the name without the arity ("sum/2" -> "sum").
 * returns 0 if the name has no arity.
 */
static int name_without_arity(const char* k, char* base, size_t size) {
  const char* slash = strrchr(k, '/');
  if (slash == NULL || slash == k) return 0;

  size_t n = (size_t) (slash - k);
  if (n >= size) return 0;

  memcpy(base, k, n);
  base[n] = '\0';
  return 1;
}

/* the hashes of a name (see d_name_hdr), in one pass over the characters */
static void name_scan(const char* k, d_name_hdr* h) {
  uint32_t fk = D_FNV_OFFSET, bk = 0, fb = 0, bb = 0;
  int slash = -1, i;

  for (i = 0; k[i] != '\0'; i++) {
    if (k[i] == '/') { slash = i; fb = fk; bb = bk; }
    fk = d_fnv_step(fk, k[i]);
    bk = d_bucket_step(bk, k[i]);
  }

  h->magic = D_NAME_MAGIC;
  h->slash = slash;
  h->fk = fk;
  h->bk = bk;
  h->fb = fb;
  h->bb = bb;
}

/**
 * The hashes of a name used by an instruction: they were computed when
 * the name was created (dpool_strndup). 'tmp' is only used for a string that
 * is not from the pool.
 */
static inline const d_name_hdr* name_info(const char* k, d_name_hdr* tmp) {
  const d_name_hdr* h = ((const d_name_hdr*) k) - 1;
  if (D_UNLIKELY(h->magic != D_NAME_MAGIC)) {
    name_scan(k, tmp);
    return tmp;
  }
  return h;
}

/**
 * A local variable of the function that is running, by the hash of its
 * name. The VM reads the locals by slot, this is used to capture them when a
 * function is created inside of another one (see build_self_dep_fn).
 */
static int enclosing_local(d_vm* vm, uint32_t fnv, const char* name, drax_value* v) {
  d_instructions* instr = vm->active_instr;
  d_local_var_node* nodes;
  int i;

  if (vm->call_stack->count == 0 || instr->local_range == 0) return 0;

  nodes = &vm->envs->local->array[CURR_FRAME(vm)->local_base];

  for (i = 0; i < instr->local_range; i++) {
    d_name_hdr tmp;
    /* the hash selects the candidates, the name decides (hashes can collide) */
    if (nodes[i].key != 0 && instr->local_names[i] != NULL &&
        name_info(instr->local_names[i], &tmp)->fk == fnv &&
        strcmp(instr->local_names[i], name) == 0) {
      *v = nodes[i].value;
      return 1;
    }
  }

  return 0;
}

/**
 * Calls are compiled with the arity in the name ("sum/2"), so
 * functions with the same name and different arities can coexist,
 * like in Erlang.
 *
 * Order: modules, locals (they shadow the globals), global function
 * with the same arity, global name (variables, lambdas), natives.
 */
static int lookup_name(d_vm* vm, char* k, int is_local, drax_value* v) {
  d_name_hdr tmp;
  const d_name_hdr* h = name_info(k, &tmp);
  const int slash = h->slash;

  /* "name/arity" is a call of "name" (a name that is too long is not) */
  const int has_arity = slash > 0 && slash < 256;
  char base[256];

  /* the modules start with a few characters, the others skip this */
  if (d_mod_may_have(vm->envs->modules, k[0])) {
    char* plain = k;
    if (has_arity) {
      memcpy(base, k, (size_t) slash);
      base[slash] = '\0';
      plain = base;
    }

    if (get_mod_table(vm->envs->modules, plain, v)) {
      /**
       * a call with the name of a module is the native function with
       * the same name when there is one: Error(x) and Error.is(x)
       */
      if (has_arity) {
        drax_value nf = get_fun_table(vm->envs->native, plain, 0);
        if (nf) *v = nf;
      }
      return 1;
    }
  }

  if (is_local) {
    if (enclosing_local(vm, h->fk, k, v)) return 1;
    if (has_arity) {
      memcpy(base, k, (size_t) slash);
      base[slash] = '\0';
      if (enclosing_local(vm, h->fb, base, v)) return 1;
    }
  }

  if (get_var_table_h(vm->envs->global, h->bk, h->fk, v)) return 1;
  if (has_arity && get_var_table_h(vm->envs->global, h->bb, h->fb, v)) return 1;

  char* plain = k;
  if (has_arity) {
    memcpy(base, k, (size_t) slash);
    base[slash] = '\0';
    plain = base;
  }

  *v = get_fun_table(vm->envs->native, plain, 0);
  return *v ? 1 : 0;
}

static int get_definition(d_vm* vm) {
  char* k = (char*) GET_VALUE(vm);
  drax_value v;

  if (!lookup_name(vm, k, 0, &v)) {
    char base[256];
    raise_drax_error(vm, _L_MSG_NOT_DEF, name_without_arity(k, base, sizeof(base)) ? base : k);
    return 0;
  }

  push(vm, v);
  return 1;
}

/**
 * A local slot used while no function is running: the parser and the VM
 * disagree about the scope, so it is an error and not a write on NULL.
 */
D_COLD static int local_outside_frame(d_vm* vm) {
  raise_drax_error(vm, "runtime error: local variable used outside of a function\n");
  return 0;
}

/**
 * A local variable that was not assigned (it is assigned on another branch)
 * is looked up as a global name, like any name that is not local.
 */
D_COLD static int get_unset_local(d_vm* vm, int slot) {
  char* k = vm->active_instr->local_names[slot];
  drax_value v;

  if (!lookup_name(vm, k, 0, &v)) {
    char base[256];
    raise_drax_error(vm, _L_MSG_NOT_DEF, name_without_arity(k, base, sizeof(base)) ? base : k);
    return 0;
  }

  push(vm, v);
  return 1;
}

#define return_if_not_found_error(vm, v, n, a) \
  if (v == 0) { \
    raise_drax_error(vm, "error: function '%s/%d' is not defined\n", n, a); \
    return STATUS_DCALL_ERROR; \
  }

/**
 * import "your/path" as lib
 * 
 * this function process importations.
 */
#ifndef IMPORT_PATH_MAX
#define IMPORT_PATH_MAX 4096
#endif

/**
 * Import path rules (always the same):
 *
 *   import('./a.dx'), import('../a.dx')  relative to the importing file
 *   import('/abs/a.dx')                  absolute
 *   import('name'), import('dir/name')   looked up on the default
 *                                        'lib' directory
 *
 * The '.dx' extension is optional. The 'lib' directory is searched on
 * the directory of the entry script first, then on the current directory.
 */
static char entry_file[IMPORT_PATH_MAX] = "";

static int is_dir_path(const char* p) {
#ifdef _WIN32
  DWORD a = GetFileAttributesA(p);
  return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
#else
  struct stat st;
  return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

static int is_explicit_path(const char* s) {
  return s[0] == '/' || s[0] == '\\' ||
         (s[0] == '.' && (s[1] == '/' || s[1] == '\\' ||
         (s[1] == '.' && (s[2] == '/' || s[2] == '\\'))));
}

/* a file, the same with ".dx", or a package directory (its main.dx) */
static int try_resolve(const char* path, char* out) {
  size_t n = strlen(path);
  if (n + 9 >= IMPORT_PATH_MAX) return 0;

  int is_dir = 0;
  if (get_realpath(path, out)) {
    if (!is_dir_path(out)) return 1;
    is_dir = 1;
  }

  if (n < 3 || strcmp(path + n - 3, ".dx") != 0) {
    char with_ext[IMPORT_PATH_MAX];
    memcpy(with_ext, path, n);
    memcpy(with_ext + n, ".dx", 4);
    if (get_realpath(with_ext, out) && !is_dir_path(out)) return 1;
  }

  if (is_dir) {
    char main_file[IMPORT_PATH_MAX];
    memcpy(main_file, path, n);
    memcpy(main_file + n, "/main.dx", 9);
    if (get_realpath(main_file, out) && !is_dir_path(out)) return 1;
  }
  return 0;
}

static int resolve_import_path(d_vm* vm, const char* spec, char* out) {
  char* full;
  int ok = 0;

  if (is_explicit_path(spec)) {
    full = normalize_path(vm->active_instr->file, (char*) spec);
    ok = try_resolve(full, out);
    free(full);
    return ok;
  }

  /* default 'lib' directory */
  char lib_spec[IMPORT_PATH_MAX];
  if (strlen(spec) + 5 >= IMPORT_PATH_MAX) return 0;
  snprintf(lib_spec, sizeof(lib_spec), "lib/%s", spec);

  if (entry_file[0]) {
    full = normalize_path(entry_file, lib_spec);
    ok = try_resolve(full, out);
    free(full);
    if (ok) return 1;
  }
  full = normalize_path(NULL, lib_spec);
  ok = try_resolve(full, out);
  free(full);
  return ok;
}

static int import_file(d_vm* vm, drax_value v) {
  char * content = 0;

  if (!IS_STRING(v)) {
    raise_drax_error(vm, "import with unexpected type, the argument must be a string");
    return 1;
  }

  drax_string* p = CAST_STRING(v);

  /* resolve the canonical path first: it is the cache key */
  char resolved[IMPORT_PATH_MAX];
  if (!resolve_import_path(vm, p->chars, resolved)) {
    printf("file '%s' not found.\n", p->chars);
    return 1;
  }
  char* new_p;

  /* already imported (or being imported): no disk access needed */
  drax_value ivl;
  if (get_var_table(vm->imported_files, resolved, &ivl)) {
    push(vm, ivl);
    return 0;
  }

  if (get_file_content(NULL, resolved, &content)) {
    printf("file '%s' not found.\n", p->chars);
    return 1;
  }
  new_p = resolved;

  /**
   * fix infinity loop in secondary imports
   * a.file [import] -> (b.file [import] -> b.file)
   */
  put_var_table(vm->imported_files, new_p, DRAX_NIL_VAL);

  /**
   * the module runs with its own code and global scope, both
   * are released after the import: what the module exports is
   * kept by the imported_files table (a GC root).
   */
  d_instructions* module_code = new_instructions();
  d_generic_var_table* module_env = new_var_table();
  callstack_push(vm, module_code, module_env);

  int stat = 0;
  if (__build__(vm, content, new_p)) {
    /* the buffer of the module grows while it is built (realloc): the
     * pointer taken by callstack_push may be stale, a big module would
     * start on freed memory */
    vm->ip = module_code->values;
    stat = __irun__(vm, 0);
  } else {
    stat = 1;
  }
  free(content);

  if (stat == 0) {
    /**
     * Define exported value on global
     * 
     * vm->exported[0] -> is used only the 
     * first element, for now.
     */
    put_var_table(vm->imported_files, new_p, vm->exported[0]);
    callstack_pop(vm);
    push(vm, vm->exported[0]);
  } else if (vm->active_instr == module_code) {
    callstack_pop(vm);
  }

  if (stat != 0) {
    /* do not cache the placeholder of a failed import */
    del_var_table(vm->imported_files, new_p);
  }
  free_instructions(module_code);
  free_var_table(vm, module_env);
  return stat;
}

/**
 * build self-dependent lambda
 * 
 * generates a new function, solving variables 
 * outside the scope.
 */
/**
 * true when 'k' is a reference of the function to itself:
 * "name" or "name/arity".
 */
static int is_self_reference(drax_function* fn, const char* k) {
  if (fn->name == NULL) return 0;

  size_t n = strlen(fn->name);
  if (strncmp(fn->name, k, n) != 0) return 0;
  if (k[n] == '\0') return 1;
  if (k[n] != '/') return 0;

  return atoi(k + n + 1) == fn->arity;
}

static int build_self_dep_fn(d_vm* vm, drax_value* v) {
  drax_function* f = CAST_FUNCTION(*v);
  drax_function* new_fn = new_function(vm);
  int n_refs = f->instructions->extrn_ref_count;
  
  new_fn->name = f->name;
  new_fn->arity = f->arity;

  /**
   * The slots of the new function: the ones of the original function, and
   * one more for each external reference (the captured value).
   */
  const int base_range = f->instructions->local_range;

  new_fn->instructions->file = f->instructions->file;
  new_fn->instructions->extrn_ref_count = 0;
  new_fn->instructions->_ip = f->instructions->_ip;

  int i;

  /**
   * Resolve the references to names outside of the function.
   *
   * A name that is found is captured now (its value becomes a local
   * of the new function). A name that does not exist yet (a function
   * defined later in the file, mutual recursion) keeps OP_GET_G_ID and
   * is resolved when it is used (late binding, like in Erlang).
   */
  drax_value* values = (drax_value*) malloc(sizeof(drax_value) * (n_refs + 1));
  int* found = (int*) calloc(n_refs + 1, sizeof(int));
  int* slot_of = (int*) calloc(n_refs + 1, sizeof(int));
  int* is_first = (int*) calloc(n_refs + 1, sizeof(int)); /* owns its slot (the first of a name) */
  int n_slots = 0;

  for (i = 0; i < n_refs; i++) {
    d_extrn_ref r = f->instructions->extrn_ref[i];
    char* k = (char*) r.instr->values[r.pos + 1];

    if (r.instr->values[r.pos] != OP_GET_G_ID) {
      free(values);
      free(found);
      free(slot_of);
      free(is_first);
      raise_drax_error(vm, "runtime error: unspected OP on factory");
      return 0;
    }

    if (is_self_reference(new_fn, k)) {
      values[i] = DS_VAL(new_fn);
      found[i] = 1;
    } else {
      found[i] = lookup_name(vm, k, 1, &values[i]);
    }

    /**
     * Only the captured references take a slot, and a name that is used
     * more than once shares the slot of its first use.
     */
    if (found[i]) {
      int j;
      for (j = 0; j < i; j++) {
        if (found[j] && is_first[j] && strcmp((char*) f->instructions->extrn_ref[j].instr->values[f->instructions->extrn_ref[j].pos + 1], k) == 0) break;
      }

      if (j < i) {
        slot_of[i] = slot_of[j];
      } else {
        is_first[i] = 1;
        slot_of[i] = base_range + n_slots++;
      }
    }
  }

  const int new_range = base_range + n_slots;
  new_fn->instructions->local_range = new_range;
  if (new_range > 0) {
    new_fn->instructions->local_names = (char**) calloc((size_t) new_range, sizeof(char*));
    new_fn->instructions->local_names_cap = new_range;
    vm->gc_meta->bytes += sizeof(char*) * (size_t) new_range;
    if (base_range > 0) {
      memcpy(new_fn->instructions->local_names, f->instructions->local_names, sizeof(char*) * (size_t) base_range);
    }
  }

  /**
   * For each resolved "OP_GET_G_ID" operation, four more slots are
   * generated for commands, a new local definition in the prologue.
   * 
   * instructions before:
   * [ OP_GET_G_ID | drax_value ] 
   * 
   * instructions after:
   * [ OP_PUSH | drax_value | OP_SET_L_ID | slot | ... | OP_GET_L_ID | slot ]
   */
  new_fn->instructions->instr_count = f->instructions->instr_count + (n_slots * 4);
  
  if (new_fn->instructions->instr_size < new_fn->instructions->instr_count) {
    new_fn->instructions->instr_size = new_fn->instructions->instr_count + 1;

    new_fn->instructions->values = (drax_value*) realloc(
      new_fn->instructions->values,
      sizeof(drax_value) * (new_fn->instructions->instr_size + 1)
    );

    new_fn->instructions->lines = (int*) realloc(
      new_fn->instructions->lines,
      sizeof(int) * (new_fn->instructions->instr_size + 1)
    );
  }

  *v = DS_VAL(new_fn);

  int i_new = f->arity * 2;

  if (i_new > 0) {
    memcpy(
      new_fn->instructions->values,
      f->instructions->values,
      i_new * sizeof(drax_value)
    );
    memcpy(
      new_fn->instructions->lines,
      f->instructions->lines,
      i_new * sizeof(int)
    );
  }

  /* the captured values are defined on the line where the function starts */
  int first_line = f->instructions->instr_count > 0 ? f->instructions->lines[0] : 0;
  for (i = i_new; i < i_new + (n_slots * 4); i++) {
    new_fn->instructions->lines[i] = first_line;
  }

  for (i = 0; i < n_refs; i++) {
    if (!found[i] || !is_first[i]) continue;

    d_extrn_ref r = f->instructions->extrn_ref[i];
    new_fn->instructions->values[i_new++] = OP_PUSH;
    new_fn->instructions->values[i_new++] = values[i];
    new_fn->instructions->values[i_new++] = OP_SET_L_ID;
    new_fn->instructions->values[i_new++] = (drax_value) slot_of[i];
    new_fn->instructions->local_names[slot_of[i]] = (char*) r.instr->values[r.pos + 1];
  }

  /**
   * Copy instructions and update the resolved
   * OP_GET_G_ID to OP_GET_L_ID.
   */
  int j;
  int old_i = (f->arity * 2); 

  for (i = i_new; i < new_fn->instructions->instr_count; i++, old_i++) {
    new_fn->instructions->values[i] = f->instructions->values[old_i];
    new_fn->instructions->lines[i] = f->instructions->lines[old_i];

    for (j = 0; j < n_refs; j++) {
      d_extrn_ref r = f->instructions->extrn_ref[j];
      if (found[j] && r.instr == f->instructions && r.pos == old_i) {
        /* the name operand becomes the slot of the captured value */
        new_fn->instructions->values[i] = OP_GET_L_ID;
        new_fn->instructions->values[i + 1] = (drax_value) slot_of[j];
        new_fn->instructions->lines[i + 1] = f->instructions->lines[old_i + 1];
        i++;
        old_i++;
        break;
      }
    }
  }

  free(values);
  free(found);
  free(slot_of);
  free(is_first);
  return 1;
}

static void process_pipe_args(d_vm* vm, drax_value a) {
  /**
   * if the function is called via pipe to a 
   * sub-element [module.function()], we invert
   * the argument with the base element.
   * 
   * the base element must be inverted to not be 
   * considered an argument.
   */

  drax_value _t = vm->stack[vm->stack_count - a];
  vm->stack[vm->stack_count - a] = vm->stack[vm->stack_count -1 - a];
  vm->stack[vm->stack_count -1 - a] = _t;
}

static void remove_elem_on_stack(d_vm* vm, size_t distance) {
  if (distance == 0) {
    vm->stack_count--;
    return;
  }

  drax_value* tgt = &vm->stack[vm->stack_count - 1 - distance];
  drax_value* src = &vm->stack[vm->stack_count - distance];

  /**
   * most calls have a few arguments: no call to memmove for them. The
   * areas overlap (everything moves one place down), the order is ascending.
   */
  switch (distance) {
    case 1:
      tgt[0] = src[0];
      break;
    case 2:
      tgt[0] = src[0];
      tgt[1] = src[1];
      break;
    case 3:
      tgt[0] = src[0];
      tgt[1] = src[1];
      tgt[2] = src[2];
      break;
    default:
      memmove(tgt, src, sizeof(drax_value) * distance);
  }
  vm->stack_count--;
}

int execute_d_function(d_vm* vm, drax_value a, drax_value v) {
  if (IS_STRUCT(v)) {
    d_struct* s = (d_struct*) CAST_STRUCT(v);

    if (s->type == DS_FUNCTION) {
      drax_function* f = (drax_function*) s;
      
      if (f->arity != (int) a) {
        raise_drax_error(vm, MSG_FUNCTION_IS_NOT_DEFINED, f->name ? f->name : "<function>", a);
        return 1;
      }

      if (vm->call_stack->count >= vm->max_call_depth) {
        raise_drax_error(vm, MSG_STACK_OVERFLOW);
        return 1;
      }

      callstack_push_inline(vm, f->instructions, NULL);

      /**
       * the arguments are on the top of the stack and are
       * consumed by the function prologue (OP_SET_L_ID).
       */
      CURR_FRAME(vm)->stack_base = vm->stack_count - (int) a;
      CURR_FRAME(vm)->callee = v;
      
      if (f->instructions->local_range > 0) {
        zero_locals_inline(vm, f->instructions->local_range);
      }
      return 0;
    }

    if (s->type == DS_NATIVE) {
      drax_os_native* nf = (drax_os_native*) s;
      low_level_callback* native_fn = nf->function;

      if (nf->arity != (int) a) {
        /* Module.name is resolved by name only: another function of the
         * module with the same name may have the arity of this call */
        int ov = nf->module ? get_fun_on_module_arity(nf->module, nf->name, (int) a) : -1;
        if (ov == -1) {
          raise_drax_error(vm, MSG_FUNCTION_IS_NOT_DEFINED, nf->name ? nf->name : "<function>", a);
          return 1;
        }
        native_fn = nf->module->fun[ov];
      }

      int scs = 0;
      drax_value result = native_fn(vm, &scs);

      if (!scs) {
        /* an empty error was already reported */
        if (CAST_ERROR(result)->length > 0) {
          raise_drax_error(vm, "%s", CAST_ERROR(result)->chars);
        }
        return 1;
      }
      
      push(vm, result);
      return 0;
    }
  }

  if (IS_NIL(v)) {
    raise_drax_error(vm, "nil is not function!\n");
  } else if (IS_ERROR(v)) {
    raise_operand_error(vm, v, 0, "vm_error: function is not valid!\n");
  } else {
    raise_drax_error(vm, "vm_error: function is not valid!\n");
  }
  return 1;
}

/**
 * Protected call, f?(...): the function and its arguments are on the
 * stack. Returns 1 when the failure ends the program.
 */
D_COLD static int protected_call(d_vm* vm, drax_value a) {
  dgc_maybe_collect(vm);

  drax_value v = peek(vm, a);
  remove_elem_on_stack(vm, a);

  d_snapshot snap;
  snapshot_take(vm, &snap);
  /* a failed call leaves neither the function nor its arguments */
  snap.stack_count -= (int) a;

  vm->soft_depth++;
  int failed = execute_d_function(vm, a, v);
  vm->soft_depth--;

  if (failed) {
    /* without a pending error it was already reported: it ends the program */
    if (!vm->pending_error) return 1;

    snapshot_restore(vm, &snap);
    push(vm, take_pending_error(vm));
  } else if (IS_FUNCTION(v)) {
    /* a Drax function runs on its own frame, which is the protected one */
    protect_top_frame(vm);
  }

  return 0;
}

/* the interpreter loop, __start__ restarts it after a recovered failure */
static int __exec__(d_vm* vm, int inter_mode) {

  #define dbin_bool_op(op, v) \
      do { \
        if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) { \
          SYNC_IP(); \
          raise_operand_error(vm, peek(vm, 0), peek(vm, 1), MSG_BAD_AGR_ARITH_OP); \
          return 1; \
        } \
        double bbop = CAST_NUMBER(pop(vm)); \
        double abop = CAST_NUMBER(pop(vm)); \
        push(vm, v(abop op bbop)); \
      } while (false)

  UNUSED(inter_mode);

  drax_value a;
  drax_value* ip;
  d_local_var_node* lbase;

  RELOAD_IP();

  #ifdef _COMP_GOTO
  static void* dispatch_table[] = {
    [OP_CONST] = &&do_OP_CONST,
    [OP_NIL] = &&do_OP_NIL, 
    [OP_TRUE] = &&do_OP_TRUE,
    [OP_FALSE] = &&do_OP_FALSE,
    [OP_LIST] = &&do_OP_LIST,
    [OP_FRAME] = &&do_OP_FRAME,
    [OP_DSTR] = &&do_OP_DSTR,
    [OP_POP] = &&do_OP_POP,
    [OP_PUSH] = &&do_OP_PUSH,
    [OP_EQUAL] = &&do_OP_EQUAL,
    [OP_GREATER] = &&do_OP_GREATER,
    [OP_LESS] = &&do_OP_LESS,
    [OP_GREATER_EQUAL] = &&do_OP_GREATER_EQUAL,
    [OP_LESS_EQUAL] = &&do_OP_LESS_EQUAL,
    [OP_CONCAT] = &&do_OP_CONCAT,
    [OP_ADD] = &&do_OP_ADD,
    [OP_SUB] = &&do_OP_SUB,
    [OP_MUL] = &&do_OP_MUL,
    [OP_DIV] = &&do_OP_DIV,
    [OP_NOT] = &&do_OP_NOT,
    [OP_NEG] = &&do_OP_NEG,
    [OP_JMP] = &&do_OP_JMP,
    [OP_JMF] = &&do_OP_JMF,
    [OP_LOOP] = &&do_OP_LOOP,
    [OP_D_CALL] = &&do_OP_D_CALL,
    [OP_D_CALL_P] = &&do_OP_D_CALL_P,
    [OP_D_CALL_T] = &&do_OP_D_CALL_T,
    [OP_D_CALL_P_T] = &&do_OP_D_CALL_P_T,
    [OP_D_CALL_S] = &&do_OP_D_CALL_S,
    [OP_D_CALL_P_S] = &&do_OP_D_CALL_P_S,
    [OP_JMP_ERR] = &&do_OP_JMP_ERR,
    [OP_FUN] = &&do_OP_FUN,
    [OP_SET_G_ID] = &&do_OP_SET_G_ID,
    [OP_GET_G_ID] = &&do_OP_GET_G_ID,
    [OP_SET_L_ID] = &&do_OP_SET_L_ID,
    [OP_GET_L_ID] = &&do_OP_GET_L_ID,
    [OP_SET_I_ID] = &&do_OP_SET_I_ID,
    [OP_GET_I_ID] = &&do_OP_GET_I_ID,
    [OP_IMPORT] = &&do_OP_IMPORT,
    [OP_EXPORT] = &&do_OP_EXPORT,
    [OP_RETURN] = &&do_OP_RETURN,
    [OP_EXIT] = &&do_OP_EXIT,
  };
  #endif

  VMDispatch(vm) {
    VMcond(vm) {
      VMCase(OP_NIL) {
        push(vm, DRAX_NIL_VAL);
        NEXT_OP(vm);
      }
      VMCase(OP_TRUE) {
        push(vm, DRAX_TRUE_VAL);
        NEXT_OP(vm);
      }
      VMCase(OP_FALSE) {
        push(vm, DRAX_FALSE_VAL);
        NEXT_OP(vm);
      }
      VMCase(OP_LIST) {
        drax_value lc = pop(vm);
        int limit = (int) CAST_NUMBER(lc);
        drax_list* l = new_dlist(vm, limit);

        int i;
        for (i = 0; i < limit; i++) {
          put_value_dlist(l, peek(vm, (limit -1) - i));
        }

        pop_times(vm, limit);
        push(vm, DS_VAL(l));
        NEXT_OP(vm);
      }
      VMCase(OP_FRAME) {
        drax_value lc = pop(vm);
        int limit = (int) CAST_NUMBER(lc);
        drax_frame* l = new_dframe(vm, limit);

        int i;
        for (i = limit; i > 0; i-=2) {
          char* k = (char*) peek(vm, i - 1);
          put_value_dframe(l, k, peek(vm, i - 2));
        }

        pop_times(vm, limit);
        push(vm, DS_VAL(l));
        NEXT_OP(vm);
      }
      VMCase(OP_DSTR) {
        /* TODO: process db string */
        push(vm, OPERAND());
        NEXT_OP(vm);
      }
      VMCase(OP_CONST) {
        push(vm, OPERAND());
        NEXT_OP(vm);
      }
      VMCase(OP_POP) {
        pop(vm);
        NEXT_OP(vm);
      }
      VMCase(OP_PUSH) {
        push(vm, OPERAND());
        NEXT_OP(vm);
      }
      VMCase(OP_SET_G_ID) {
        drax_value v = pop(vm);
        char* k = (char*) OPERAND();
        d_name_hdr tmp;
        const d_name_hdr* h = name_info(k, &tmp);
        put_var_table_h(vm->envs->global, h->bk, h->fk, v);
        NEXT_OP(vm);
      }
      VMCase(OP_GET_G_ID) {
        SYNC_IP();
        if(get_definition(vm) == 0) { return 1; }
        RELOAD_IP();
        NEXT_OP(vm);
      }
      VMCase(OP_SET_L_ID) {
        /* the operand is the slot of the variable on the frame of the function */
        int slot = (int) OPERAND();
        d_local_var_node* node;
        if (D_UNLIKELY(lbase == NULL)) {
          SYNC_IP();
          local_outside_frame(vm);
          return 1;
        }
        node = &lbase[slot];
        node->key = 1; /* a slot with key 0 is not set */
        node->value = pop(vm);
        NEXT_OP(vm);
      }
      VMCase(OP_GET_L_ID) {
        int slot = (int) OPERAND();
        d_local_var_node* node;

        if (D_UNLIKELY(lbase == NULL)) {
          SYNC_IP();
          local_outside_frame(vm);
          return 1;
        }

        node = &lbase[slot];
        if (D_UNLIKELY(node->key == 0)) {
          /* not set (assigned on another branch): the name may be a global one */
          SYNC_IP();
          if (get_unset_local(vm, slot) == 0) { return 1; }
          NEXT_OP(vm);
        }

        push(vm, node->value);
        NEXT_OP(vm);
      }
      VMCase(OP_SET_I_ID) {
        /**
         * This operation is invalid, we cannot change values 
         * of structures such as frames and lists, as this will 
         * influence old references, resulting in side effects.
         */

        SYNC_IP();

        raise_drax_error(vm, "error: invalid assigment\nuse helper functions to change values of structures\n");
        return 1;
      }
      VMCase(OP_GET_I_ID) {
        drax_value k = pop(vm);
        drax_value f = pop(vm);
        drax_value val;

        if (IS_FRAME(f)) {
          if(get_value_dframe(CAST_FRAME(f), (char*) k, &val) == -1) {
            push(vm, DRAX_NIL_VAL);
            NEXT_OP(vm);
          }
          push(vm, val);
          NEXT_OP(vm);
        }

        if (IS_MODULE(f)) {
          drax_native_module* md = CAST_MODULE(f);
          int idx = get_fun_on_module(md, (const char*) k);

          if (idx == -1) {
            SYNC_IP();
            raise_drax_error(vm, "error: key '%s' is not defined\n", k);
            return 1;
          }

          low_level_callback* nf = md->fun[idx];
          int art = md->arity[idx];
          drax_os_native* mn = new_dllcallback(vm, nf, (const char*) k, art);
          mn->module = md;
          push(vm, DS_VAL(mn));
          NEXT_OP(vm);
        }
        
        SYNC_IP();
        
        raise_drax_error(vm, "error: key '%s' is not defined\n", k);
        return 1;
      }
      VMCase(OP_EQUAL) {
        drax_value beq = pop(vm);
        drax_value aeq = pop(vm);

        if (IS_NUMBER(aeq) && IS_NUMBER(beq)) {
          push(vm, BOOL_VAL(CAST_NUMBER(aeq) == CAST_NUMBER(beq)));
          NEXT_OP(vm);
        }

        bool too_deep = false;
        bool eq = values_equal(aeq, beq, 0, &too_deep);
        if (too_deep) {
          SYNC_IP();
          raise_drax_error(vm, "error: values nested deeper than %d levels can not be compared\n", MAX_VALUE_DEPTH);
          return 1;
        }
        push(vm, BOOL_VAL(eq));
        NEXT_OP(vm);
      }
      VMCase(OP_GREATER) {
        dbin_bool_op(>, BOOL_VAL);
        NEXT_OP(vm);
      }
      VMCase(OP_LESS) {
        dbin_bool_op(<, BOOL_VAL);
        NEXT_OP(vm);
      }
      VMCase(OP_GREATER_EQUAL) {
        /* not the same as !(a < b): every comparison with nan is false */
        dbin_bool_op(>=, BOOL_VAL);
        NEXT_OP(vm);
      }
      VMCase(OP_LESS_EQUAL) {
        dbin_bool_op(<=, BOOL_VAL);
        NEXT_OP(vm);
      }
      VMCase(OP_CONCAT) {
        if (IS_STRING(peek(vm, 0)) && IS_STRING(peek(vm, 1))) {
          drax_string* bstr = CAST_STRING(peek(vm, 0));
          drax_string* astr = CAST_STRING(peek(vm, 1));

          int length = astr->length + bstr->length;
          char* n_str = (char*) malloc(length + 1);
          memcpy(n_str, astr->chars, astr->length);
          memcpy(n_str + astr->length, bstr->chars, bstr->length);
          n_str[length] = '\0';

          drax_string* result = new_dstring(vm, n_str, length);
          pop_times(vm, 2);
          push(vm, DS_VAL(result));
        } else if (IS_LIST(peek(vm, 0)) && IS_LIST(peek(vm, 1))) {
          drax_list* bl = CAST_LIST(peek(vm, 0));
          drax_list* al = CAST_LIST(peek(vm, 1));
          int length = al->length + bl->length;
          drax_list* result = new_dlist(vm, length);
          memcpy(result->elems, al->elems, al->length * sizeof(drax_value));
          memcpy(result->elems + al->length, bl->elems, bl->length * sizeof(drax_value));

          result->length = length;
          pop_times(vm, 2);
          push(vm, DS_VAL(result));
        } else{
          SYNC_IP();
          raise_operand_error(vm, peek(vm, 0), peek(vm, 1), "Concat with unspected type");
          return 1;
        }
        NEXT_OP(vm);
      }
      VMCase(OP_ADD) {
        binary_op(vm, +);
        NEXT_OP(vm);
      }
      VMCase(OP_SUB) {
        binary_op(vm, -);
        NEXT_OP(vm);
      }
      VMCase(OP_MUL) {
        binary_op(vm, *);
        NEXT_OP(vm);
      }
      VMCase(OP_DIV) {
        binary_op(vm, /);
        NEXT_OP(vm);
      }
      VMCase(OP_NOT) {
        /**
         * Direct OP on stack
         */
        vm->stack[vm->stack_count - 1] = BOOL_VAL(IS_FALSE(vm->stack[vm->stack_count - 1]));
        NEXT_OP(vm);
      }
      VMCase(OP_NEG) {
        if (!IS_NUMBER(peek(vm, 0))) {
          SYNC_IP();
          raise_operand_error(vm, peek(vm, 0), 0, MSG_BAD_AGR_ARITH_OP);
          return 1;
        }
        
        push(vm, NUMBER_VAL(-CAST_NUMBER(pop(vm))));
        NEXT_OP(vm);
      }
      VMCase(OP_JMP) {
        uint16_t offset = OPERAND16();
        ip += offset;
        NEXT_OP(vm);
      }
      VMCase(OP_JMF) {
        uint16_t offset = OPERAND16();
        if (IS_FALSE(peek(vm, 0))) ip += offset;
        NEXT_OP(vm);
      }
      VMCase(OP_LOOP) {
        NEXT_OP(vm);
      }
      VMCase(OP_JMP_ERR) {
        /**
         * a |?> f(): when 'a' is an Error the call (the function and its
         * arguments) is skipped and the Error is the result.
         */
        uint16_t offset = OPERAND16();
        if (IS_ERROR(peek(vm, 0))) ip += offset;
        NEXT_OP(vm);
      }
      VMCase(OP_D_CALL_P_S) {
        a = OPERAND();
        process_pipe_args(vm, a);
        goto S_COMMON_CALL;
      }
      VMCase(OP_D_CALL_S) {
        /**
         * Protected call, f?(...): a failure inside the call (a runtime
         * error or an error of a native function) does not end the program,
         * the call evaluates to an Error. Never a tail call, the frame must
         * stay to catch the failure.
         */
        a = OPERAND();

        S_COMMON_CALL:
        SYNC_IP();
        if (protected_call(vm, a)) return 1;
        RELOAD_IP();
        NEXT_OP(vm);
      }
      VMCase(OP_D_CALL_P) {
        /**
         * Direct call
         * when the function is on stack
         */
        a = OPERAND();
        process_pipe_args(vm, a);
        goto L_COMMON_CALL;
      }
      VMCase(OP_D_CALL) {
        /**
         * Direct call
         * when the function is on stack
         */
        a = OPERAND();
        
        L_COMMON_CALL: {
          SYNC_IP();
          /* safe point: the function and the arguments are on the stack */
          dgc_maybe_collect(vm);

          drax_value v = peek(vm, a);
          remove_elem_on_stack(vm, a);

          if (execute_d_function(vm, a, v) == 1) {
            return 1;
          }
          RELOAD_IP();
        }
        NEXT_OP(vm);
      }
      VMCase(OP_D_CALL_P_T) {
        a = OPERAND();
        process_pipe_args(vm, a);
        goto P_T_COMMON_CALL;
      }
      VMCase(OP_D_CALL_T) {
        /**
         * Tail call (last call optimization).
         *
         * The call is the last thing the current function does, so
         * the current frame is reused: everything the function left
         * on the value stack and its locals are discarded and the
         * arguments are moved to the base of the frame.
         *
         * A loop written with tail recursion runs in constant memory.
         */
        a = OPERAND();
        P_T_COMMON_CALL: {
          SYNC_IP();
          dgc_maybe_collect(vm);

          drax_value v = peek(vm, a);
          drax_function* f;
          d_frame* fr;

          if (!IS_FUNCTION(v) || vm->call_stack->count == 0) {
            /**
             * natives and invalid values use the default
             * call, the next instruction is a OP_RETURN.
             */
            goto L_COMMON_CALL;
          }

          f = CAST_FUNCTION(v);
          if (f->arity != (int) a) {
            raise_drax_error(vm, MSG_FUNCTION_IS_NOT_DEFINED, f->name ? f->name : "<function>", a);
            return 1;
          }

          fr = CURR_FRAME(vm);

          memmove(
            &vm->stack[fr->stack_base],
            &vm->stack[vm->stack_count - a],
            sizeof(drax_value) * a
          );

          vm->stack_count = fr->stack_base + (int) a;
          vm->envs->local->count = fr->local_base;
          fr->callee = v;
          vm->active_instr = f->instructions;
          ip = f->instructions->values;
          vm->ip = ip; /* vm->ip never points into the code of another function */

          if (f->instructions->local_range > 0) {
            zero_locals_inline(vm, f->instructions->local_range);
          }

          /* the table of locals may have grown */
          lbase = &vm->envs->local->array[fr->local_base];
        }
        NEXT_OP(vm);
      }
      VMCase(OP_FUN) {
        /**
         * Check if is factory
         * 
         * go through all the instructions, if any are global, 
         * try to solve it (locally and then global) by replacing 
         * the values ​​in the instruction itself.
         * 
         * but perhaps these instructions need to be cloned somehow
         */

        drax_value v = OPERAND();
        drax_value extern_ref = OPERAND();
        if (extern_ref == DRAX_TRUE_VAL) {
          /**
           * This routine will replace the external
           * references on lambda.
           */
          SYNC_IP();
          if(build_self_dep_fn(vm, &v) == 0) { return 1; }
        }
        push(vm, v);
        NEXT_OP(vm);
      }
      VMCase(OP_IMPORT) {
        SYNC_IP();
        if (import_file(vm, pop(vm))) return 1;
        RELOAD_IP();
        NEXT_OP(vm);
      }
      VMCase(OP_EXPORT) {
        drax_value v = pop(vm);
        vm->exported[0] = v;
        NEXT_OP(vm);
      }
      VMCase(OP_RETURN) {
        if (vm->call_stack->count == 0) {
          /* return outside of a function */
          NEXT_OP(vm);
        }

        /**
         * The result is the top of the stack (nil if the
         * function did not produce a value), everything else
         * the function left on the stack is discarded.
         */
        d_frame* fr = CURR_FRAME(vm);
        drax_value result = (vm->stack_count > fr->stack_base) ?
          vm->stack[vm->stack_count - 1] : DRAX_NIL_VAL;

        if (vm->protect_count > 0 && vm->prot_stack[vm->protect_count - 1].frame == vm->call_stack->count - 1) {
          vm->protect_count--;
        }

        vm->stack_count = fr->stack_base;
        vm->envs->local->count = fr->local_base;
        callstack_pop(vm);
        RELOAD_IP();
        push(vm, result);

        if (!vm->active_instr) return 0;
        NEXT_OP(vm);
      }
      VMCase(OP_EXIT) {
        /* check if is stopVM or exit */

        if (inter_mode) {
          if (peek(vm, 0) == 0) return 0;

          print_drax(pop(vm), 0);
          dbreak_line();
        }
        return 0;
      }
      
      #ifndef _COMP_GOTO
        default: {
          printf("runtime error.\n");
          return 1;
        }
      #endif
    }
  }

  return 0;  
}

int __start__(d_vm* vm, int inter_mode) {
  int r;

  if (-1 == vm->vid) {
    if (vm->active_instr->file && *vm->active_instr->file) {
      put_var_table(vm->imported_files, vm->active_instr->file, DRAX_NIL_VAL);
      /* __start__ also runs for imported modules: keep only the first */
      if (!entry_file[0]) {
        snprintf(entry_file, sizeof(entry_file), "%s", vm->active_instr->file);
      }
    }
  }

  vm->loop_depth++;

  /**
   * A failure returns 1 from the loop (after raise_drax_error). When a
   * protected call is waiting for it, the state is restored and the
   * loop goes on from the instruction after the call.
   */
  while ((r = __exec__(vm, inter_mode)) != 0 && vm->pending_error && recover_protected(vm)) {}

  /* a pending error here has no handler left: it is reported now */
  if (r != 0 && vm->pending_error && vm->loop_depth == 1) {
    flush_pending_error(vm);
  }

  vm->loop_depth--;
  return r;
}

/* Constructor */

static void register_builtin(d_vm* vm, const char* n, int a, low_level_callback* f) {
  drax_value v = DS_VAL(new_dllcallback(vm, f, n, a));
  put_fun_table(vm->envs->native, v);
}

static void initialize_builtin_functions(d_vm* vm) {
  load_callback_fn(vm, &register_builtin);
}

dt_envs* new_environment(int ignore_natives, int ignore_local, int ignore_global) {
  dt_envs* e = (dt_envs*) malloc(sizeof(dt_envs));

  if (!ignore_global) {
    e->global = new_var_table();
  }

  if (!ignore_local) {
    e->local = new_local_table();
  }

  if (ignore_natives) return e;

  e->native = new_fun_table();
  e->modules = new_mod_table();
  return e;
}


/**
 * String pool
 *
 * Names created by the compiler are operands of instructions and
 * are shared by a function and its closures, so they are owned by
 * the VM and released by __destroy__.
 */
/* the pool owns 'base' (what free() takes) until the VM is destroyed */
static void dpool_push(d_vm* vm, char* base) {
  d_str_pool* p = vm->str_pool;

  if (p->count >= p->cap) {
    p->cap = p->cap == 0 ? 64 : p->cap * 2;
    p->items = (char**) realloc(p->items, sizeof(char*) * p->cap);
  }

  p->items[p->count++] = base;
}

char* dpool_add(d_vm* vm, char* s) {
  dpool_push(vm, s);
  return s;
}

/**
 * A copy of 's' for the pool, with the hashes of the name in front of it.
 * Every name that an instruction uses comes from here.
 */
char* dpool_strndup(d_vm* vm, const char* s, size_t n) {
  d_name_hdr* h = (d_name_hdr*) malloc(sizeof(d_name_hdr) + n + 1);
  char* r = (char*) (h + 1);

  memcpy(r, s, n);
  r[n] = '\0';
  name_scan(r, h);
  dpool_push(vm, (char*) h);
  return r;
}

/**
 * Releases all the memory of the VM.
 */
void __destroy__(d_vm* vm) {
  int i;
  if (vm == NULL) return;

  dgc_free_all(vm);
  free(vm->gc_meta);

  free_instructions(vm->instructions);

  free_var_table(vm, vm->envs->global);
  free_var_table(vm, vm->imported_files);

  free(vm->envs->local->array);
  free(vm->envs->local);

  free(vm->envs->native->pairs);
  free(vm->envs->native);

  free(vm->envs->modules->modules);
  free(vm->envs->modules);
  free(vm->envs);

  for (i = 0; i < vm->str_pool->count; i++) {
    free(vm->str_pool->items[i]);
  }
  free(vm->str_pool->items);
  free(vm->str_pool);

  free(vm->pending_msg);
  free(vm->prot_stack);
  free(vm->call_stack->frames);
  free(vm->call_stack);
  free(vm->stack);
  free(vm->exported);
  free(vm);
}

/**
 * Create a new drax vm
*/

static void __init__(d_vm* vm) {
  /**
   * vm->active_instr->values: Must always point to the first statement.
   *  - Used by stack trace.
   *  - Used to initiate a function call.
   */

  vm->ip = vm->active_instr->values;
}

void __reset__(d_vm* vm) { 
  free_instructions(vm->instructions);
  
  vm->instructions = new_instructions();
  vm->active_instr = vm->instructions;

  vm->call_stack->count = 0;
  vm->stack_count = 0;
  vm->envs->local->count = 0;
  vm->ip = NULL;
  vm->protect_count = 0;
  vm->pending_error = 0;
}

d_vm* createMainVM() {
  d_vm* vm = (d_vm*) malloc(sizeof(d_vm));
  vm->vid = -1;
  vm->instructions = new_instructions();
  vm->active_instr = vm->instructions;
  vm->d_ls = NULL;

  /**
   * Only one item is allowed to be 
   * exported, for now.
   */
  vm->exported = (drax_value*) malloc(sizeof(drax_value) * 1);
  vm->exported[0] = DRAX_NIL_VAL;

  vm->stack = (drax_value*) malloc(sizeof(drax_value) * MAX_STACK_SIZE);
  vm->stack_size = MAX_STACK_SIZE;
  vm->stack_count = 0;
  vm->envs = new_environment(0, 0, 0);

  vm->gc_meta = (dgc_meta*) malloc(sizeof(dgc_meta));
  vm->gc_meta->n_cycles = 0;
  vm->gc_meta->n_free_structs = 0;
  vm->gc_meta->bytes = 0;
  vm->gc_meta->next_gc = GC_INITIAL_THRESHOLD;
  vm->gc_meta->gray = NULL;
  vm->gc_meta->gray_count = 0;
  vm->gc_meta->gray_cap = 0;
  vm->imported_files = new_var_table();

  vm->str_pool = (d_str_pool*) malloc(sizeof(d_str_pool));
  vm->str_pool->items = NULL;
  vm->str_pool->count = 0;
  vm->str_pool->cap = 0;

  /**
   * Created on builtin definitions
  */
  create_native_modules(vm);

  initialize_builtin_functions(vm);

  vm->call_stack = (dcall_stack*) malloc(sizeof(dcall_stack));
  vm->call_stack->size = CALL_STACK_SIZE;
  vm->call_stack->count = 0;
  vm->call_stack->frames = (d_frame*) malloc(sizeof(d_frame) * CALL_STACK_SIZE);
  vm->pstatus = VM_STATUS_STOPED;

  /**
   * DRAX_MAX_CALL_DEPTH overrides the limit of non tail calls.
   */
  vm->native_nesting = 0;
  vm->prot_stack = NULL;
  vm->prot_cap = 0;
  vm->protect_count = 0;
  vm->soft_depth = 0;
  vm->loop_depth = 0;
  vm->pending_error = 0;
  vm->pending_msg = NULL;
  vm->max_call_depth = MAX_CALL_DEPTH;
  const char* env_depth = getenv("DRAX_MAX_CALL_DEPTH");
  if (env_depth != NULL) {
    long d = strtol(env_depth, NULL, 10);
    if (d > 0 && d <= INT32_MAX) vm->max_call_depth = (int) d;
  }

  return vm;
}

/**
 * creates a new virtual machine without starting 
 * the base modules and builtin functions, these 
 * are cloned into the main VM.
 *
 * but creates the base environments
 */

int __run__(d_vm* vm, int inter_mode) {
  vm->pstatus = VM_STATUS_WORKING;
  __init__(vm);

  DEBUG_OP(inspect_opcode(vm->ip, 0));

  int r = __start__(vm, inter_mode);
  vm->pstatus = VM_STATUS_STOPED;

  if (-1 == vm->vid) dgc_swap(vm);

  return r;
}

int __irun__(d_vm* vm, int inter_mode) {
  vm->pstatus = VM_STATUS_WORKING;

  DEBUG_OP(inspect_opcode(vm->ip, 0));

  int r = __start__(vm, inter_mode);
  vm->pstatus = VM_STATUS_STOPED;

  if (-1 == vm->vid) dgc_swap(vm);

  return r;
}
