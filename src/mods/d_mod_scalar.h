/* drax Lang - 2023
 * Jean Carlos (jeantux)
 */

 
#ifndef __D_MOD_SCALAR
#define __D_MOD_SCALAR

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include "../dvm.h"

drax_value __d_scalar_concat(d_vm* vm, int* stat);

drax_value __d_scalar_at(d_vm* vm, int* stat);

drax_value __d_scalar_head(d_vm* vm, int* stat);

drax_value __d_scalar_tail(d_vm* vm, int* stat);

drax_value __d_scalar_length(d_vm* vm, int* stat);

drax_value __d_scalar_is_empty(d_vm* vm, int* stat);

drax_value __d_scalar_is_present(d_vm* vm, int* stat);

drax_value __d_scalar_remove_at(d_vm* vm, int* stat);

drax_value __d_scalar_insert_at(d_vm* vm, int* stat);

drax_value __d_scalar_replace_at(d_vm* vm, int* stat);

drax_value __d_scalar_slice(d_vm* vm, int* stat);

drax_value __d_scalar_sum(d_vm* vm, int* stat);

drax_value __d_scalar_sparse(d_vm* vm, int* stat);


























#endif