/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BPRINT
#define __BPRINT

#include "btypes.h"

void bshow_error(beorn_state* curr);

void bprint_pack(beorn_state* curr);

void bprint_list(beorn_state* curr);

void bprint_expression(beorn_state* curr);

void bprint(beorn_state* curr);

#endif
