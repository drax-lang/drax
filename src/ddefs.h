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

typedef uint8_t d_byte_def;

#endif