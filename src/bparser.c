#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bparser.h"
#include "blex.h"

b_token* gtoken;
int ignore_command = 0;

/* state handler */
stack_bpsm* create_stack_bpsm() {
  stack_bpsm* gsb = (stack_bpsm*) malloc(sizeof(stack_bpsm));
  gsb->size = 0;
  gsb->bpsm = (bpsm**) malloc(sizeof(bpsm*));
  return gsb;
}

int increment_stack_bpsm(stack_bpsm* gsb) {
  if ((NULL == gsb) || (gsb->size == 0)) return 0;
  gsb->bpsm[gsb->size - 1]->count++;
  return 0;
}

int del_first_stack_bpsm(stack_bpsm* gsb) {
  if ((NULL == gsb) || (gsb->size == 0)) return 0;

  free(gsb->bpsm[gsb->size -1]);
  gsb->size--;
  gsb->bpsm = (bpsm**) realloc(gsb->bpsm, sizeof(bpsm*) * gsb->size);
  return 0;
}

int add_elem_stack_bpsm(stack_bpsm* gsb) {
  if (NULL == gsb) return 0;

  gsb->size++;
  gsb->bpsm = (bpsm**) realloc(gsb->bpsm, sizeof(bpsm*) * gsb->size);
  gsb->bpsm[gsb->size -1] = (bpsm*) malloc(sizeof(bpsm));
  gsb->bpsm[gsb->size -1]->mode = BP_NONE;
  gsb->bpsm[gsb->size -1]->count = 0;
  return 0;
}

/* helpers */

beorn_state* new_definition() {
  beorn_state* bdef = new_expression("(");
  return bdef;
}

beorn_state* new_parser_error(const char* msg) {
  beorn_state* err = new_error(BPARSER_ERROR, msg);
  return err;
}

int is_simple_expressions(const char* key) {
  return (strcmp("import", key) == 0) ||
         (strcmp("set", key) == 0) ||
         (strcmp("let", key) == 0) ||
         (strcmp("fun", key) == 0) ||
         (strcmp("lambda", key) == 0) ||
         (strcmp("if", key) == 0);
}

esm keyword_to_bpsm(const char* key) {
  if (strcmp("set", key) == 0) {
    return BP_SIMPLE_DEFINITIONS;
  } else if (strcmp("let", key) == 0) {
    return BP_SIMPLE_DEFINITIONS;
  } else if (strcmp("lambda", key) == 0) {
    return BP_LAMBDA_DEFINITION;
  } else if (strcmp("fun", key) == 0) {
    return BP_FUNCTION_DEFINITION;
  } else if (strcmp("import", key) == 0) {
    return BP_ONE_ARG;
  } else if (strcmp("if", key) == 0) {
    return BP_THREE_ARG;
  }

  return BP_NONE;
}

int initialize_new_state(stack_bpsm* gs, esm s) {
  add_elem_stack_bpsm(gs);

  gs->bpsm[gs->size -1]->mode = s;
  return 1;
}

int bauto_state_update(stack_bpsm* gs, beorn_state* b, esm tp, int lenght) {
  if (gs->size <= 0) return 0;

  bpsm* curr = gs->bpsm[gs->size -1];
  if (curr->mode == tp) {
    if (curr->count == lenght) {
      b->closed = 1;
      close_pending_structs(gs, b, BT_EXPRESSION);   
      del_first_stack_bpsm(gs);
    }
  }
  return 0; 
}

void auto_state_update(stack_bpsm* gs, beorn_state* b) {
  bauto_state_update(gs, b, BP_ONE_ARG,             2);
  bauto_state_update(gs, b, BP_TWO_ARG,             3);
  bauto_state_update(gs, b, BP_THREE_ARG,           4);
  bauto_state_update(gs, b, BP_FOUR_ARG,            5);

  bauto_state_update(gs, b, BP_SIMPLE_DEFINITIONS,  3);
  bauto_state_update(gs, b, BP_LAMBDA_DEFINITION,   3);
  bauto_state_update(gs, b, BP_FUNCTION_DEFINITION, 4);
}

