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
    if (CAST_STRUCT(v)->checked) return;

    DEBUG(printf("  --dgc free\n"));
    if (IS_STRING(v)) {
      drax_string* s = CAST_STRING(v);
      free(s->chars);
      free(s);
    } else if (IS_LIST(v)) {
      drax_list* l = CAST_LIST(v);
      int i;
      for (i = 0; i < l->length; i++) {
        dgc_safe_free(l->elems[i]);
      }
      free(l);
    } else if (IS_FRAME(v)) {
      drax_frame* f = CAST_FRAME(v);
      int i;
      for (i = 0; i < f->length; i++) {
        /**
         * listerals is a char** currently.
         */
        free(f->literals[i]);
        dgc_safe_free(f->values[i]);
      }
      free(f);
    }
  }
}

/**
 * Swap local variables
 * 
 * cycle through all elements from the final 
 * peak to the last unused value.
 */
int dgc_swap_locals(d_local_var_table* t) {
  DEBUG(printf("--dcl swap\n"));
  if (t->peak <= 1) return 1;

  int i;
  for (i = t->peak; i >= 0 && i > t->count; i--) {
    /* DEBUG(printf("  --gcl idx[%d] key[%ld]\n", i, t->array[i -1]->key)); */
    /*
    if (IS_STRUCT(t->array[i -1]->value)) {
      CAST_STRUCT(t->array[i -1]->value)->checked = 1;
    }
    */
    t->peak--;
  }
  DEBUG(printf("--dcl end\n\n"));
  return 1;
}

int dgc_swap(d_vm* vm) {
  d_struct *d = vm->d_ls;
  d_struct* u = NULL; /* Unused struct */
  d_struct* p = NULL;

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