#include <stdlib.h>
#include <string.h>
#include "dfunctions.h"
#include "dvm.h"
#include "dparser.h"
#include "dprint.h"
#include "dio.h"

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

#define bdo_op(op, left, rigth) left op rigth

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
      if (curr->child[i]->type == BT_ERROR) {
        char* msg = curr->child[i]->cval;
        del_bstate(curr);
        return new_error(BTYPE_ERROR, msg);
      } else {
        return new_error(BTYPE_ERROR, "unsupported type.");
      }
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

    if (op == '+') { bdo_op(+=, r, tvl); }
    if (op == '-') { bdo_op(-=, r, tvl); }
    if (op == '*') { bdo_op(*=, r, tvl); }
    if (op == '/') {
      if (tvl == 0) {
        del_bstate(x);
        del_bstate(y);
        x = new_error(BZERO_DIVISION_ERROR, "bad argument in arithmetic expression.");
        break;
      }

      tval = BT_FLOAT;
      bdo_op(/=, r, tvl);
    }

    del_bstate(y);
  }

  del_bstate(curr);

  if (x->type == BT_ERROR) return x;

  x->type = tval;
  if (tval == BT_FLOAT) {
    x->fval = r;
  } else {
    x->ival = (long long) r;
  }

  return x;
}

beorn_state* bb_typeof(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length != 2,  BTYPE_ERROR, "expected one argument.");

  beorn_state* r = process(benv, exp->child[1]);
  const char* t = btype_to_str(r->type);
  free(r);
  free(exp);

  return new_string(t);
}

beorn_state* bb_set(beorn_env* benv, beorn_state* exp) {
  bset_env(benv, exp->child[1], exp->child[2]);
  del_bstate(exp);
  return new_nil();
}

beorn_state* bb_register_function(beorn_env* benv, beorn_state* exp) {
  del_bstate(bpop(exp, 0));
  bregister_env_function(benv, exp);
  del_bstate(exp);
  return new_nil();
}

beorn_state* bb_lambda(beorn_env* benv, beorn_state* exp) {
  beorn_state* lbd = new_lambda(benv);
  
  lbd->length = 2;
  lbd->child[0] = exp->child[1];
  lbd->child[1] = exp->child[2];

  free(exp);

  return lbd;
}

beorn_state* bb_fun(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length > 4,  BRUNTIME_ERROR, "bad definitions, unpected format.");

  return bb_register_function(benv, exp);
}

beorn_state* bcall_runtime_function(beorn_env* benv, beorn_state* func, beorn_state* exp) {
  UNUSED(benv);
  del_bstate(bpop(exp, 0));
  del_bstate(bpop(func, 0));

  BASSERT(
    func->child[0]->length != exp->length, BTYPE_ERROR, 
    "Function with number of non-compatible arguments"
  );

  // add lenv
  for (int i = 0; i < exp->length; i++) {
    bset_env(func->blenv, func->child[0]->child[i], exp->child[i]);
  }

  beorn_state* res = NULL;
  int size = func->child[1]->length;
  for (int i = 0; i < size; i++) {
    if (res != NULL) {
      del_bstate(res);
    }

    res = process(func->blenv, func->child[1]->child[i]);
  }

  if (res == NULL) {
    return new_nil();
  }

  del_benv(func->blenv);
  return res;
}

beorn_state* bb_let(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  bset_env(exp->blenv, exp->child[1], exp->child[2]);
  del_bstate(exp);
  return new_nil();
}

beorn_state* bb_concat(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length != 3,  BTYPE_ERROR, "'cat' expected only two arguments.");
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
  beorn_state* result = new_nil();
  beorn_state* r_exp = NULL;
  if (exp->child[1]->ival) {
    r_exp = bpop(exp, 2);
  } else if (exp->length >= 4) {
    r_exp = bpop(exp, 3);
  }
  
  if (r_exp->type == BT_PACK) {
    for (int i = 0; i < r_exp->length; i++) {
      if (result != NULL) del_bstate(result);

      result = process(benv, r_exp->child[i]);
    }
  } else if (r_exp != NULL) {
    result = process(benv, r_exp);
  } else {
    result = new_nil();
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

      case BT_NIL:
        breturn_and_realease_expr(exp, 1);

      default: breturn_and_realease_expr(exp, 0);
    }

  }

  del_bstate(exp);
  return new_integer(1);
}

