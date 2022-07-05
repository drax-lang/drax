/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BPARSER
#define __BPARSER

#include "btypes.h"
#include "blex.h"

/* alias handler */
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

typedef enum b_operator {
    BINVALID,
    BNONE,
    BADD,
    BSUB,
    BMUL,
    BDIV
} b_operator;

typedef union beorn_values {
    char *cval;
    int ival;
    long double fval;
} beorn_value;

typedef struct expr_tree {
    blex_types type;
    b_operator op;
    struct expr_tree *left;
    struct expr_tree *right;
    beorn_state* value;
} expr_tree;

typedef struct bg_error {
  int has_error;
  size_t line;
  beorn_state* state_error;
} bg_error;

/* alias functions handler */
int create_stack_bpsm();

int increment_stack_bpsm();

int del_first_stack_bpsm();

int add_elem_stack_bpsm();

/* helpers */

beorn_state* new_definition();

beorn_state* new_call_definition();

beorn_state* new_parser_error(const char* msg);

int initialize_new_state(stack_bpsm* gs, esm s);

int bauto_state_update(stack_bpsm* gs, beorn_state* b, esm tp, int lenght);

void auto_state_update(stack_bpsm* gs, beorn_state* b);

beorn_state* get_last_state();

int add_child(stack_bpsm* gs, beorn_state* root, beorn_state* child);

int close_pending_structs(stack_bpsm* gs, beorn_state* root, types ct);

void next_token();

void set_gberror(const char *msg);

beorn_state* get_curr_bvalue();

blex_types get_crr_type();

b_operator get_operator();

expr_tree *new_node(blex_types type, b_operator operation, expr_tree *left, 
  expr_tree *right, beorn_state *value
);

expr_tree *value_expr();

expr_tree *mult_expr();

expr_tree *add_expr();

expr_tree *build_expr_tree();

void infix_to_bexpression(beorn_state* bs, expr_tree *expr);

int get_args_by_comma();

void process_token();

beorn_state* beorn_parser(char *input);

#endif
