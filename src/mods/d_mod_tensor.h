/* drax Lang - 2024
 * Jean Carlos (jeantux)
 */

 
#ifndef __D_MOD_TENSOR
#define __D_MOD_TENSOR

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include "../dvm.h"

drax_value __d_tensor_concat(d_vm* vm, int* stat);

drax_value __d_tensor_at(d_vm* vm, int* stat);

drax_value __d_tensor_head(d_vm* vm, int* stat);

drax_value __d_tensor_tail(d_vm* vm, int* stat);

drax_value __d_tensor_length(d_vm* vm, int* stat);

drax_value __d_tensor_is_empty(d_vm* vm, int* stat);

drax_value __d_tensor_is_present(d_vm* vm, int* stat);

drax_value __d_tensor_remove_at(d_vm* vm, int* stat);

drax_value __d_tensor_insert_at(d_vm* vm, int* stat);

drax_value __d_tensor_replace_at(d_vm* vm, int* stat);

drax_value __d_tensor_slice(d_vm* vm, int* stat);

drax_value __d_tensor_sparse(d_vm* vm, int* stat);

drax_value __d_tensor_type(d_vm* vm, int* stat);
/**
 * Mathematical functions
 */
drax_value __d_tensor_sum(d_vm* vm, int* stat);

drax_value __d_tensor_add(d_vm* vm, int* stat);

#endif