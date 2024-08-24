/* drax Lang - 2024
 * Jean Carlos (jeantux)
 */

 
#ifndef __D_MOD_HTTP
#define __D_MOD_HTTP

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#include "../dvm.h"

drax_value start_http_server(d_vm* vm, char *options[],
    void (*callback_caller)(d_vm* vm, drax_value call), drax_value call);

void stop_http_server(drax_value v);

#endif