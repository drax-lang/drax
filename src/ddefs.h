/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */
 
#ifndef __DDEFS
#define __DDEFS

#include <stdint.h>

#define UNUSED(x) (void)(x)

#define MAX_INSTRUCTIONS 80

#define STACK_OP_SIZE   256

#define MAX_STACK_SIZE (MAX_INSTRUCTIONS * STACK_OP_SIZE)

#define MAX_LOCAL_INSTRUCTIONS (MAX_INSTRUCTIONS * STACK_OP_SIZE)

#define CALL_STACK_SIZE (MAX_INSTRUCTIONS * STACK_OP_SIZE)

#define MAX_SIZE_TABLE 0.85

#define MODE_T_MAX ((mode_t)-1)

typedef uint8_t d_byte_def;

/**
 * Global Macros
 */

#define args_fail_required_size(a, b, msg) \
   if (a != b) { { raise_drax_error(vm, msg); return 0; } };

#define args_fail_required_bigger_than(a, b) if (a < b) { return 0; };

#define args_fail_required_less_than(a, b) if (a > b) { return 0; };

#define match_dfunction(n, v, f, vm, a, s) if ((strcmp(n, v) == 0)) { return f(vm, a, s); }

#endif