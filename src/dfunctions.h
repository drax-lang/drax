/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BFUN
#define __BFUN

#include <string.h>
#include "dtypes.h"

long double get_number(beorn_state* v);

beorn_state* do_op(beorn_env* benv, beorn_state* curr);

beorn_state* bb_typeof(beorn_env* benv, beorn_state* exp);

beorn_state* bb_set(beorn_env* benv, beorn_state* exp);

beorn_state* bb_register_function(beorn_env* benv, beorn_state* exp);

beorn_state* bb_lambda(beorn_env* benv, beorn_state* exp);

beorn_state* bb_fun(beorn_env* benv, beorn_state* exp);

beorn_state* bcall_runtime_function(beorn_env* benv, beorn_state* func, beorn_state* exp);

beorn_state* bb_let(beorn_env* benv, beorn_state* exp);

beorn_state* bb_concat(beorn_env* benv, beorn_state* exp);

beorn_state* bb_if(beorn_env* benv, beorn_state* exp);

beorn_state* bb_equal(beorn_env* benv, beorn_state* exp);

beorn_state* bb_double_equal(beorn_env* benv, beorn_state* exp);

beorn_state* bb_diff(beorn_env* benv, beorn_state* exp);

beorn_state* bb_less(beorn_env* benv, beorn_state* exp);

beorn_state* bb_bigger(beorn_env* benv, beorn_state* exp);

beorn_state* bb_less_equal(beorn_env* benv, beorn_state* exp);

beorn_state* bb_bigger_equal(beorn_env* benv, beorn_state* exp);

beorn_state* bb_double_diff(beorn_env* benv, beorn_state* exp);

beorn_state* bb_or(beorn_env* benv, beorn_state* exp);

beorn_state* bb_and(beorn_env* benv, beorn_state* exp);

beorn_state* bb_print(beorn_env* benv, beorn_state* exp);

beorn_state* bb_get(beorn_env* benv, beorn_state* exp);

beorn_state* bb_put(beorn_env* benv, beorn_state* exp);

beorn_state* bb_hd(beorn_env* benv, beorn_state* exp);

beorn_state* bb_tl(beorn_env* benv, beorn_state* exp);

void put_function_env(beorn_env** benv, const char* name, beorn_func fn);

beorn_state* bb_import(beorn_env* benv, beorn_state* exp);

beorn_env* get_main_env(beorn_env* benv);

int block_process(char* fun_n);

beorn_state* bcall_function(beorn_env* benv, beorn_state* exp);

void load_builtin_functions(beorn_env** benv);

beorn_state* bcall_native_function(beorn_env* benv, beorn_state* fun, beorn_state* exp);

#endif
