#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dparser.h"
#include "dlex.h"

drax_state* bs;
d_token* gtoken;
bg_error* gberr;

int gcurr_line_number = 1;

g_act_state* global_act_state;

/* helpers */
drax_state* new_definition() {
  drax_state* bdef = new_expression();
  return bdef;
}

drax_state* new_call_definition() {
  drax_state* bdef = new_expression();
  bdef->act = BACT_CALL_OP;
  return bdef;
}

drax_state* new_parser_error(const char* msg) {
  drax_state* err = new_error(BPARSER_ERROR, msg);
  return err;
}

drax_state* get_last_state(drax_state* root) {
  if (root->length <= 0) { return NULL; }
  drax_state* curr;

  if (((root->child[root->length - 1]->type == BT_PACK) || 
       (root->child[root->length - 1]->type == BT_LIST) || 
       (root->child[root->length - 1]->type == BT_EXPRESSION)) &&
       (root->child[root->length - 1]->closed == 0))
  {
    curr = get_last_state(root->child[root->length - 1]);
    return curr;
  }
  
  curr = root->child[root->length - 1];
  root->length--;
  root->child = (drax_state**) realloc(root->child, sizeof(drax_state*) * root->length);
  return curr;
}

/**
 * Add new nodes to AST.
 * Always the last open expression.
 */
int add_child(drax_state* root, drax_state* child) {
  if (root->length <= 0) {
    root->length++;
    root->child = (drax_state**) malloc(sizeof(drax_state*));
    root->child[0] = child;

    return 1;
  } else {
    drax_state* crr;
    if (((root->child[root->length - 1]->type == BT_PACK) || 
         (root->child[root->length - 1]->type == BT_LIST) || 
         (root->child[root->length - 1]->type == BT_EXPRESSION)) &&
         (root->child[root->length - 1]->closed == 0))
    {
      crr  = root->child[root->length - 1];
      if (add_child(crr, child)) return 1;
    } else {
      crr = root;
    }

    crr->length++;

    if (crr->length <= 1) {
      crr->child = (drax_state**) malloc(sizeof(drax_state*));
    } else {
      crr->child = (drax_state**) realloc(crr->child, sizeof(drax_state*) * crr->length);
    }
    crr->child[crr->length - 1] = child;
    return 1;
  }
}

/**
 * Close the last open expression/sub expression in AST.
 */
int close_pending_structs(drax_state* root, types ct) {
  if (root->length == 0) return 0;

  if (((root->child[root->length - 1]->type == BT_PACK) || 
       (root->child[root->length - 1]->type == BT_LIST) ||
       (root->child[root->length - 1]->type == BT_EXPRESSION)) &&
       (root->child[root->length - 1]->closed == 0))
  {
    if (close_pending_structs(root->child[root->length - 1], ct )) {
      return 1;
    } else {
      if (root->child[root->length - 1]->type == ct) {
        root->child[root->length - 1]->closed = 1;
        return 1;
      }
    }
  }

  return 0;
}

void next_token() {
  free(gtoken);
  gtoken = lexan();

  if (TK_BREAK_LINE == gtoken->type) {
    gcurr_line_number++;
  }
}

static void next_token_ignore_space() {
  next_token();

  while (TK_BREAK_LINE == gtoken->type) {
    next_token();
  }
}

void set_gberror(const char *msg) {
  if (gberr->has_error) return;

  gberr->has_error = 1;
  gberr->state_error = new_error(BSYNTAX_ERROR, msg);
  gberr->state_error->trace = (bstack_trace*) malloc(sizeof(bstack_trace));
  gberr->state_error->trace->line = gcurr_line_number;
  gberr->state_error->trace->file = NULL;
  gberr->line = gcurr_line_number;
}

/**
 * Helpers to arith. expression tree.
 */

drax_state* get_curr_bvalue() {  
  switch (gtoken->type)
  {
    case TK_INTEGER: return new_integer(gtoken->fval);
    
    case TK_FLOAT: return new_float(gtoken->fval);

    default:
      set_gberror("Unspected expression type!");
      break;
  }

  return NULL;
}

dlex_types get_crr_type() {
  if (NULL == gtoken) return 0;
  return gtoken->type;
}

