#include <stdio.h>
#include "dgc.h"

/**
 * The safe free function remove safely the
 * value from the memory, checking if the value
 * is a drax value or a native value.
*/
static void dgc_safe_free(drax_value v) {
  if (v == 0) return;

  if (IS_NUMBER(v) || IS_BOOL(v) || IS_NIL(v)) return;

  /* check if is char* */

  if (IS_STRUCT(v)) {
    /**
     * If is checked then the value is used
    */
    d_struct* sct = CAST_STRUCT(v);
    if (sct->checked) return;

    DEBUG(printf("  -- dgc free\n"));
    if (IS_STRING(v)) {
      DEBUG(printf("    -- dgc string free\n"));
      drax_string* s = CAST_STRING(v);
      
      if (NULL != s->chars) {
        free(s->chars);
        s->chars = NULL;
      }
    } else if (IS_ERROR(v)) {
      DEBUG(printf("    -- dgc error free\n"));
      drax_error* e = CAST_ERROR(v);
      
      if (NULL != e->chars) {
        free(e->chars);
        e->chars = NULL;
      }
    } else if (IS_FRAME(v)) {
      DEBUG(printf("    -- dgc frame free\n"));
      drax_frame* f = CAST_FRAME(v);
      
      if (NULL != f->literals) {
        /*int i;
        for (i = 0; i < f->length; i++) {
          free(f->literals[i]);
        }*/
        free(f->keys);

        f->keys = NULL;
        f->literals = NULL;
      }
    } else if (IS_FUNCTION(v)) {
      DEBUG(printf("    -- dgc function free\n"));

      drax_function* ff = CAST_FUNCTION(v);
      free(ff->instructions->values);
      free(ff->instructions);
      free(ff->instructions->extrn_ref);
    }

    free(sct);
    sct = NULL;
  }
}

static void dgc_mark(drax_value v) {
  if (v == 0) return;

  if IS_STRUCT(v) {
    DEBUG(printf("marked\n"));
    CAST_STRUCT(v)->checked = 1;

    /* if is list mark all elements */
    if (IS_LIST(v)) {
      drax_list* l = CAST_LIST(v);
      int i;
      for (i = 0; i < l->length; i++) {
        dgc_mark(l->elems[i]);
      }
    } else if (IS_FRAME(v)) {
      drax_frame* f = CAST_FRAME(v);
      int i;
      for (i = 0; i < f->length; i++) {
        dgc_mark(f->values[i]);
      }
    } else if (IS_FUNCTION(v)) {
      drax_function* f = CAST_FUNCTION(v);
      int i;
      for (i = 0; i < f->instructions->instr_count; i++) {
        if (f->instructions->values[i] == v) continue;

        dgc_mark(f->instructions->values[i]);
      }
    }
  }
}

/**
 * Swap local variables
 * 
 * cycle through all elements from the final
 */
static int dgc_swap_locals(d_local_var_table* t) {
  DEBUG(printf("--dcloc swap\n"));
  if (t->count <= 0) return 1;

  int i;
  for (i = t->count; i > 0; i--) {
    if (t->array[i - 1] == NULL) continue;
    dgc_mark(t->array[i - 1]->value);
  }
  DEBUG(printf("--dcloc end\n\n"));
  return 1;
}

static int dgc_swap_generic_table(d_generic_var_table* t) {
  DEBUG(printf("--dcgl swap\n"));
  if (t->size <= 0) return 1;

  int i;
  for (i = 0; i < t->size; i++) {
    if (t->array[i] != NULL) {
      drax_generic_var_node* current = t->array[i];
      while (current != NULL) {
        drax_generic_var_node* next = current->next;
        dgc_mark(current->value);
        current = next;
      }
    }
  }
  DEBUG(printf("--dcgl end\n\n"));
  return 1;
}

static int dgc_swap_function(d_fun_table* t) {
  DEBUG(printf("--dcfn swap\n"));
  if (t->count <= 0) return 1;

  int i;
  for (i = 0; i < t->count; i++) {
    dgc_mark(t->pairs[i].value);
  }
  DEBUG(printf("--dcfn end\n\n"));
  return 1;
}

static int dgc_swap_stack(d_vm* vm) {
  DEBUG(printf("--dcstk swap\n"));
  if (vm->stack_count <= 0) return 1;

  int i;
  for (i = 0; i < vm->stack_count; i++) {
    dgc_mark(vm->stack[i]);
  }
  DEBUG(printf("--dcstk end\n\n"));
  return 1;
}

static int dgc_swap_native(d_fun_table* t) {
  DEBUG(printf("--dcnat swap\n"));
  if (t->count <= 0) return 1;

  int i;
  for (i = 0; i < t->count; i++) {
    dgc_mark(t->pairs[i].value);
  }
  DEBUG(printf("--dcnat end\n\n"));
  return 1;
}

static int dgc_swap_modules(d_mod_table* t) {
  DEBUG(printf("--dcmod swap\n"));
  if (t->count <= 0) return 1;

  int i;
  for (i = 0; i < t->count; i++) {
      dgc_mark(t->modules[i]);
  }
  DEBUG(printf("--dcmod end\n\n"));
  return 1;
}

int dgc_swap(d_vm* vm) {
  /**
   * disabled
  return 1;
   */
  DEBUG(printf("[GC] swap\n"));
  d_struct *d = vm->d_ls->next;
  d_struct* u = NULL; /* Unused struct */
  d_struct* p = vm->d_ls;

  dgc_swap_locals(vm->envs->local);

  dgc_swap_generic_table(vm->envs->global);

  dgc_swap_function(vm->envs->functions);

  dgc_swap_native(vm->envs->native);

  dgc_swap_modules(vm->envs->modules);
 
  dgc_swap_stack(vm);

  while (d != NULL) {
    if (d->checked) {
      d->checked = 0;
    } else {
      u = d;
      d = d->next;
      p->next = d;
      dgc_safe_free(DS_VAL(u));
      continue;
    }

    p = d;
    d = d->next;
  }
  return 1;  
}
