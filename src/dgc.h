/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __D_GC
#define __D_GC

#include <stdlib.h>
#include "dstructs.h"
#include "dvm.h"

/* runs a full cycle (mark and sweep) */
int dgc_swap(d_vm* vm);

/* frees every struct, used when the VM is destroyed */
void dgc_free_all(d_vm* vm);

/**
 * Runs a cycle when enough memory was allocated since the last one.
 * Must be called only at safe points: where every value in use is
 * reachable from the roots (see dgc.c).
 */
#ifdef DGC_STRESS
  /* collect on every safe point (tests: a missing root becomes a use after free) */
  #define dgc_maybe_collect(vm) dgc_swap(vm)
#else
  #define dgc_maybe_collect(vm) \
    do { \
      if ((vm)->gc_meta->bytes >= (vm)->gc_meta->next_gc) dgc_swap(vm); \
    } while (0)
#endif

#endif
