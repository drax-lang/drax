/* drax Lang - 2023
 * Jean Carlos (jeantux)
 */

 
#ifndef __DBUILTIN
#define __DBUILTIN

#include "dstructs.h"
#include "dvm.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<unistd.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>

typedef void (vm_builtin_setter) (d_vm* vm, const char* n, int a, low_level_callback* f);

void load_callback_fn(d_vm* vm, vm_builtin_setter* reg);

void create_native_modules(d_vm* vm);

#endif
