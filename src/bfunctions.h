/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __FUN
#define __FUN

#include <string.h>
#include "btypes.h"

beorn_state* bpop(beorn_state* curr, int i);
beorn_state* bpush(beorn_state* curr);

beorn_state* do_op(beorn_env* benv, beorn_state* curr);

beorn_state* bb_type_of(beorn_env* benv, beorn_state* exp);

void load_buildtin_functions(beorn_env** benv);

beorn_state* call_func_builtin(beorn_env* benv, beorn_state* exp);

#endif
