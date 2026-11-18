#include <stdio.h>
#include "dgc.h"

/**
 * Garbage collector (mark and sweep).
 *
 * Every struct (string, list, frame, function, ...) is on the list
 * vm->d_ls. A cycle marks everything reachable from the roots and frees
 * the rest. It runs automatically (see dgc_maybe_collect) and can be
 * started with Core.gc_swap().
 *
 * Roots: value stack, locals, globals (current and of every frame),
 * natives, modules, the exported value, imported modules, the function
 * running in each frame and the instructions being executed.
 *
 * Marking uses an explicit stack (gray list) instead of recursion, so
 * deeply nested data (a list inside a list, a million times) does not
 * overflow the C stack.
 */

/* Mark */

static void gray_push(d_vm* vm, drax_value v) {
  dgc_meta* m = vm->gc_meta;

  if (m->gray_count >= m->gray_cap) {
    int new_cap = m->gray_cap == 0 ? 256 : m->gray_cap * 2;
    drax_value* ng = (drax_value*) realloc(m->gray, sizeof(drax_value) * new_cap);

    if (ng == NULL) {
      fprintf(stderr, "fatal: out of memory on garbage collector.\n");
      exit(1);
    }

    m->gray = ng;
    m->gray_cap = new_cap;
  }

  m->gray[m->gray_count++] = v;
}

static void dgc_mark(d_vm* vm, drax_value v) {
  if (v == 0 || !IS_STRUCT(v)) return;

  d_struct* s = CAST_STRUCT(v);
  if (s->checked) return;

  s->checked = 1;
  gray_push(vm, v);
}

/**
 * Instructions keep constants (strings, functions) between the
 * operations. Raw operands (names, opcodes) are not structs and
 * are ignored by dgc_mark.
 *
 * Marks the constants of a block from 'ip' to the end.
 *
 * Used for the code being executed. The top level code (of the main
 * file and of an imported module) only jumps forward, so a constant
 * before 'ip' is never used again. The code of a function can run
 * again, but the function itself is marked (it is the callee of a
 * frame), which marks all of its instructions.
 *
 * A block can be asked many times in a cycle (it is the caller of every
 * frame of a recursion, and it is marked again as the code of a function):
 * the part that was already marked in this cycle is skipped, see gc_epoch
 * and gc_from.
 */
static void dgc_mark_instructions_from(d_vm* vm, d_instructions* instr, drax_value* ip) {
  if (instr == NULL || instr->values == NULL || ip == NULL) return;

  int start = (int) (ip - instr->values);
  if (start < 0 || start > instr->instr_count) start = 0;

  /**
   * A block that is the caller on many frames (a recursion) is read once
   * for each cycle: only the part that was not read yet is read now.
   */
  int end = instr->instr_count;
  const unsigned epoch = vm->gc_meta->epoch;

  if (instr->gc_epoch == epoch) {
    if (start >= instr->gc_from) return;
    end = instr->gc_from;
  }

  instr->gc_epoch = epoch;
  instr->gc_from = start;

  int i;
  for (i = start; i < end; i++) {
    dgc_mark(vm, instr->values[i]);
  }
}

static void dgc_mark_children(d_vm* vm, drax_value v) {
  int i;

  switch (DRAX_STYPEOF(v)) {
    case DS_LIST: {
      drax_list* l = CAST_LIST(v);
      for (i = 0; i < l->length; i++) {
        dgc_mark(vm, l->elems[i]);
      }

      if (l->owner != NULL) {
        dgc_mark(vm, DS_VAL(l->owner));
      }
      break;
    }

    case DS_FRAME: {
      drax_frame* f = CAST_FRAME(v);
      for (i = 0; i < f->length; i++) {
        dgc_mark(vm, f->values[i]);
      }
      break;
    }

    case DS_FUNCTION:
      {
        d_instructions* code = CAST_FUNCTION(v)->instructions;
        /* all of the code: a function can run again from the start */
        if (code != NULL) dgc_mark_instructions_from(vm, code, code->values);
      }
      break;

    case DS_ERROR:
      dgc_mark(vm, CAST_ERROR(v)->payload);
      break;

    default:
      break;
  }
}

static void dgc_drain(d_vm* vm) {
  dgc_meta* m = vm->gc_meta;

  while (m->gray_count > 0) {
    drax_value v = m->gray[--m->gray_count];
    dgc_mark_children(vm, v);
  }
}

static void dgc_mark_var_table(d_vm* vm, d_generic_var_table* t) {
  if (t == NULL) return;

  /* the same table is the environment of every frame of a recursion */
  if (t->gc_epoch == vm->gc_meta->epoch) return;
  t->gc_epoch = vm->gc_meta->epoch;

  int i;
  for (i = 0; i < t->size; i++) {
    drax_generic_var_node* current = t->array[i];
    while (current != NULL) {
      dgc_mark(vm, current->value);
      current = current->next;
    }
  }
}

static void dgc_mark_roots(d_vm* vm) {
  int i;

  /* value stack */
  for (i = 0; i < vm->stack_count; i++) {
    dgc_mark(vm, vm->stack[i]);
  }

  /* locals */
  d_local_var_table* l = vm->envs->local;
  for (i = 0; i < l->count; i++) {
    if (l->array[i].key != 0) dgc_mark(vm, l->array[i].value);
  }

  /* globals, natives and modules */
  dgc_mark_var_table(vm, vm->envs->global);

  for (i = 0; i < vm->envs->native->count; i++) {
    dgc_mark(vm, vm->envs->native->pairs[i].value);
  }

  for (i = 0; i < vm->envs->modules->count; i++) {
    dgc_mark(vm, vm->envs->modules->modules[i]);
  }

  /* exported value and imported modules */
  if (vm->exported != NULL) dgc_mark(vm, vm->exported[0]);
  dgc_mark_var_table(vm, vm->imported_files);

  /* code being executed */
  dgc_mark_instructions_from(vm, vm->active_instr, vm->ip);

  for (i = 0; i < vm->call_stack->count; i++) {
    d_frame* frame = &vm->call_stack->frames[i];
    dgc_mark(vm, frame->callee);
    dgc_mark_instructions_from(vm, frame->instr, frame->ip);
    dgc_mark_var_table(vm, frame->env_global);
  }

  dgc_drain(vm);
}

