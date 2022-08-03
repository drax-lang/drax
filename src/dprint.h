/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BPRINT
#define __BPRINT

#include "dtypes.h"

#define BDEF  "\x1B[0m"
#define BBLU  "\x1B[34m"
#define BGRE  "\x1B[32m"
#define BRED  "\x1B[31m"
#define BMAG  "\x1b[35m"

void bshow_error(beorn_state* curr);

void bprint_pack(beorn_state* curr);

void bprint_list(beorn_state* curr);

void bprint_expression(beorn_state* curr);

void bprint(beorn_state* curr);

void bprint_default(beorn_state* curr, int sstr);

void bbreak_line();

void bspace_line();

#endif
