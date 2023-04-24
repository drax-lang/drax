/* drax Lang - 2023
 * Jean Carlos (jeantux)
 */

 
#ifndef __DBUILTIN
#define __DBUILTIN

#include "dstructs.h"
#include "dvm.h"

typedef void (vm_builtin_setter) (d_vm* vm, const char* n, int a, low_level_callback* f);

void load_callback_fn(d_vm* vm, vm_builtin_setter* reg);

#endif
