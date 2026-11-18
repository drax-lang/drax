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
#define D_COLOR_PURPLE "\033[35m"
#else
#define D_COLOR_RESET  ""
#define D_COLOR_RED    ""
#define D_COLOR_GREEN  ""
#define D_COLOR_YELLOW ""
#define D_COLOR_BLUE   ""
#define D_COLOR_PURPLE ""
#endif


void drax_print_error(const char* format, va_list args);

/* the text of an error (see Error.message), 'buf' is used when it is not a string */
const char* derror_text(drax_error* e, char* buf, size_t size);

/* raw: a string is written as it is, without quotes (only at the top level) */
void print_drax(drax_value value, int raw);

void dbreak_line();

#endif
