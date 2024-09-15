#ifndef __D_EVAL
#define __D_EVAL

#include <stdarg.h>
#include "dstructs.h"
#include "ddefs.h"

#ifdef _B_BUILF_FULL
#define D_COLOR_RESET  "\033[0m"
#define D_COLOR_RED    "\033[31m"
#define D_COLOR_GREEN  "\033[32m"
#define D_COLOR_YELLOW "\033[33m"
#define D_COLOR_BLUE   "\033[34m"
#else
#define D_COLOR_RESET  ""
#define D_COLOR_RED    ""
#define D_COLOR_GREEN  ""
#define D_COLOR_YELLOW ""
#define D_COLOR_BLUE   ""
#endif


void drax_print_error(const char* format, va_list args);

void print_drax(drax_value value, int formated);

void dbreak_line();

#endif
