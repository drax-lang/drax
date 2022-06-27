#include <stdlib.h>
#include <string.h>
#include "bfunctions.h"
#include "bvm.h"
#include "bparser.h"
#include "bprint.h"
#include "bio.h"

#define breturn_and_realease_expr(exp, bool_op) { \
  del_bstate(exp);                                \
    return new_integer(bool_op);                  \
  }                                               \
  break;

#define bcond_op_default(first, exp, op)             \
    int result = 0;                                  \
    switch (first->type) {                           \
    case BT_INTEGER:                                 \
      if (exp->child[1]->type == BT_INTEGER) {       \
        result = first->ival op exp->child[1]->ival; \
      } else {                                       \
        result = first->ival op exp->child[1]->fval; \
      } breturn_and_realease_expr(exp, result);      \
    case BT_FLOAT:                                   \
      if (exp->child[1]->type == BT_FLOAT) {         \
        result = first->fval op exp->child[1]->fval; \
      } else {                                       \
        result = first->fval op exp->child[1]->ival; \
      } breturn_and_realease_expr(exp, result);      \
    default: breturn_and_realease_expr(exp, 0); }    \


beorn_state* bpop(beorn_state* curr, int i){
  beorn_state* ele = curr->child[i];

  memmove(&curr->child[i], &curr->child[i+1],
    sizeof(beorn_state*) * (curr->length -i -1));

  curr->length--;
  curr->child = (beorn_state**) realloc(curr->child, sizeof(beorn_state*) * curr->length);
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
  types tval = x->type;

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

beorn_state* bb_typeof(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length == 1, BTYPE_ERROR, "missing one argument.");
  BASSERT(exp->length > 2,  BTYPE_ERROR, "expected only one argument.");

  beorn_state* r = process(benv, exp->child[1]);
  const char* t = btype_to_str(r->type);
  free(r);
  free(exp);

  return new_string(t);
}

beorn_state* bb_set(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'set' missing two arguments.");
  BASSERT(exp->length > 3,  BTYPE_ERROR, "expected only two arguments.");
  BASSERT(exp->child[1]->type != BT_SYMBOL,  BTYPE_ERROR, "invalid argment after 'set'");

  bset_env(benv, exp->child[1], exp->child[2]);
  beorn_state* pck = new_pack("");
  pck->closed = 1;
  del_bstate(exp);
  return pck;
}

beorn_state* bb_lambda(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'set' missing two arguments.");
  BASSERT(exp->length > 3,  BTYPE_ERROR, "expected only two arguments.");
  BASSERT(exp->child[1]->type != BT_LIST, BTYPE_ERROR, "exprected a list of args to lambda function.");
  BASSERT(exp->child[2]->type != BT_PACK, BTYPE_ERROR, "exprected a pack to make body to lambda function.");

  beorn_state* lbd = new_lambda(benv);
  
  lbd->length = 2;
  lbd->child[0] = exp->child[1];
  lbd->child[1] = exp->child[2];

  free(exp);

  return lbd;
}

beorn_state* bb_fun(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length <= 3, BTYPE_ERROR, "'set' missing two arguments.");
  BASSERT(exp->length > 4,  BTYPE_ERROR, "expected only two arguments.");
  BASSERT(exp->child[1]->type != BT_SYMBOL, BTYPE_ERROR, "exprected a symbol to define function name.");
  BASSERT(exp->child[2]->type != BT_LIST, BTYPE_ERROR, "exprected a list of args to function.");
  BASSERT(exp->child[3]->type != BT_PACK, BTYPE_ERROR, "exprected a pack to make body to function.");

  beorn_state* lbd = new_lambda(benv);
  
  lbd->length = 2;
  lbd->child[0] = exp->child[2];
  lbd->child[1] = exp->child[3];

  beorn_state* expfun = new_expression("none");
  expfun->child = (beorn_state**) malloc(sizeof(beorn_state) * 2);
  expfun->length = 3;
  expfun->child[0] = new_symbol("set");
  expfun->child[1] = exp->child[1];
  expfun->child[2] = lbd;

  beorn_state* def = bb_set(benv, expfun);

  free(exp);

  return def;
}