beorn_state* bb_double_diff(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  
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
    
      case BT_NIL:
        breturn_and_realease_expr(exp, 0);

    default: breturn_and_realease_expr(exp, 1);
    }

  }

  del_bstate(exp);
  return new_integer(0);
}

beorn_state* bb_equal(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);  
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
  
    case BT_NIL:
      breturn_and_realease_expr(exp, 1);

    default: breturn_and_realease_expr(exp, 0);
  }

  del_bstate(exp);
  return new_integer(1);
}

beorn_state* bb_less(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'<' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  beorn_state* first = exp->child[0];
  bcond_op_default(first, exp, <);
}

beorn_state* bb_less_equal(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'<' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  beorn_state* first = exp->child[0];
  bcond_op_default(first, exp, <=);
}

beorn_state* bb_bigger(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'>' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  beorn_state* first = exp->child[0];
  bcond_op_default(first, exp, >);
}

beorn_state* bb_bigger_equal(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'>' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  beorn_state* first = exp->child[0];
  bcond_op_default(first, exp, >=);
}

beorn_state* bb_diff(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);  
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
  
      case BT_NIL:
        breturn_and_realease_expr(exp, 0);

    default: breturn_and_realease_expr(exp, 1);
  }

  del_bstate(exp);
  return new_integer(0);
}

beorn_state* bb_and(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);  
  del_bstate(bpop(exp, 0));

  beorn_state* left = exp->child[0];
  beorn_state* rigth = exp->child[1];

  int result = (left->ival && rigth->ival);
  del_bstate(exp);
  return new_integer(result);
}

beorn_state* bb_or(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);  
  del_bstate(bpop(exp, 0));

  beorn_state* left = exp->child[0];
  beorn_state* rigth = exp->child[1];

  int result = (left->ival || rigth->ival);
  del_bstate(exp);
  return new_integer(result);
}

beorn_state* bb_print(beorn_env* benv, beorn_state* exp) {
  UNUSED(benv);
  bpop(exp, 0);

  for (int i = 0; i < exp->length; i++)
  {
    bprint_default(process(benv, exp->child[i]), 0);
    bspace_line();
  }
  
  bbreak_line();
  return new_nil();
}

void put_function_env(beorn_env** benv, const char* name, beorn_func fn) {
  beorn_state* fun = new_function(fn);
  bset_env((*benv), new_string(name), fun);
}

beorn_state* bb_import(beorn_env* benv, beorn_state* exp) {
  char * content = 0;
  if(get_file_content(exp->child[1]->cval, &content)) {
    char pm[25];
  
    memcpy(pm, &exp->child[1]->cval[0], 21);
    pm[21] = '.';  pm[22] = '.'; pm[23] = '.';
    pm[24] = '\0';
  
    return new_error(BFILE_NOT_FOUND, "Cannot find file: '%s'", pm);
  }
  beorn_state* out = beorn_parser(content);
  
  __run__(benv, out, 0);

  return new_nil();
}

beorn_state* bb_get(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length != 3, BUNSPECTED_TYPE, "Expected two arguments");
  BASSERT(exp->child[1]->type != BT_LIST, BUNSPECTED_TYPE, "Expected list as argument.");

  UNUSED(benv);
  int idx = exp->child[2]->ival;
  if (idx < 0) {
    idx = exp->child[1]->length + idx;
  }

  if ((idx >= exp->child[1]->length) || (idx < 0)) {
    return new_nil();
  }

  return exp->child[1]->child[idx];
}