b_operator get_operator() {
  switch (gtoken->type)
  {
    case TK_ADD: return BADD;
    case TK_SUB: return BSUB;
    case TK_MUL: return BMUL;
    case TK_DIV: return BDIV;
    default: return BINVALID;
  }
}

expr_tree *new_node(dlex_types type, b_operator operation, expr_tree *left, 
  expr_tree *right, drax_state *value
) {
    expr_tree *n = (expr_tree*) malloc(sizeof(expr_tree));
    if (NULL == n) { set_gberror("Memory error, fail to process expression"); }
    n->type = type;
    n->op = operation;
    n->left = left;
    n->right = right;
    n->value = value;
    return n;
}

expr_tree *value_expr() {
  if (TK_SYMBOL == gtoken->type) {
    char* rigth_symbol = gtoken->cval;
    next_token();
    if (TK_PAR_OPEN == gtoken->type)  {
      add_child(bs, new_definition());
      add_child(bs, new_symbol(rigth_symbol));
      next_token();
      get_args_by_comma();

      if(!close_pending_structs(bs, BT_EXPRESSION)) 
        set_gberror("expression error, pair not found.");

      drax_state* crr_bs = get_last_state(bs);
      expr_tree *result = new_node(TK_SYMBOL, BNONE, NULL, NULL, crr_bs);
      next_token();
      return result;
    } else {
      expr_tree *result = new_node(TK_SYMBOL, BNONE, NULL, NULL, new_symbol(rigth_symbol));
      return result;
    }
  }

  if ((TK_INTEGER == gtoken->type) || (TK_FLOAT == gtoken->type)) {
      expr_tree *result = new_node(gtoken->type, BNONE, NULL, NULL, get_curr_bvalue());
      next_token();
      return result;
  }

  set_gberror("ArithmeticError, bad argument to expression.");
  return NULL;
}

expr_tree *preference_expr() {
    if (gtoken->type == TK_PAR_OPEN) {
      next_token();
      expr_tree *expr = add_expr();
      next_token();
      return expr;
    }

    return value_expr();
}

expr_tree *mult_expr() {
    expr_tree *expr = preference_expr();
    while (gtoken->type == TK_MUL || gtoken->type == TK_DIV) {
        b_operator op = get_operator();
        next_token();
        expr_tree *expr2 = preference_expr();
        expr = new_node(gtoken->type, op, expr, expr2, NULL);
    }
    return expr;
}

expr_tree *add_expr() {
    expr_tree *expr = mult_expr();
    while (gtoken->type == TK_ADD || gtoken->type == TK_SUB) {
        b_operator op = get_operator();
        next_token();
        expr_tree *expr2 = mult_expr();
        expr = new_node(gtoken->type, op, expr, expr2, NULL);
    }
    return expr;
}

expr_tree *build_expr_tree() {
    expr_tree *expr = add_expr();
    return expr;
}

/**
 * Convert expression to drax expression
 */

void infix_to_bexpression(drax_state* sbs, expr_tree *expr) {
  if (NULL == expr) return;

  if ((BNONE == expr->op) &&
      ((TK_INTEGER == expr->type) || (TK_FLOAT == expr->type) || (TK_SYMBOL == expr->type))
  ) {
    add_child(sbs, expr->value);
    free(expr);
    return;
  }

  add_child(bs, new_expression());

  switch (expr->op)
  {
    case BADD: add_child(bs, new_symbol("+")); break;
    case BSUB: add_child(bs, new_symbol("-")); break;
    case BMUL: add_child(bs, new_symbol("*")); break;
    case BDIV: add_child(bs, new_symbol("/")); break;
    
    default:
      set_gberror("invalid operation.");
      break;
  }

  infix_to_bexpression(bs, expr->left);
  infix_to_bexpression(bs, expr->right);

  free(expr);

  if(!close_pending_structs(bs, BT_EXPRESSION))
    set_gberror("expression error, pair not found.");
}

int get_args_by_comma() {
  int processing = 1;
  int param_qtt = 0;

  while (processing)
  {
    process_token();
    if (TK_BREAK_LINE == gtoken->type) { next_token_ignore_space(); }
    processing = TK_COMMA == gtoken->type; 

    if (processing) {
      next_token_ignore_space();
      param_qtt++;
    }
  }

  return param_qtt;
}

