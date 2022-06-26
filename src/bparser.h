/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BPARSER
#define __BPARSER

#include "btypes.h"

typedef enum esm { 
  BP_NONE,
  BP_DINAMIC,
  BP_SIMPLE_DEFINITIONS,
  BP_FUNCTION_DEFINITION,
  BP_LAMBDA_DEFINITION,
  BP_ONE_ARG,
  BP_TWO_ARG,
  BP_THREE_ARG,
  BP_FOUR_ARG,
} esm;

typedef struct bpsm {
  esm mode;
  int count;
} bpsm;

typedef struct stack_bpsm {
  bpsm** bpsm;
  int size;
} stack_bpsm;

/* state handler */
stack_bpsm* create_stack_bpsm();

int increment_stack_bpsm(stack_bpsm* gsb);

int del_first_stack_bpsm(stack_bpsm* gsb);

int add_elem_stack_bpsm(stack_bpsm* gsb);

/* helpers */

char* append_char(const char *str, const char c);

beorn_state* new_definition();

beorn_state* new_parser_error(const char* msg);

int is_symbol(const char c);

int is_number(const char c);

int is_simple_expressions(const char* key);

esm keyword_to_bpsm(const char* key);

int initialize_new_state(stack_bpsm* gs, esm s);

int bauto_state_update(stack_bpsm* gs, beorn_state* b, esm tp, int lenght);

void auto_state_update(stack_bpsm* gs, beorn_state* b);

int add_child(stack_bpsm* gs, beorn_state* root, beorn_state* child);

int close_pending_structs(stack_bpsm* gs, beorn_state* root, types ct);

beorn_state* beorn_parser(char *input);

#endif
