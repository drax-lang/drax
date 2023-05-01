#include "dgc.h"

int dgc_swap(d_vm* vm) {
  d_struct *d = vm->d_ls;
  d_struct* u = NULL; /* Unused struct */
  d_struct* p = NULL;

  while (d != NULL) {
    if (d->checked) {
      u = d;
      d = d->next;
      p->next = d;
      free(u);
      continue;
    }

    p = d;
    d = d->next;
  }
  return 1;  
}