int add_child(stack_bpsm* gs, beorn_state* root, beorn_state* child) {
  if (root->length <= 0) {
    root->length++;
    root->child = (beorn_state**) malloc(sizeof(beorn_state*));
    root->child[0] = child;
    increment_stack_bpsm(gs);
    return 1;
  } else {
    beorn_state* crr;
    if (((root->child[root->length - 1]->type == BT_PACK) || 
         (root->child[root->length - 1]->type == BT_LIST) || 
         (root->child[root->length - 1]->type == BT_EXPRESSION)) &&
         (root->child[root->length - 1]->closed == 0))
    {
      crr  = root->child[root->length - 1];
      if (add_child(gs, crr, child)) return 1;
    } else {
      crr = root;
    }

    crr->length++;

    if (crr->length <= 1) {
      crr->child = (beorn_state**) malloc(sizeof(beorn_state*));
    } else {
      crr->child = (beorn_state**) realloc(crr->child, sizeof(beorn_state*) * crr->length);
    }
    crr->child[crr->length - 1] = child;
    increment_stack_bpsm(gs);
    return 1;
  }
}

int close_pending_structs(stack_bpsm* gs, beorn_state* root, types ct) {
  if (root->length == 0) return 0;

  if (((root->child[root->length - 1]->type == BT_PACK) || 
       (root->child[root->length - 1]->type == BT_LIST) ||
       (root->child[root->length - 1]->type == BT_EXPRESSION)) &&
       (root->child[root->length - 1]->closed == 0))
  {
    if (close_pending_structs(gs, root->child[root->length - 1], ct )) {
      return 1;
    } else {
      if (root->child[root->length - 1]->type == ct) {
        root->child[root->length - 1]->closed = 1;
        del_first_stack_bpsm(gs);
        return 1;
      }
    }
  }

  return 0;
}

void next_token() {
  if (ignore_command) {
    ignore_command = 0;
    return;
  };

  gtoken = lexan();
}

void ignore_next_command() {
  ignore_command= 1;
}

