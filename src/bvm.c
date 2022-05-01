#include <stdlib.h>
#include <stdio.h>
#include "bvm.h"
#include "bfunctions.h"
#include "btypes.h"

#define BASSERT(cond, t, msg) if (!(cond)) { return new_error(t, msg); }

beorn_state* process_expression(beorn_env* benv, beorn_state* curr) {
  return do_op(curr);
}

beorn_state* process_symbol(beorn_env* benv, beorn_state* curr) {
  return curr;
}

beorn_state* process(beorn_env* benv, beorn_state* curr) {
  switch (curr->type) {
    case BT_INTEGER:      return curr;
    case BT_FLOAT:        return curr;
    case BT_STRING:       return curr;
    case BT_PACK_FREEZE:  return curr;
    case BT_SYMBOL:       return process_symbol(benv, curr);
    case BT_EXPRESSION:   return process_expression(benv, curr);
    
    default: return new_error(BUNKNOWN_TYPE_ERROR, "type not found.");
  }
}
