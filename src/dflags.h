/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __DFLAGS
#define __DFLAGS

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dtypes.h"
#include "dperm.h"

#define drax_VM_VERSION  "v0.0.1-dev"
#define drax_LIB_VERSION "LC-0.0.0-dev"

void initial_info();

/**
 * Reads the options (drax [options] [script]). The permission options go
 * to 'perms'; 'script' is the index in argv of the script, 0 when there is none.
 */
bimode get_bimode(int argc, char** argv, d_perms* perms, int* script);

#endif
