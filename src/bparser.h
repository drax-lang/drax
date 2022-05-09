/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BPARSER
#define __BPARSER

#include "btypes.h"

typedef enum esm { 
  BP_NONE,
  BP_SIMPLE_DEFINITIONS,
  BP_FUNCTION_DEFINITION,
  BP_LAMBDA_DEFINITION,
} esm;

typedef struct bpsm {
  esm mode;
  int count;
} bpsm;

beorn_state* beorn_parser(char *input);

#define BAUTO_STATE_UPDATE(gs, tp, lenght) \
  if (gs->mode == tp) {                    \
  if (b->length == lenght) {               \
    b->closed = 1;                         \
    gs->mode = BP_NONE;                    \
  }                                        \
}

#endif
