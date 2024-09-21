/* drax Lang - 2023
 * Jean Carlos (jeantux)
 */

 
#ifndef __DBUILTIN
#define __DBUILTIN

#include "dstructs.h"
#include "dvm.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#ifdef _WIN32
    #undef mkdir
#endif

typedef void (vm_builtin_setter) (d_vm* vm, const char* n, int a, low_level_callback* f);

void load_callback_fn(d_vm* vm, vm_builtin_setter* reg);

void create_native_modules(d_vm* vm);

#endif
