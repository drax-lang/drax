/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __DFUN
#define __DFUN

#include <string.h>
#include "dtypes.h"

long double get_number(drax_state* v);

drax_state* do_op(drax_env* benv, drax_state* curr);

drax_state* bb_typeof(drax_env* benv, drax_state* exp);

drax_state* bb_set(drax_env* benv, drax_state* exp);

drax_state* bb_register_function(drax_env* benv, drax_state* exp);

drax_state* bb_lambda(drax_env* benv, drax_state* exp);

drax_state* bb_fun(drax_env* benv, drax_state* exp);

drax_state* bcall_runtime_function(drax_env* benv, drax_state* func, drax_state* exp);

drax_state* bb_let(drax_env* benv, drax_state* exp);

drax_state* bb_concat(drax_env* benv, drax_state* exp);

drax_state* bb_if(drax_env* benv, drax_state* exp);

drax_state* bb_equal(drax_env* benv, drax_state* exp);

drax_state* bb_double_equal(drax_env* benv, drax_state* exp);

drax_state* bb_diff(drax_env* benv, drax_state* exp);

drax_state* bb_less(drax_env* benv, drax_state* exp);

drax_state* bb_bigger(drax_env* benv, drax_state* exp);

drax_state* bb_less_equal(drax_env* benv, drax_state* exp);

drax_state* bb_bigger_equal(drax_env* benv, drax_state* exp);

drax_state* bb_double_diff(drax_env* benv, drax_state* exp);

drax_state* bb_or(drax_env* benv, drax_state* exp);

drax_state* bb_and(drax_env* benv, drax_state* exp);

drax_state* bb_print(drax_env* benv, drax_state* exp);

drax_state* bb_get(drax_env* benv, drax_state* exp);

drax_state* bb_put(drax_env* benv, drax_state* exp);

drax_state* bb_hd(drax_env* benv, drax_state* exp);

drax_state* bb_tl(drax_env* benv, drax_state* exp);

void put_function_env(drax_env** benv, const char* name, drax_func fn);

drax_state* bb_import(drax_env* benv, drax_state* exp);

drax_env* get_main_env(drax_env* benv);

int block_process(char* fun_n);

drax_state* bcall_function(drax_env* benv, drax_state* exp);

void load_builtin_functions(drax_env** benv);

drax_state* bcall_native_function(drax_env* benv, drax_state* fun, drax_state* exp);

#endif
