/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __BPARSER
#define __BPARSER

#include "dtypes.h"
#include "dlex.h"

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

/* Handler active state */
typedef enum type_act_state {
  AS_NONE,
  AS_BOOL,
  AS_LOGIC
} type_act_state;

typedef struct g_act_state {
  type_act_state state;
} g_act_state;

/* alias functions handler */
int create_stack_bpsm();

int increment_stack_bpsm();

int del_first_stack_bpsm();

int add_elem_stack_bpsm();

/* helpers */

beorn_state* new_definition();

beorn_state* new_call_definition();

beorn_state* new_parser_error(const char* msg);

beorn_state* get_last_state();

int add_child(beorn_state* root, beorn_state* child);

int close_pending_structs(beorn_state* root, types ct);

void next_token();

void set_gberror(const char *msg);

beorn_state* get_curr_bvalue();

blex_types get_crr_type();

b_operator get_operator();

expr_tree *new_node(blex_types type, b_operator operation, expr_tree *left, 
  expr_tree *right, beorn_state *value
);

expr_tree *value_expr();

expr_tree *preference_expr();

expr_tree *mult_expr();

expr_tree *add_expr();

expr_tree *build_expr_tree();

void infix_to_bexpression(beorn_state* bs, expr_tree *expr);

int get_args_by_comma();

void process_token();

beorn_state* beorn_parser(char *input);

#endif
