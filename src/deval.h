#ifndef __D_EVAL
#define __D_EVAL

#include <stdarg.h>
#include "dstructs.h"
#include "ddefs.h"

void drax_print_error(const char* format, va_list args);

void print_drax(drax_value value, int formated);

void dbreak_line();

#endif