beorn_state* call_function_lambda(beorn_env* benv, beorn_state* func, beorn_state* exp) {
  UNUSED(benv);
  beorn_state* lfunc = bpop(exp, 0);
  del_bstate(lfunc);

  BASSERT(
    func->child[0]->length != exp->length, BTYPE_ERROR, 
    "Lambda Function with number of non-compatible arguments"
  );

  // add lenv
  for (int i = 0; i < exp->length; i++) {
    bset_env(func->blenv, func->child[0]->child[i], exp->child[i]);
  }

  beorn_state* res = NULL;
  for (int i = 0; i < func->child[1]->length; i++) {
    if (res != NULL)
      del_bstate(res);

    res = process(func->blenv, func->child[1]->child[i]);
  }

  if (res == NULL)
    return new_error(BRUNTIME_ERROR, "Empty return of process");

  return res;
}

beorn_state* bb_let(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'set' missing two arguments.");
  BASSERT(exp->length > 3,  BTYPE_ERROR, "expected only two arguments.");
  BASSERT(exp->child[1]->type != BT_SYMBOL,  BTYPE_ERROR, "invalid argment after 'set'");

  bset_env(exp->blenv, exp->child[1], exp->child[2]);
  beorn_state* pck = new_pack("none");
  pck->closed = 1;
  del_bstate(exp);
  return pck;
}

beorn_state* bb_cat(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'cat' missing two arguments.");
  BASSERT(exp->length > 3,  BTYPE_ERROR, "'cat' expected only two arguments.");
  BASSERT(exp->child[1]->type != BT_STRING,  BTYPE_ERROR, "'cat' expects string only");

  del_bstate(bpop(exp, 0));
  
  char* lstr = exp->child[0]->cval;
  char* rstr = exp->child[1]->cval;

  size_t ls = strlen(lstr);
  size_t rs = strlen(rstr);
  char* result = (char*) malloc(sizeof(char) * (ls + rs + 1));
  
  strncpy(result, lstr, ls);
  result[ls] = '\0';
  strncat(result, rstr, rs);
  result[ls + rs] = '\0';

  del_bstate(exp);

  return new_string(result);
}

beorn_state* bb_if(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length <= 3, BTYPE_ERROR, "'if' missing two arguments.");
  BASSERT(exp->length > 4,  BTYPE_ERROR, "expected only two or three arguments.");
  BASSERT(exp->child[1]->type != BT_INTEGER, BTYPE_ERROR, "'if' with invalid argument");
  
  beorn_state* result = new_pack("");
  beorn_state* r_exp = NULL;
  if (exp->child[1]->ival) {
    r_exp = bpop(exp, 2);
  } else if (exp->length >= 4) {
    r_exp = bpop(exp, 3);
  }
  
  if (r_exp->type == BT_PACK) {
    for (int i = 0; i < r_exp->length; i++) {
      if (result != NULL) del_bstate(result);

      beorn_state* tmp = bpop(r_exp, i);
      result = process(benv, tmp);
    }
  } else if (r_exp != NULL) {
    result = process(benv, r_exp);
  } else {
    result = new_pack("");
  }

  del_bstate(exp);
  return result;
}

beorn_state* bb_double_equal(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 1, BTYPE_ERROR, "'==' missing at least one argument.");
  
  del_bstate(bpop(exp, 0));

  beorn_state* first = exp->child[0];

  for (int i = 1; i < exp->length; i++)
  {
    if (exp->child[i]->type != first->type) {
      del_bstate(exp);
      return new_integer(0);
    }
    
    switch (exp->child[i]->type)
    {
      case BT_INTEGER: 
        if (exp->child[i]->ival != first->ival)
          breturn_and_realease_expr(exp, 0);

      case BT_FLOAT: 
        if (exp->child[i]->fval != first->fval)
          breturn_and_realease_expr(exp, 0);

      case BT_STRING: 
        if (strcmp(exp->child[i]->cval, first->cval) != 0)
          breturn_and_realease_expr(exp, 0);
    
    default: breturn_and_realease_expr(exp, 0);
    }

  }

  del_bstate(exp);
  return new_integer(1);
}

