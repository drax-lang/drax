/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BVM
#define __BVM

#include "dtypes.h"

#define is_call_fn(v) ((v->act != BACT_CALL_OP) && (v->act != BACT_CORE_OP))

drax_state* process_expression(drax_env* benv, drax_state* curr);

drax_state* process_symbol(drax_env* benv, drax_state* curr);

drax_state* process_list(drax_env* benv, drax_state* curr);

drax_state* process(drax_env* benv, drax_state* curr);

void __run_bs__(drax_env* benv, drax_state* curr, int inter_mode);

void __run__(drax_env* benv, drax_state* curr, int inter_mode);

#endif
