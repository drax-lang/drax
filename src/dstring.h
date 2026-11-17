/* drax Lang - 2023
 * Jean Carlos (jeantux)
 */

 
#ifndef __D_MOD_STRING
#define __D_MOD_STRING
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "dvm.h"
#include "dstructs.h"

#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200809L

char* strndup(const char *s, size_t n);

#endif

char* str_format_output(const char* str);

/**
 * Text of a number, the same for print and Number.to_string: integers with
 * no exponent (up to 1e21), anything else with the fewest digits that read
 * back as the same number (0.1 -> "0.1", 0.1 + 0.2 -> "0.30000000000000004").
 * out needs at least 32 bytes. Returns the length.
 */
int dnum_to_str(double n, char* out, size_t size);

drax_value dstr_to_number(d_vm* vm, int* stat);

drax_value dstr_split(d_vm *vm, int* stat);

drax_value dstr_length(d_vm* vm, int* stat);

drax_value dstr_copy(d_vm* vm, int* stat);

drax_value dstr_copy2(d_vm* vm, int* stat);

drax_value dstr_at(d_vm* vm, int* stat);

drax_value dstr_to_uppercase(d_vm* vm, int* stat);

drax_value dstr_to_lowercase(d_vm* vm, int* stat);

drax_value dstr_contains(d_vm* vm, int* stat);

int dstr_handle_str_call(d_vm* vm, char* n, int a, drax_value o);

#endif