/* Main language definition */

static void drax_arith_expression() {
  expr_tree* trr = build_expr_tree();
  infix_to_bexpression(bs, trr);
}

static int drax_call_function() {
  d_token* nxt = b_check_next(NULL);
  if (TK_PAR_OPEN != nxt->type) {
    free(nxt);
    return 0;
  }

  free(nxt);
  char* rigth_symbol = gtoken->cval;
  next_token();

  next_token();
  add_child(bs, new_call_definition());
  add_child(bs, new_symbol(rigth_symbol));

  if (TK_PAR_CLOSE != gtoken->type) {
    get_args_by_comma();
  }

  if (TK_PAR_CLOSE != gtoken->type) {
    set_gberror("missing ) after argument list.");
  }

  if(!close_pending_structs(bs, BT_EXPRESSION)) {
    set_gberror("Expression pair not found.");
  }

  next_token();

  return 1;
}

static int drax_define_var() {
  d_token* nxt = b_check_next(NULL);
  if (TK_EQ != nxt->type) {
    free(nxt);
    return 0;
  }

  free(nxt);
  char* rigth_symbol = gtoken->cval;
  next_token();

  next_token();
  add_child(bs, new_definition());
  add_child(bs, new_symbol("set"));
  add_child(bs, new_symbol(rigth_symbol));
  process_token();

  if(!close_pending_structs(bs, BT_EXPRESSION)) {
      set_gberror("function pair not found.");
  }

  return 1;
}

static int is_arith_op(dlex_types type) {
  return (TK_ADD == type) || (TK_DIV == type) || 
         (TK_MUL == type) || (TK_SUB == type);
}

static int check_is_conclusion_token() {
  return 
    (TK_BREAK_LINE == gtoken->type) ||
    (TK_EOF == gtoken->type) ||
    (TK_END == gtoken->type) ||
    (TK_COMMA == gtoken->type) ||
    (TK_PAR_CLOSE == gtoken->type) ||
    (TK_BRACE_CLOSE == gtoken->type) ||
    (TK_BRACKET_CLOSE == gtoken->type);
}

static int arithm_simple_terminator(dlex_types tt){//add comma
  return ((TK_BREAK_LINE == tt) || (TK_EOF == tt) || (TK_END == tt));
}

static int curr_exp_is_arithm_op() {
  int zero = 0;
  int* jump_count = &zero;
  int cnte_proc = 1;
  d_token* nxt = b_check_next(jump_count);
  size_t stack_counter = 0;

  if (is_arith_op(nxt->type))  { return 1; }
  
  /* if curr. expr. is number */
  if (arithm_simple_terminator(nxt->type)) { return 0; }

  while (cnte_proc) {
    if (TK_PAR_OPEN == nxt->type)  { stack_counter++; }
    if (TK_PAR_CLOSE == nxt->type) { stack_counter--; }
    nxt = b_check_next(jump_count);

    if ((stack_counter == 0) && (is_arith_op(nxt->type))) return 1;

    cnte_proc = (!arithm_simple_terminator(nxt->type));
  }
  free(nxt);

  return 0;
}

static int drax_arith_op() {
  
  if (!curr_exp_is_arithm_op()) { return 0; }

  drax_arith_expression();
  if (!check_is_conclusion_token()) {
    set_gberror("Unspected token");
  }
  return 1;
}

static int drax_import_file() {
  next_token();
  add_child(bs, new_call_definition());
  add_child(bs, new_symbol("import"));

  if (TK_STRING != gtoken->type) {
    set_gberror("Cannot use import to current definition");
    return 1;
  }
  add_child(bs, new_string(gtoken->cval));

  if(!close_pending_structs(bs, BT_EXPRESSION)) {
      set_gberror("function pair not found.");
  }

  next_token();

  return 1;
}

static int drax_end_definition() {
  if(!close_pending_structs(bs, BT_PACK)) {
    set_gberror("pack freeze pair not found.");
    return 1;
  }

  if(!close_pending_structs(bs, BT_EXPRESSION)) {
    set_gberror("function pair not found.");
    return 1;
  }

  return 0;
}

