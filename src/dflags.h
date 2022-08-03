/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BFLAGS
#define __BFLAGS

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dtypes.h"

#define BEORN_VM_VERSION  "v0.0.1-dev"
#define BEORN_LIB_VERSION "LC-0.0.0-dev"

void initial_info();

void version_app();

int non_flag(char * name);

int argcmp(char sname, const char * name, char * arg);

bimode get_bimode(int argc, char** argv);

#endif
