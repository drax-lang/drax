/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __DPRINT
#define __DPRINT

#include "dtypes.h"

#define BDEF  "\x1B[0m"
#define BBLU  "\x1B[34m"
#define BGRE  "\x1B[32m"
#define BRED  "\x1B[31m"
#define BMAG  "\x1b[35m"

void bshow_error(drax_state* curr);

void bprint_pack(drax_state* curr);

void bprint_list(drax_state* curr);

void bprint_expression(drax_state* curr);

void bprint(drax_state* curr);

void bprint_default(drax_state* curr, int sstr);

void bbreak_line();

void bspace_line();

#endif
