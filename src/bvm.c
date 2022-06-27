#include <stdlib.h>
#include <stdio.h>
#include "bvm.h"
#include "bprint.h"
#include "btypes.h"
#include "bfunctions.h"

beorn_state* process_expression(beorn_env* benv, beorn_state* curr) {
  if(curr->length == 0)
    return curr;

  if (
      (curr->child[0]->type != BT_SYMBOL) ||
      ((curr->length == 1) && (curr->child[0]->type == BT_SYMBOL))
      )
  {
    curr->child[0] = process(benv, curr->child[0]);
  }

  for (int i = 0; i < curr->length; i++) {
    if (curr->child[i]->type == BT_ERROR) {
      return curr->child[i];
    }
  }

  if (curr->length == 1) {
    beorn_state* res = curr->child[0];
    free(curr);
    return res;
  }

  BASSERT(
    curr->child[0]->type != BT_SYMBOL &&
    curr->child[0]->type != BT_LAMBDA &&
    curr->child[0]->type != BT_FUNCTION,
    BUNSPECTED_TYPE,
    "Invalid expression."
  )

  beorn_state* r = call_func_builtin(benv, curr);

  // call_lamda
  return r;
}

beorn_state* process_symbol(beorn_env* benv, beorn_state* curr) {
  for (int i = 0; i < benv->length; i++) {
    if (strcmp(benv->symbol[i], curr->cval) == 0) {
      return bcopy_state(benv->bval[i]);
    }
  }

  if (benv->global != NULL) {
    return process_symbol(benv->global, curr);
  }

  return new_error(BREFERENCE_ERROR, "symbol '%s' not found.", curr->cval);
}

beorn_state* process_list(beorn_env* benv, beorn_state* curr) {
  for (int i = 0; i < curr->length; i++)
  {
    curr->child[i] = process(benv, curr->child[i]);
  }

  return curr;
}

beorn_state* process(beorn_env* benv, beorn_state* curr) {
  switch (curr->type) {
    case BT_PROGRAM:
    case BT_INTEGER:
    case BT_FLOAT:
    case BT_STRING:
    case BT_ERROR:
    case BT_FUNCTION:
    case BT_LAMBDA:
    case BT_PACK:       return curr;
    case BT_LIST:       return process_list(benv, curr);
    case BT_SYMBOL:     return process_symbol(benv, curr);
    case BT_EXPRESSION: return process_expression(benv, curr);
    
    default: return new_error(BUNKNOWN_TYPE_ERROR, "type '%s' not found.", btype_to_str(curr->type));
  }
}

/**
 * run all childs of beorn state.
 */
void __run_bs__(beorn_env* benv, beorn_state* curr) {
  if (curr->type == BT_ERROR) {
    bprint(curr);
    bbreak_line();
  } else {
    for (int i = 0; i < curr->length; i++) {
        beorn_state* evaluated = process(benv, curr->child[i]);
        if (evaluated->type == BT_ERROR) {
          bprint(evaluated);
          bbreak_line();
        }
    }
    del_bstate(curr);
  }
}

void __run__(beorn_env* benv, beorn_state* curr) {
  __run_bs__(benv, curr);
}
