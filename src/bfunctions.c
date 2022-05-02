#include "bfunctions.h"
#include <stdlib.h>
#include <string.h>

beorn_state* bpop(beorn_state* curr, int i){
  beorn_state* ele = curr->child[i];

  memmove(&curr->child[i], &curr->child[i+1],
    sizeof(beorn_state*) * (curr->length -i -1));

  curr->length--;
  curr->child = realloc(curr->child, sizeof(beorn_state*) * curr->length);
  return ele;
}

beorn_state* bpush(beorn_state* curr) {
  return curr;
}

long double get_number(beorn_state* v) {
  switch (v->type)
  {
  case BT_FLOAT:
    return v->fval;

  case BT_INTEGER:
    return v->ival;
    
  default:
    return 0;
  }
}

beorn_state* do_op(beorn_state* curr) {

  beorn_state* opr = bpop(curr, 0);
  char op = opr->cval[0];
  del_bstate(opr);

  for (int i = 0; i < curr->length; i++) {
    if (
          (curr->child[i]->type != BT_FLOAT) &&
          (curr->child[i]->type != BT_INTEGER) &&
          (curr->child[i]->type != BT_EXPRESSION)
        )
    {
      del_bstate(curr);
      return new_error(BTYPE_ERROR, "unsupported type.");
    }
  }

  beorn_state* x = bpop(curr, 0);

  if ((op == '-') && curr->length == 0) {
    if (x->type == BT_INTEGER) {
      x->ival = -x->ival;
    }

    if (x->type == BT_FLOAT) {
      x->fval = -x->fval;
    }
  }

  long double r = get_number(x);
  int tval = x->type;

  while (curr->length > 0) {
    beorn_state* y = bpop(curr, 0);

    if (y->type == BT_EXPRESSION) {
      y = do_op(y);
    }

    if (y->type == BT_FLOAT) {
      tval = BT_FLOAT;
    }

    long double tvl = get_number(y);

    if (op == '+') { r += tvl; }
    if (op == '-') { r -= tvl; }
    if (op == '*') { r *= tvl; }
    if (op == '/') {
      if (tvl == 0) {
        del_bstate(x);
        del_bstate(y);
        x = new_error(BZERO_DIVISION_ERROR, "bad argument in arithmetic expression.");
        break;
      }
      r /= tvl;
    }

    del_bstate(y);
  }

  del_bstate(curr);

  x->type = tval;
  if (tval == BT_FLOAT) {
    x->fval = r;
  } else {
    x->ival = (int) r;
  }

  return x;
}

beorn_state* bb_type_of(beorn_state* exp) {
  BASSERT(exp->type != BT_EXPRESSION, BTYPE_ERROR, "expeted expression, example:\n  (type-of 123)");
  BASSERT(exp->length == 1, BTYPE_ERROR, "missing one argument.");
  BASSERT(exp->length > 2,  BTYPE_ERROR, "expected only one argument.");

  char* t = btype_to_str(exp->child[1]->type);
  free(exp);

  return new_string(t);
}

beorn_state* bb_set(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->type != BT_EXPRESSION, BTYPE_ERROR, "expeted expression, example:\n  (set name 123)");
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'set' missing two arguments.");
  BASSERT(exp->length > 3,  BTYPE_ERROR, "expected only two arguments.");
  BASSERT(exp->child[1]->type != BT_SYMBOL,  BTYPE_ERROR, "invalid argment after 'set'");

  bset_env(benv, exp->child[1], exp->child[2]);
  beorn_state* pck = new_pack("none");
  pck->closed = 1;
  del_bstate(exp);
  return pck;
}

beorn_state* call_func_builtin(beorn_env* benv, beorn_state* exp) {
  beorn_state* bs = exp->child[0];

  if (strcmp("+", bs->cval) == 0) { return do_op(exp); }
  if (strcmp("-", bs->cval) == 0) { return do_op(exp); }
  if (strcmp("*", bs->cval) == 0) { return do_op(exp); }
  if (strcmp("/", bs->cval) == 0) { return do_op(exp); }

  if (strcmp("type-of", bs->cval) == 0) { return bb_type_of(exp); }
  
  if (strcmp("set", bs->cval) == 0) { return bb_set(benv, exp);}
  
  return exp;

}