void fatal(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

/*
 * Handle expression tree
 */

beorn_value* get_curr_bvalue() {
  beorn_value* v = (beorn_value*) malloc(sizeof(beorn_value));
  
  switch (gtoken->type)
  {
  case TK_INTEGER:
    v->ival = gtoken->ival;
    break;
  
  case TK_FLOAT:
    v->fval = gtoken->fval;
    break;

  case TK_SYMBOL:
  // case TK_FUN: // handle function
    v->cval = gtoken->cval;
    break;

  default:
    fatal("unspected type on expression!");
    break;
  }

  return v;
}

blex_types get_crr_type() {
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

expr_tree *new_node(blex_types type, b_operator operation, expr_tree *left, 
  expr_tree *right, beorn_value *value
) {
    struct expr_tree *n = (struct expr_tree*) malloc(sizeof(struct expr_tree));
    if (n == NULL) {
        fatal("unable to Malloc New Structure Tree in \'create_new_node()\' Function in tree.c File");
    }
    n->type = type;
    n->op = operation;
    n->left = left;
    n->right = right;
    n->value = value;
    return n;
}

expr_tree *value_expr() {
    if (gtoken->type == TK_INTEGER || gtoken->type == TK_FLOAT) {
        expr_tree *result = new_node(gtoken->type, BNONE, NULL, NULL, get_curr_bvalue());
        next_token();
        return result;
    }
    fatal("can't determine value for token");
    return NULL;
}

expr_tree *mult_expr() {
    expr_tree *expr = value_expr();
    while (gtoken->type == TK_MUL || gtoken->type == TK_DIV) {
        b_operator op = get_operator();
        next_token();
        expr_tree *expr2 = value_expr();
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

void infix_to_bexpression(beorn_state* bs, expr_tree *expr) {
  if (NULL == expr) return;

  if ((BNONE == expr->op) && ((TK_INTEGER == expr->type) || (TK_FLOAT == expr->type))) {
    if (TK_INTEGER == expr->type) { add_child(0, bs, new_integer(expr->value->ival)); }
    if (TK_FLOAT == expr->type)   { add_child(0, bs, new_float(expr->value->fval));   }
    free(expr);
    return;
  }

  add_child(0, bs, new_expression(""));

  switch (expr->op)
  {
    case BADD: add_child(0, bs, new_symbol("+")); break;
    case BSUB: add_child(0, bs, new_symbol("-")); break;
    case BMUL: add_child(0, bs, new_symbol("*")); break;
    case BDIV: add_child(0, bs, new_symbol("/")); break;
    
    default:
      fatal("invalid operation.");
      break;
  }

  infix_to_bexpression(bs, expr->left);
  infix_to_bexpression(bs, expr->right);
  free(expr);
  if(!close_pending_structs(0, bs, BT_EXPRESSION))
    fatal("expression error, pair not found.");
}


beorn_state* beorn_parser(char *input) {
  stack_bpsm* gsb = create_stack_bpsm();

  beorn_state* bs = (beorn_state*) malloc(sizeof(beorn_state));
  bs->type = BT_PROGRAM;
  bs->child = (beorn_state**) malloc(sizeof(beorn_state*));
  bs->length = 0;

  init_lexan(input);
  gtoken = NULL;

  while (TK_EOF != get_crr_type()) {
    auto_state_update(gsb, bs);

    next_token();
     
     switch (gtoken->type)
     {
      case TK_SYMBOL:
        char* rigth_symbol = gtoken->cval;
        next_token();

        if (TK_EQ == gtoken->type) {
          if (initialize_new_state(gsb, BP_SIMPLE_DEFINITIONS) == 0)
            return new_error(BPARSER_ERROR, "Fail to initialize definition");

          next_token();
          add_child(0,   bs, new_definition(""));
          add_child(gsb, bs, new_symbol("set"));
          add_child(gsb, bs, new_symbol(rigth_symbol));
          ignore_next_command();
          break;
        }

        if (TK_PAR_OPEN == gtoken->type) {
          next_token();
          add_child(0, bs, new_definition(""));
          add_child(0, bs, new_symbol(rigth_symbol));
          ignore_next_command();
          break;
        }

        add_child(gsb, bs, new_symbol(rigth_symbol));
        break;
      
      case TK_IF: break;


      case TK_IMPORT: 
          next_token();
          add_child(0, bs, new_definition(""));
          add_child(0, bs, new_symbol("import"));
          if (TK_STRING != gtoken->type) { fatal("invalid import name."); }
          add_child(0, bs, new_string(gtoken->cval));
        break;

      case TK_FUN:
        add_child(0, bs, new_definition(""));
        add_child(0, bs, new_symbol("fun"));
        next_token();

        if (TK_SYMBOL != gtoken->type) { fatal("invalid function name."); }
          
        add_child(0, bs, new_symbol(gtoken->cval));
        next_token();

        if (TK_PAR_OPEN != gtoken->type) { fatal("invalid function name."); }

        next_token();

        int catching_params = 1;
        
        add_child(0, bs, new_list(""));
        while (catching_params) {
          if (TK_SYMBOL != gtoken->type) { fatal("invalid function param."); }

          add_child(0, bs, new_symbol(gtoken->cval));
          next_token();

          catching_params = (TK_COMMA == gtoken->type);
          if (catching_params) { next_token(); }
        }
        
        if(!close_pending_structs(gsb, bs, BT_LIST))
          return new_parser_error("params pair not found."); // fix
        
        if (TK_PAR_CLOSE!= gtoken->type) { fatal("pair not identified to params."); }

        next_token();

        if (TK_DO != gtoken->type) { fatal("bad definition to function."); }
        add_child(0, bs, new_pack(""));
        break;

      case TK_LAMBDA: break;

      case TK_FLOAT: 
      case TK_INTEGER:
        expr_tree* trr = build_expr_tree();
        infix_to_bexpression(bs, trr);
        break;
      case TK_STRING:
        add_child(gsb, bs, new_string(gtoken->cval));
        break;
      case TK_PAR_CLOSE:
        if(!close_pending_structs(gsb, bs, BT_EXPRESSION))
          return new_parser_error("expression pair not found.");
        break;
      case TK_BRACE_OPEN: break;
      case TK_BRACE_CLOSE: break;
      case TK_BRACKET_OPEN: {
        add_child(gsb, bs, new_list(""));

        if (initialize_new_state(gsb, BP_DINAMIC) == 0)
          return new_error(BPARSER_ERROR, "fail to create list.");

        break;
      }

      case TK_BRACKET_CLOSE: {
        if(!close_pending_structs(gsb, bs, BT_LIST))
          return new_parser_error("list pair not found.");
        break;
      }

      case TK_END:
        if(!close_pending_structs(gsb, bs, BT_PACK))
          return new_parser_error("pack freeze pair not found."); // fix

        if(!close_pending_structs(gsb, bs, BT_EXPRESSION))
          return new_parser_error("function pair not found."); // fix
        break;

     default:
       break;
     }
    
  }
  
  free(gsb);
  return bs;
}