static int drax_function_definition() {
  add_child(bs, new_definition());
  add_child(bs, new_symbol("fun"));
  next_token();

  if (TK_SYMBOL != gtoken->type) { set_gberror("Invalid function name."); }

  add_child(bs, new_symbol(gtoken->cval));
  next_token();

  if (TK_PAR_OPEN != gtoken->type) { set_gberror("Bad definition to function."); }

  next_token();

  int process = 1;
  
  add_child(bs, new_list());
  if (TK_PAR_CLOSE != gtoken->type) {
    while (process) {
      if (TK_SYMBOL != gtoken->type) {
        set_gberror("Invalid function param.");
        return 1;
      }

      add_child(bs, new_symbol(gtoken->cval));
      next_token();

      process = (TK_COMMA == gtoken->type);
      if (process) { next_token(); }
    }
  }
  
  if(!close_pending_structs(bs, BT_LIST)) {
    set_gberror("params pair not found.");
    return 1;
  }
  
  if (TK_PAR_CLOSE!= gtoken->type) {
    set_gberror("pair not identified to params.");
    return 1;
  }

  next_token();

  if (TK_DO != gtoken->type) { 
    set_gberror("bad definition to function.");
    return 1;
  }
  add_child(bs, new_pack());
  next_token();

  process = 1;
  while (process) {
    if (TK_END == gtoken->type) {
      drax_end_definition();
      return 1;
    }
    process_token();

    if (TK_END == gtoken->type) {
      drax_end_definition();
      return 1;
    }
    process = ((TK_EOF != gtoken->type) && (TK_END != gtoken->type));
  }

  return 1;
}

/* if definition */

static int is_bool_op(dlex_types type) {
  return (TK_DEQ == type)    || (TK_TEQ == type)     || 
         (TK_LE == type)     || (TK_LS == type)      ||
         (TK_NOT_EQ == type) || (TK_NOT_DEQ == type) ||
         (TK_BE == type)     || (TK_BG == type);
}

static const char* bbool_to_str(dlex_types type) {
  switch (type) {
    case TK_NOT_EQ: return "!=";
    case TK_NOT_DEQ: return "!==";
    case TK_DEQ: return "==";
    case TK_TEQ: return "===";
    case TK_LE:  return "<=";
    case TK_LS:  return "<";
    case TK_BE:  return ">=";
    case TK_BG:  return ">";
  
    default:
    set_gberror("bool symbol with unspected type.");
    return "";
  }
}

static int process_bool_expr() {
  if (is_bool_op(gtoken->type)) {
    global_act_state->state = AS_BOOL;
    drax_state* left = get_last_state(bs);
    add_child(bs, new_expression());
    add_child(bs, new_symbol(bbool_to_str(gtoken->type)));
    add_child(bs, left);
    next_token();
    process_token();
   
    global_act_state->state = AS_NONE;
    if(!close_pending_structs(bs, BT_EXPRESSION)) {
      set_gberror("bool pair not found.");
      return 1;
    }
    return 1;
  }
  return 0;
}

static int drax_if_definition() {
  add_child(bs, new_definition());
  add_child(bs, new_symbol("if"));

  next_token();
  process_token();

  if (TK_DO != gtoken->type) { 
    set_gberror("Fail to define 'if', unspected token.");
    return 1;
  }

  add_child(bs, new_pack());

  next_token();
  int process = 1;
  while (process) {
    
    process = (
      (TK_EOF != gtoken->type) &&
      (TK_END != gtoken->type) &&
      (TK_ELSE != gtoken->type)
    );

    if (process) { process_token(); }
  }

  if(!close_pending_structs(bs, BT_PACK)) {
    set_gberror("pack freeze pair not found.");
    return 1;
  }

  if (TK_ELSE == gtoken->type) {

    add_child(bs, new_pack());

    next_token();
    process = 1;
    while (process) {

      process = (
        (TK_EOF != gtoken->type) &&
        (TK_END != gtoken->type) &&
        (TK_ELSE != gtoken->type)
      );

      if (process) { process_token(); }
    }
  } else {
    add_child(bs, new_pack());
  }

  if (TK_END != gtoken->type) { 
    set_gberror("Fail to define 'if', unspected token.");
    return 1;
  }

  drax_end_definition();

  next_token();
  
  return 1;
}

