#include "bfunctions.h"
#include <stdlib.h>
#include <string.h>
#include "bvm.h"

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

beorn_state* do_op(beorn_env* benv, beorn_state* curr) {

  beorn_state* opr = bpop(curr, 0);
  char op = opr->cval[0];
  del_bstate(opr);

  for (int i = 0; i < curr->length; i++) {
    if (
          (curr->child[i]->type != BT_FLOAT) &&
          (curr->child[i]->type != BT_INTEGER) &&
          (curr->child[i]->type != BT_SYMBOL) &&
          (curr->child[i]->type != BT_EXPRESSION)
        )
    {
      del_bstate(curr);
      return new_error(BTYPE_ERROR, "unsupported type.");
    }
  }

  beorn_state* x = bpop(curr, 0);
  x = process(benv, x);


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

    if (y->type == BT_EXPRESSION) { y = do_op(benv, y); }
    y = process(benv, y);

    if ((y->type != BT_FLOAT) && (y->type != BT_INTEGER)) {
      if (y->type == BT_ERROR) return y;

      return new_error(BUNSPECTED_TYPE, "Invalid Expression.");
    }

    if (y->type == BT_FLOAT) { tval = BT_FLOAT; }
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

      tval = BT_FLOAT;
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

beorn_state* bb_type_of(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->type != BT_EXPRESSION, BTYPE_ERROR, "expeted expression, example:\n  (type-of 123)");
  BASSERT(exp->length == 1, BTYPE_ERROR, "missing one argument.");
  BASSERT(exp->length > 2,  BTYPE_ERROR, "expected only one argument.");

  beorn_state* r = process(benv, exp->child[1]);
  char* t = btype_to_str(r->type);
  free(r);
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

beorn_state* bb_lambda(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->type != BT_EXPRESSION, BTYPE_ERROR, "expeted expression in lambda function.");
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'set' missing two arguments.");
  BASSERT(exp->length > 3,  BTYPE_ERROR, "expected only two arguments.");
  BASSERT(exp->child[1]->type != BT_LIST, BTYPE_ERROR, "exprected a list of args to lambda function.");
  BASSERT(exp->child[2]->type != BT_PACK, BTYPE_ERROR, "exprected a pack to make body to lambda function.");

  beorn_state* lbd = new_lambda();

  lbd->child[0] = exp->child[1];
  lbd->child[1] = exp->child[2];
  free(exp);

  return lbd;
}

beorn_state* call_function_lambda(beorn_env* benv, beorn_state* func, beorn_state* exp) {
  beorn_state* lfunc = bpop(exp, 0);

  BASSERT(
    lfunc->child[1]->length != exp->length, BTYPE_ERROR, 
    "Lambda Function with number of non-compatible arguments"
  );

  // add lenv
  for (size_t i = 0; i < exp->length; i++) {
    bset_env(lfunc->blenv, lfunc->child[0]->child[i], exp->child[i]);
  }

  beorn_state* res = NULL;
  for (size_t i = 0; i < exp->length; i++) {
    if (res != NULL)
      del_bstate(res);

    res = process(lfunc->blenv, lfunc->child[1]->child[i]);
  }

  if (res == NULL)
    return new_error(BRUNTIME_ERROR, "Empty return of process");

  return res;
}

beorn_state* bb_let(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->type != BT_EXPRESSION, BTYPE_ERROR, "expeted expression, example:\n  (set name 123)");
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'set' missing two arguments.");
  BASSERT(exp->length > 3,  BTYPE_ERROR, "expected only two arguments.");
  BASSERT(exp->child[1]->type != BT_SYMBOL,  BTYPE_ERROR, "invalid argment after 'set'");

  bset_env(exp->blenv, exp->child[1], exp->child[2]);
  beorn_state* pck = new_pack("none");
  pck->closed = 1;
  del_bstate(exp);
  return pck;
}

void put_function_env(beorn_env** benv, char* name, beorn_func fn) {
  beorn_state* fun = new_function(fn);
  bset_env((*benv), new_string(name), fun);
}

void load_buildtin_functions(beorn_env** benv) {
  put_function_env(benv, "+",       do_op);
  put_function_env(benv, "-",       do_op);
  put_function_env(benv, "*",       do_op);
  put_function_env(benv, "/",       do_op);
  put_function_env(benv, "type-of", bb_type_of);
  put_function_env(benv, "set",     bb_set);
  put_function_env(benv, "let",     bb_let);
  put_function_env(benv, "lambda",  bb_lambda);
}

beorn_state* call_func_native(beorn_env* benv, beorn_state* fun, beorn_state* exp) {
  BASSERT(fun->type != BT_FUNCTION, BTYPE_ERROR, "Fail to call function.");

  if (fun->bfunc) {
    return fun->bfunc(benv, exp);
  }

  return new_error(BRUNTIME_ERROR, "fail to call function '%s'.", fun->cval);
}

beorn_state* call_func_builtin(beorn_env* benv, beorn_state* exp) {
  beorn_state* bs = exp->child[0];  

  if (bs->type == BT_SYMBOL) {
    for (int i = 0; i < benv->length; i++) {
      if (strcmp(benv->symbol[i], bs->cval) == 0) {
        return call_func_native(benv, benv->bval[i], exp);
      }
    }
  }

  // lambda
  beorn_state* resolved = process(benv, bs);
  
  if (resolved->type == BT_LAMBDA) {
    return call_function_lambda(benv, resolved, exp);
  }

  beorn_state* err = new_error(BREFERENCE_ERROR, "function '%s' not found.", bs->cval);
  del_bstate(exp);

  return err;
}
