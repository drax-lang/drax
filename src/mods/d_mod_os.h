/* drax Lang - 2023
 * Jean Carlos (jeantux)
 */

 
#ifndef __D_MOD_OS
#define __D_MOD_OS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#include "../dvm.h"

char* replace_special_char(char* str);

int d_command(const char *command, char *output, int output_size);

#endif