beorn_state* bb_double_diff(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 1, BTYPE_ERROR, "'!==' missing at least one argument.");
  
  del_bstate(bpop(exp, 0));

  beorn_state* first = exp->child[0];

  for (int i = 1; i < exp->length; i++)
  {
    if (exp->child[i]->type != first->type) {
      del_bstate(exp);
      return new_integer(1);
    }
    
    switch (exp->child[i]->type)
    {
      case BT_INTEGER: 
        if (exp->child[i]->ival != first->ival)
          breturn_and_realease_expr(exp, 1);

      case BT_FLOAT: 
        if (exp->child[i]->fval != first->fval)
          breturn_and_realease_expr(exp, 1);

      case BT_STRING: 
        if (strcmp(exp->child[i]->cval, first->cval) != 0)
          breturn_and_realease_expr(exp, 1);
    
    default: breturn_and_realease_expr(exp, 1);
    }

  }

  del_bstate(exp);
  return new_integer(0);
}

beorn_state* bb_equal(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'=' missing at least two argument.");
  BASSERT(exp->length > 3, BTYPE_ERROR, "'=' waits only two arguments.");
  
  del_bstate(bpop(exp, 0));

  beorn_state* first = exp->child[0];

  if (exp->child[1]->type != first->type) {
    del_bstate(exp);
    return new_integer(0);
  }
  
  switch (first->type)
  {
    case BT_INTEGER: 
      if (exp->child[1]->ival != first->ival)
        breturn_and_realease_expr(exp, 0);

    case BT_FLOAT: 
      if (exp->child[1]->fval != first->fval)
        breturn_and_realease_expr(exp, 0);

    case BT_STRING: 
      if (strcmp(exp->child[1]->cval, first->cval) != 0)
        breturn_and_realease_expr(exp, 0);
  
    default: breturn_and_realease_expr(exp, 0);
  }

  del_bstate(exp);
  return new_integer(1);
}

beorn_state* bb_less(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'<' missing at least two argument.");
  BASSERT(exp->length > 3, BTYPE_ERROR, "'<' waits only two arguments.");
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'<' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  beorn_state* first = exp->child[0];
  bcond_op_default(first, exp, <);
}

beorn_state* bb_less_equal(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'<=' missing at least two argument.");
  BASSERT(exp->length > 3, BTYPE_ERROR, "'<=' waits only two arguments.");
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'<' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  beorn_state* first = exp->child[0];
  bcond_op_default(first, exp, <=);
}

beorn_state* bb_bigger(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'>' missing at least two argument.");
  BASSERT(exp->length > 3, BTYPE_ERROR, "'>' waits only two arguments.");
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'>' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  beorn_state* first = exp->child[0];
  bcond_op_default(first, exp, >);
}

beorn_state* bb_bigger_equal(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'>=' missing at least two argument.");
  BASSERT(exp->length > 3, BTYPE_ERROR, "'>=' waits only two arguments.");
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'>' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  beorn_state* first = exp->child[0];
  bcond_op_default(first, exp, >=);
}

beorn_state* bb_diff(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 2, BTYPE_ERROR, "'!=' missing at least one argument.");
  BASSERT(exp->length > 3, BTYPE_ERROR, "'!=' waits only two arguments.");
  
  del_bstate(bpop(exp, 0));

  beorn_state* first = exp->child[0];

  if (exp->child[1]->type != first->type) {
    del_bstate(exp);
    return new_integer(1);
  }
  
  switch (first->type)
  {
    case BT_INTEGER: 
      if (exp->child[1]->ival != first->ival)
        breturn_and_realease_expr(exp, 1);

    case BT_FLOAT: 
      if (exp->child[1]->fval != first->fval)
        breturn_and_realease_expr(exp, 1);

    case BT_STRING: 
      if (strcmp(exp->child[1]->cval, first->cval) != 0)
        breturn_and_realease_expr(exp, 1);
  
    default: breturn_and_realease_expr(exp, 1);
  }

  del_bstate(exp);
  return new_integer(0);
}

beorn_state* bb_print(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  bpop(exp, 0);

  for (int i = 0; i < exp->length; i++)
  {
    bprint(process(benv, exp->child[i]));
    bspace_line();
  }
  
  bbreak_line();
  return new_pack("");
}

void put_function_env(beorn_env** benv, const char* name, beorn_func fn) {
  beorn_state* fun = new_function(fn);
  bset_env((*benv), new_string(name), fun);
}

