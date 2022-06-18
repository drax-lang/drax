/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BVM
#define __BVM

#include "btypes.h"

beorn_state* process_expression(beorn_env* benv, beorn_state* curr);

beorn_state* process_symbol(beorn_env* benv, beorn_state* curr);

beorn_state* process(beorn_env* benv, beorn_state* curr);

#endif
