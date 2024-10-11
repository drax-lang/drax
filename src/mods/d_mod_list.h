/* drax Lang - 2024
 * Jean Carlos (jeantux)
 */

 
#ifndef __D_MOD_LIST
#define __D_MOD_LIST

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "../dvm.h"

drax_value __d_list_at(d_vm* vm, int* stat);

drax_value __d_list_concat(d_vm* vm, int* stat);

drax_value __d_list_head(d_vm* vm, int* stat);

drax_value __d_list_tail(d_vm* vm, int* stat);

drax_value __d_list_length(d_vm* vm, int* stat);

drax_value __d_list_is_empty(d_vm* vm, int* stat);

drax_value __d_list_is_present(d_vm* vm, int* stat);

drax_value __d_list_remove_at(d_vm* vm, int* stat);

drax_value __d_list_insert_at(d_vm* vm, int* stat);

drax_value __d_list_replace_at(d_vm* vm, int* stat);

drax_value __d_list_slice(d_vm* vm, int* stat);

drax_value __d_list_sum(d_vm* vm, int* stat);

drax_value __d_list_sparse(d_vm* vm, int* stat);

drax_value __d_list_hypot(d_vm* vm, int* stat);

drax_value __d_list_dot(d_vm* vm, int* stat);

drax_value __d_list_pop(d_vm* vm, int* stat);

drax_value __d_list_shift(d_vm* vm, int* stat);

drax_value __d_list_zip(d_vm* vm, int* stat);

#endif