/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BVM
#define __BVM

#include "dtypes.h"

#define is_call_fn(v) ((v->act != BACT_CALL_OP) && (v->act != BACT_CORE_OP))

beorn_state* process_expression(beorn_env* benv, beorn_state* curr);

beorn_state* process_symbol(beorn_env* benv, beorn_state* curr);

beorn_state* process_list(beorn_env* benv, beorn_state* curr);

beorn_state* process(beorn_env* benv, beorn_state* curr);

void __run_bs__(beorn_env* benv, beorn_state* curr, int inter_mode);

void __run__(beorn_env* benv, beorn_state* curr, int inter_mode);

#endif