beorn_state* bb_put(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length != 3, BUNSPECTED_TYPE, "Expected two arguments");
  BASSERT(exp->child[1]->type != BT_LIST, BUNSPECTED_TYPE, "Expected list as first argument.");
  UNUSED(benv);
  exp->child[1]->length++;
  exp->child[1]->child = (beorn_state**) realloc(exp->child[1]->child, (sizeof(beorn_state*)) * exp->child[1]->length);
  exp->child[1]->child[exp->child[1]->length -1] = exp->child[2];

  return exp->child[1];
}

beorn_state* bb_hd(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length != 2, BUNSPECTED_TYPE, "Expected only one argument");
  BASSERT(exp->child[1]->type != BT_LIST, BUNSPECTED_TYPE, "Expected list as first argument.");
  UNUSED(benv);
  beorn_state* first = bpop(exp->child[1], 0);
  del_bstate(exp->child[1]);
  return first;
}

beorn_state* bb_tl(beorn_env* benv, beorn_state* exp) {
  BASSERT(exp->length != 2, BUNSPECTED_TYPE, "Expected only one argument");
  BASSERT(exp->child[1]->type != BT_LIST, BUNSPECTED_TYPE, "Expected list as argument.");
  UNUSED(benv);
  beorn_state* first = bpop(exp->child[1], 0);
  del_bstate(first);
  return exp->child[1];
}

void load_builtin_functions(beorn_env** benv) {
  beorn_env* native = (*benv)->native;

  put_function_env(&native, "+",       do_op);
  put_function_env(&native, "-",       do_op);
  put_function_env(&native, "*",       do_op);
  put_function_env(&native, "/",       do_op);
  put_function_env(&native, "set",     bb_set);
  put_function_env(&native, "let",     bb_let);
  put_function_env(&native, "fun",     bb_fun);
  put_function_env(&native, "==",      bb_double_equal);
  put_function_env(&native, "!==",     bb_double_diff);
  put_function_env(&native, "=",       bb_equal);
  put_function_env(&native, "!=",      bb_diff);
  put_function_env(&native, "<",       bb_less);
  put_function_env(&native, ">",       bb_bigger);
  put_function_env(&native, "<=",      bb_less_equal);
  put_function_env(&native, ">=",      bb_bigger_equal);
  put_function_env(&native, "and",     bb_and);
  put_function_env(&native, "or",      bb_or);
  put_function_env(&native, "import",  bb_import);
  put_function_env(&native, "if",      bb_if);
  put_function_env(&native, "lambda",  bb_lambda);
  
  // builtin functions
  put_function_env(&native, "concat",  bb_concat);
  put_function_env(&native, "typeof",  bb_typeof);
  put_function_env(&native, "print",   bb_print);
  put_function_env(&native, "get",     bb_get);
  put_function_env(&native, "put",     bb_put);
  put_function_env(&native, "hd",      bb_hd);
  put_function_env(&native, "tl",      bb_tl);
}

beorn_state* bcall_native_function(beorn_env* benv, beorn_state* fun, beorn_state* exp) {
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

beorn_state* bcall_function(beorn_env* benv, beorn_state* exp) {
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
  } 

  /* Call builtn function */
  beorn_env* cenv = get_main_env(benv);

  beorn_state* bres_func = bget_env_value(cenv->native, bs);

  if (NULL != bres_func) {
      return bcall_native_function(benv, bres_func, exp);
  }

  /* Call dynamic function */
  beorn_state* bfun = bget_env_function(benv, exp);

  if (NULL != bfun) {
    bfun->blenv = new_env();
    bfun->blenv->global = benv;
    return bcall_runtime_function(benv, bfun, exp);
  }

  if (benv->global != NULL) {
    return bcall_function(benv->global, exp);
  }
  
  beorn_state* err = new_error(BREFERENCE_ERROR, "function '%s' not found.", bs->cval);
  del_bstate(exp);

  return err;
}