/* Sweep */

static size_t dgc_size_of(d_struct* d) {
  switch (d->type) {
    case DS_LIST: {
      drax_list* l = (drax_list*) d;
      return sizeof(drax_list) + (l->owner == NULL ? sizeof(drax_value) * l->cap : 0);
    }
    case DS_STRING:
      return sizeof(drax_string) + ((drax_string*) d)->length + 1;
    case DS_ERROR:
      return sizeof(drax_error) + ((drax_error*) d)->length + 1;
    case DS_FRAME:
      return sizeof(drax_frame) + (sizeof(drax_value) + sizeof(char*) + sizeof(int) + 16) * ((drax_frame*) d)->cap;
    case DS_FUNCTION: {
      d_instructions* i = ((drax_function*) d)->instructions;
      return sizeof(drax_function) + sizeof(d_instructions) + i->instr_size * (sizeof(drax_value) + sizeof(int))
           + sizeof(char*) * (size_t) i->local_names_cap + D_FUNCTION_BLOCKS * D_MALLOC_OVERHEAD;
    }
    case DS_NATIVE:
      return sizeof(drax_os_native);
    case DS_MODULE:
      return sizeof(drax_native_module);
    case DS_TID:
      return sizeof(drax_tid);
  }

  return sizeof(d_struct);
}

void free_instructions(d_instructions* instr) {
  if (instr == NULL) return;

  free(instr->values);
  free(instr->lines);
  free(instr->extrn_ref);
  free(instr->local_names);
  free(instr);
}

/**
 * Frees a struct and everything it owns.
 *
 * Names and file paths used by functions belong to the string
 * pool of the VM (shared by a function and its closures).
 */
static void dgc_free_struct(d_struct* d) {
  int i;

  switch (d->type) {
    case DS_STRING:
      free(((drax_string*) d)->chars);
      break;

    case DS_ERROR:
      free(((drax_error*) d)->chars);
      break;

    case DS_LIST: {
      drax_list* l = (drax_list*) d;
      /* a list that shares the buffer of another one does not own it */
      if (l->owner == NULL) free(l->elems);
      break;
    }

    case DS_FRAME: {
      drax_frame* f = (drax_frame*) d;
      for (i = 0; i < f->length; i++) {
        free(f->literals[i]);
      }
      free(f->literals);
      free(f->keys);
      free(f->values);
      break;
    }

    case DS_FUNCTION:
      free_instructions(((drax_function*) d)->instructions);
      break;

    case DS_MODULE: {
      drax_native_module* m = (drax_native_module*) d;
      free(m->arity);
      free(m->fun);
      free(m->fn_names);
      break;
    }

    case DS_TID: {
      drax_tid* t = (drax_tid*) d;
      if (t->destroy != NULL) t->destroy(t->value);
      break;
    }

    case DS_NATIVE:
      break;
  }

  free(d);
}

/**
 * Frees every struct that is not marked and clears the marks.
 * returns the memory still in use.
 */
static size_t dgc_sweep(d_vm* vm) {
  size_t live = 0;
  d_struct** link = &vm->d_ls;

  while (*link != NULL) {
    d_struct* d = *link;

    if (d->checked) {
      d->checked = 0;
      live += dgc_size_of(d);
      link = &d->next;
    } else {
      *link = d->next;
      dgc_free_struct(d);
      vm->gc_meta->n_free_structs++;
    }
  }

  return live;
}

/**
 * The number of a cycle, unique in the process: the stamps (gc_epoch) are in
 * tables and blocks of code that more than one VM can see, and a stamp of
 * another VM must never look like the stamp of the running cycle. 0 is
 * reserved for "never marked".
 */
static unsigned dgc_next_epoch(void) {
  static unsigned counter = 0;
  unsigned e;

#if defined(__GNUC__)
  e = __sync_add_and_fetch(&counter, 1u);
  if (e == 0) e = __sync_add_and_fetch(&counter, 1u);
#else
  e = ++counter;
  if (e == 0) e = ++counter;
#endif

  return e;
}

int dgc_swap(d_vm* vm) {
  vm->gc_meta->n_cycles++;
  vm->gc_meta->epoch = dgc_next_epoch();

  dgc_mark_roots(vm);
  size_t live = dgc_sweep(vm);

  /* the next cycle runs when the memory in use doubles */
  vm->gc_meta->bytes = 0;
  vm->gc_meta->next_gc = live * 2;
  if (vm->gc_meta->next_gc < GC_INITIAL_THRESHOLD) {
    vm->gc_meta->next_gc = GC_INITIAL_THRESHOLD;
  }

  return 1;
}

void dgc_free_all(d_vm* vm) {
  d_struct* d = vm->d_ls;

  while (d != NULL) {
    d_struct* next = d->next;
    dgc_free_struct(d);
    d = next;
  }

  vm->d_ls = NULL;
  free(vm->gc_meta->gray);
  vm->gc_meta->gray = NULL;
  vm->gc_meta->gray_count = 0;
  vm->gc_meta->gray_cap = 0;
}
