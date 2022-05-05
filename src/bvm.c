#include <stdlib.h>
#include <stdio.h>
#include "bvm.h"
#include "bfunctions.h"
#include "btypes.h"

beorn_state* process_expression(beorn_env* benv, beorn_state* curr) {
  if(curr->length == 0)
    return curr;

  for (size_t i = 0; i < curr->length; i++) {
    if ((curr->child[0]->type != BT_SYMBOL) ||
        (curr->length == 1) && (curr->child[0]->type == BT_SYMBOL))
    {
      curr->child[0] = process(benv, curr->child[0]);
    }
  }

  for (size_t i = 0; i < curr->length; i++) {
    if (curr->child[i]->type == BT_ERROR) {
      return curr->child[i];
    }
  }

  if (curr->length == 1) {
    beorn_state* res = curr->child[0];
    free(curr);
    return res;
  }

  BASSERT(curr->child[0]->type != BT_SYMBOL, BUNSPECTED_TYPE, "Invalid expression.")

  beorn_state* r = call_func_builtin(benv, curr);
  return r;
}

beorn_state* process_symbol(beorn_env* benv, beorn_state* curr) {
  for (int i = 0; i < benv->length; i++) {
    if (strcmp(benv->symbol[i], curr->cval) == 0) {
      return bcopy_state(benv->bval[i]);
    }
  }

  return new_error(BREFERENCE_ERROR, "symbol '%s' not found.", curr->cval);
}

beorn_state* process(beorn_env* benv, beorn_state* curr) {
  switch (curr->type) {
    case BT_PROGRAM:
    case BT_INTEGER:
    case BT_FLOAT:
    case BT_STRING:
    case BT_ERROR:
    case BT_FUNCTION:
    case BT_PACK:         return curr;
    case BT_SYMBOL:       return process_symbol(benv, curr);
    case BT_EXPRESSION:   return process_expression(benv, curr);
    
    default: return new_error(BUNKNOWN_TYPE_ERROR, "type '%s' not found.", btype_to_str(curr->type));
  }
}
