/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BPARSER
#define __BPARSER

#include "btypes.h"

typedef enum esm { 
  BP_NONE,
  BP_SIMPLE_DEFINITIONS,
  BP_FUNCTION_DEFINITION
} esm;

typedef struct bpsm {
  esm mode;
  int count;
} bpsm;

beorn_state* beorn_parser(char *input);

#endif
