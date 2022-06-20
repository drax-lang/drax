/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BFUN
#define __BFUN

#include <string.h>
#include "btypes.h"

beorn_state* bpop(beorn_state* curr, int i);
beorn_state* bpush(beorn_state* curr);

long double get_number(beorn_state* v);

beorn_state* do_op(beorn_env* benv, beorn_state* curr);

beorn_state* bb_type_of(beorn_env* benv, beorn_state* exp);

beorn_state* bb_set(beorn_env* benv, beorn_state* exp);

beorn_state* bb_lambda(beorn_env* benv, beorn_state* exp);

beorn_state* bb_fun(beorn_env* benv, beorn_state* exp);

beorn_state* call_function_lambda(beorn_env* benv, beorn_state* func, beorn_state* exp);

beorn_state* bb_let(beorn_env* benv, beorn_state* exp);

beorn_state* bb_if(beorn_env* benv, beorn_state* exp);

beorn_state* bb_equal(beorn_env* benv, beorn_state* exp);

beorn_state* bb_diff(beorn_env* benv, beorn_state* exp);

void put_function_env(beorn_env** benv, const char* name, beorn_func fn);

beorn_env* get_main_env(beorn_env* benv);

beorn_state* call_func_builtin(beorn_env* benv, beorn_state* exp);

void load_buildtin_functions(beorn_env** benv);

beorn_state* call_func_native(beorn_env* benv, beorn_state* fun, beorn_state* exp);

#endif