beorn_state* bb_import(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length == 1, BTYPE_ERROR, "missing path.");
  BASSERT(exp->child[1]->type != BT_STRING,  BTYPE_ERROR, "import with invalid path.");

  char * content = 0;
  if(get_file_content(exp->child[1]->cval, &content)) {
    char pm[25];
  
    memcpy(pm, &exp->child[1]->cval[0], 21);
    pm[21] = '.';  pm[22] = '.'; pm[23] = '.';
    pm[24] = '\0';
  
    return new_error(BFILE_NOT_FOUND, "Cannot find file: '%s'", pm);
  }

  beorn_state* out = beorn_parser(content);

  if (out->type == BT_ERROR) {
    bprint(out);
    bbreak_line();
  } else {
    for (int i = 0; i < out->length; i++) {
        beorn_state* evaluated = process(benv, out->child[i]);
        if (evaluated->type == BT_ERROR) bprint(evaluated);
    }
    del_bstate(out);
  }

  return new_pack("");
}

void load_buildtin_functions(beorn_env** benv) {
  beorn_env* native = (*benv)->native;

  put_function_env(&native, "set",     bb_set);
  put_function_env(&native, "let",     bb_let);
  put_function_env(&native, "fun",     bb_fun);
  put_function_env(&native, "cat",     bb_cat);

  put_function_env(&native, "+",       do_op);
  put_function_env(&native, "-",       do_op);
  put_function_env(&native, "*",       do_op);
  put_function_env(&native, "/",       do_op);

  put_function_env(&native, "==",      bb_double_equal);
  put_function_env(&native, "!==",     bb_double_diff);
  
  put_function_env(&native, "=",       bb_equal);
  put_function_env(&native, "!=",      bb_diff);
  put_function_env(&native, "<",       bb_less);
  put_function_env(&native, ">",       bb_bigger);
  put_function_env(&native, "<=",      bb_less_equal);
  put_function_env(&native, ">=",      bb_bigger_equal);

  put_function_env(&native, "typeof",  bb_typeof);
  put_function_env(&native, "lambda",  bb_lambda);
  put_function_env(&native, "if",      bb_if);
  put_function_env(&native, "print",   bb_print);
  put_function_env(&native, "import",  bb_import);
  
}

beorn_state* call_func_native(beorn_env* benv, beorn_state* fun, beorn_state* exp) {
  BASSERT(fun->type != BT_FUNCTION, BTYPE_ERROR, "Fail to call function.");

  if (fun->bfunc) {
    return fun->bfunc(benv, exp);
  }

  return new_error(BRUNTIME_ERROR, "fail to call function '%s'.", fun->cval);
}

beorn_env* get_main_env(beorn_env* benv) {
  beorn_env* cenv = benv;
  while (cenv->global != NULL) {
    cenv = benv->global;
  }
  return cenv;
}

int block_process(char* fun_n) {
  return (
    (strcmp(fun_n, "set") == 0) ||
    (strcmp(fun_n, "let") == 0) ||
    (strcmp(fun_n, "fun") == 0) ||
    (strcmp(fun_n, "import") == 0)
  );
}

beorn_state* call_func_builtin(beorn_env* benv, beorn_state* exp) {
  beorn_state* bs = exp->child[0];

  for (int i = 0; i < exp->length; i++) {
    if (exp->child[i]->type == BT_EXPRESSION) {
      exp->child[i] = process(benv, exp->child[i]);
    }
  }

  if (bs->type == BT_SYMBOL) {
    for (int i = 1; i < exp->length; i++) {
      if (!block_process(bs->cval))
        exp->child[i] = process(benv, exp->child[i]);
    }    

    beorn_env* cenv = get_main_env(benv);
    for (int i = 0; i < cenv->native->length; i++) {
      if (strcmp(cenv->native->symbol[i], bs->cval) == 0) {
        return call_func_native(benv, cenv->native->bval[i], exp);
      }
    }
  }

  // lambda
  beorn_state* resolved = process(benv, bs);

  if (resolved->type == BT_LAMBDA) {
    return call_function_lambda(benv, resolved, exp);
  }

  if (benv->global != NULL)
    return call_func_builtin(benv->global, exp);
  
  beorn_state* err = new_error(BREFERENCE_ERROR, "function '%s' not found.", bs->cval);
  del_bstate(resolved);
  del_bstate(exp);

  return err;
}
