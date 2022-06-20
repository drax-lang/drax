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

char* append_char(const char *str, const char c);

beorn_state* new_definition();

beorn_state* new_parser_error(const char* msg);

int is_symbol(const char c);

int is_number(const char c);

int is_simple_expressions(const char* key);

esm keyword_to_bpsm(const char* key);

int apply_bpsm_state(bpsm* gs, esm s);

void auto_state_update(bpsm* gs, beorn_state* b);

int add_child(beorn_state* root, beorn_state* child);

int close_pending_structs(beorn_state* root, types ct);

beorn_state* beorn_parser(char *input);

#define BAUTO_STATE_UPDATE(gs, tp, lenght) \
  if (gs->mode == tp) {                    \
  if (b->length == lenght) {               \
    b->closed = 1;                         \
    gs->mode = BP_NONE;                    \
  }                                        \
}

#endif
