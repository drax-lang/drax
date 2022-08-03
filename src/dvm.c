#include <stdlib.h>
#include <stdio.h>
#include "dvm.h"
#include "dprint.h"
#include "dtypes.h"
#include "dfunctions.h"

drax_state* process_expression(drax_env* benv, drax_state* curr) {
  if(curr->length == 0)
    return curr;

  if (
      (curr->child[0]->type != BT_SYMBOL) ||
      (is_call_fn(curr) && (curr->length == 1) && (curr->child[0]->type == BT_SYMBOL))
    )
  {
    curr->child[0] = process(benv, curr->child[0]);
  }

  for (int i = 0; i < curr->length; i++) {
    if (curr->child[i]->type == BT_ERROR) {
      return curr->child[i];
    }
  }

  if ((curr->length == 1) && (is_call_fn(curr))) {
    drax_state* res = bpop(curr, 0);
    del_bstate(curr);
    return res;
  }

  BASSERT(
    curr->child[0]->type != BT_SYMBOL &&
    curr->child[0]->type != BT_LAMBDA &&
    curr->child[0]->type != BT_FUNCTION,
    BUNSPECTED_TYPE,
    "Invalid expression."
  )

  drax_state* r = bcall_function(benv, curr);

  return r;
}

drax_state* process_symbol(drax_env* benv, drax_state* curr) {
  drax_state* bres = bget_env_value(benv, curr);

  if(NULL != bres) {
    return bres;
  }

  if (benv->global != NULL) {
    return process_symbol(benv->global, curr);
  }

  return new_error(BREFERENCE_ERROR, "symbol '%s' not found.", curr->cval);
}

drax_state* process_list(drax_env* benv, drax_state* curr) {
  for (int i = 0; i < curr->length; i++)
  {
    curr->child[i] = process(benv, curr->child[i]);
  }

  return curr;
}

drax_state* process(drax_env* benv, drax_state* curr) {
  switch (curr->type) {
    case BT_PROGRAM:
    case BT_INTEGER:
    case BT_FLOAT:
    case BT_STRING:
    case BT_ERROR:
    case BT_FUNCTION:
    case BT_LAMBDA:
    case BT_NIL:
    case BT_PACK:       return curr;
    case BT_LIST:       return process_list(benv, curr);
    case BT_SYMBOL:     return process_symbol(benv, curr);
    case BT_EXPRESSION: return process_expression(benv, curr);
    
    default: return new_error(BUNKNOWN_TYPE_ERROR, "type '%s' not found.", btype_to_str(curr->type));
  }
}

/**
 * run all childs of drax state.
 */
void __run_bs__(drax_env* benv, drax_state* curr, int inter_mode) {
  if (curr->type == BT_ERROR) {
    bprint(curr);
    bbreak_line();
  } else {
    for (int i = 0; i < curr->length; i++) {
      drax_state* evaluated = process(benv, curr->child[i]);
      if (evaluated->type == BT_ERROR) {
        bprint(evaluated);
        bbreak_line();
      } else if (inter_mode) {
        bprint(evaluated);
        bbreak_line();
      }
    }
    del_bstate(curr);
  }
}

void __run__(drax_env* benv, drax_state* curr, int inter_mode) {
  __run_bs__(benv, curr, inter_mode);
}