static const char* blogic_to_str(dlex_types type) {
  switch (type) {
    case TK_AND: return "and";
    case TK_OR: return "or";
  
    default:
    set_gberror("logic symbol with unspected type.");
    return "";
  }
}

static int is_logic_op(dlex_types type) {
  return (TK_AND == type) || (TK_OR == type);
}

static int process_logic_gates() {
  if (is_logic_op(gtoken->type)) {
    drax_state* left = get_last_state(bs);
    add_child(bs, new_expression());
    add_child(bs, new_symbol(blogic_to_str(gtoken->type)));
    add_child(bs, left);
    next_token();
    process_token();
   
    if(!close_pending_structs(bs, BT_EXPRESSION)) {
      set_gberror("logic pair not found.");
      return 1;
    }
    return 1;
  }
  return 0;
}

void process_token() {
  switch (gtoken->type) {
    case TK_BREAK_LINE:
      next_token();
      break;

    case TK_FLOAT:
      if (drax_arith_op()) { break; }
      add_child(bs, new_float(gtoken->fval));
      next_token();
      break;

    case TK_INTEGER:
      if (drax_arith_op()) { break; }
      add_child(bs, new_integer(gtoken->fval));
      next_token();
      break;

    case TK_PAR_OPEN:
      if (drax_arith_op()) { break; };
      next_token();
      process_token();

      if (TK_PAR_CLOSE != gtoken->type) { set_gberror("Expression pair not found."); }
      next_token();
      break;

    case TK_SYMBOL: {
      if (drax_define_var()) { break; };
      if (drax_arith_op()) { break; };
      if (drax_call_function()) { break; };

      add_child(bs, new_symbol(gtoken->cval));
      next_token();
      break;
    }

    case TK_IF:
      drax_if_definition();
      break;

    case TK_IMPORT:
      drax_import_file();
      break;

    case TK_FUN:
      drax_function_definition();
      next_token();
      break;

    case TK_LAMBDA: /* pending */ next_token(); break;

    case TK_STRING:
      add_child(bs, new_string(gtoken->cval));
      next_token();
      break;

    case TK_BRACE_OPEN: /* pending */ next_token(); break;
    case TK_BRACE_CLOSE: /* pending */ next_token(); break;
    case TK_BRACKET_OPEN: {
      add_child(bs, new_list());

      next_token_ignore_space();

      if (TK_BRACKET_CLOSE != gtoken->type) {
        get_args_by_comma();
      }
    
      if (TK_BRACKET_CLOSE != gtoken->type) {
        set_gberror("list pair not found.");
        break;
      }

      if(!close_pending_structs(bs, BT_LIST))
        set_gberror("list pair not found.");
      
      next_token();
      break;
    }

    case TK_END: {
      drax_end_definition();
      next_token();
      break;
    }

    case TK_NIL: {
      add_child(bs, new_nil());
      next_token();
      break;
    }

    default:
      set_gberror("Unspected token.");
      next_token();
      break;
  }

  process_bool_expr();

  /* Only if other process is finished */
  if (AS_NONE == global_act_state->state) {
    process_logic_gates();
  };
}

drax_state* drax_parser(char *input) {
  gcurr_line_number = 1;

  global_act_state = (g_act_state*) malloc(sizeof(g_act_state));
  global_act_state->state = AS_NONE;
  
  gberr = (bg_error*) malloc(sizeof(bg_error));
  gberr->line = 0;
  gberr->has_error = 0;
  gberr->state_error = NULL;

  bs = (drax_state*) malloc(sizeof(drax_state));
  bs->type = BT_PROGRAM;
  bs->child = (drax_state**) malloc(sizeof(drax_state*));
  bs->length = 0;
  bs->closed = 0;

  init_lexan(input);
  gtoken = NULL;

  next_token();
  while (TK_EOF != get_crr_type()) {
    process_token();
    
    if (gberr->has_error) {
      del_bstate(bs);
      return gberr->state_error;
    }
  }
  
  return bs;
}
