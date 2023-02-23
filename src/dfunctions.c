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

#define bcond_op_default(first, exp, op)           \
    int result = first->val op exp->child[1]->val; \
    del_bstate(exp);                               \
    return new_integer(result);  

#define bdo_op(op, left, rigth) left op rigth

drax_state* do_op(drax_env* benv, drax_state* curr) {

  drax_state* opr = bpop(curr, 0);
  char op = ((char *) opr->val)[0];
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
        char* msg = (char*) curr->child[i]->val;
        del_bstate(curr);
        return new_error(BTYPE_ERROR, msg);
      } else {
        return new_error(BTYPE_ERROR, "unsupported type.");
      }
    }
  }

  drax_state* x = bpop(curr, 0);
  x = process(benv, x);

  if ((op == '-') && curr->length == 0) {
    x->val = -x->val;
  }

  double r = draxvalue_to_num(x->val);
  types tval = x->type;

  while (curr->length > 0) {
    drax_state* y = bpop(curr, 0);

    if (y->type == BT_EXPRESSION) { y = do_op(benv, y); }
    y = process(benv, y);

    if ((y->type != BT_FLOAT) && (y->type != BT_INTEGER)) {
      if (y->type == BT_ERROR) return y;

      return new_error(BUNSPECTED_TYPE, "Invalid Expression.");
    }

    if (y->type == BT_FLOAT) { tval = BT_FLOAT; }
    double tvl = draxvalue_to_num(y->val);

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
  x->val = num_to_draxvalue(r);

  return x;
}

drax_state* bb_typeof(drax_env* benv, drax_state* exp) {
  BASSERT(exp->length != 2,  BTYPE_ERROR, "expected one argument.");

  drax_state* r = process(benv, exp->child[1]);
  const char* t = btype_to_str(r->type);
  free(r);
  free(exp);

  return new_string(t);
}

drax_state* bb_set(drax_env* benv, drax_state* exp) {
  bset_env(benv, exp->child[1], process(benv, exp->child[2]));
  del_bstate(exp);
  return new_nil();
}

drax_state* bb_let(drax_env* benv, drax_state* exp) {
  bset_env(exp->blenv, exp->child[1], process(benv, exp->child[2]));
  del_bstate(exp);
  return new_nil();
}

drax_state* bb_register_function(drax_env* benv, drax_state* exp) {
  del_bstate(bpop(exp, 0));
  bregister_env_function(benv, exp);
  del_bstate(exp);
  return new_nil();
}

drax_state* bb_lambda(drax_env* benv, drax_state* exp) {
  drax_state* lbd = new_lambda(benv);
  
  lbd->length = 2;
  lbd->child[0] = exp->child[1];
  lbd->child[1] = exp->child[2];

  free(exp);

  return lbd;
}

drax_state* bb_fun(drax_env* benv, drax_state* exp) {
  BASSERT(exp->length > 4,  BRUNTIME_ERROR, "bad definitions, unpected format.");

  return bb_register_function(benv, exp);
}

drax_state* bcall_runtime_function(drax_env* benv, drax_state* func, drax_state* exp) {
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

  drax_state* res = NULL;
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

drax_state* bb_concat(drax_env* benv, drax_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length != 3,  BTYPE_ERROR, "'cat' expected only two arguments.");
  BASSERT(exp->child[1]->type != BT_STRING,  BTYPE_ERROR, "'cat' expects string only");

  del_bstate(bpop(exp, 0));
  
  char* lstr = (char*) exp->child[0]->val;
  char* rstr = (char*) exp->child[1]->val;

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

drax_state* bb_if(drax_env* benv, drax_state* exp) {  
  drax_state* result = new_nil();
  drax_state* r_exp = NULL;
  if (exp->child[1]->val) {
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

drax_state* bb_double_equal(drax_env* benv, drax_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length <= 1, BTYPE_ERROR, "'==' missing at least one argument.");
  
  del_bstate(bpop(exp, 0));

  drax_state* first = exp->child[0];

  for (int i = 1; i < exp->length; i++)
  {
    if (exp->child[i]->type != first->type) {
      del_bstate(exp);
      return new_integer(0);
    }
    
    switch (exp->child[i]->type)
    {
      case BT_INTEGER:
      case BT_FLOAT:
        if (exp->child[i]->val != first->val)
          breturn_and_realease_expr(exp, 0);

      case BT_STRING:
        if (strcmp((char*) exp->child[i]->val, (char*) first->val) != 0)
          breturn_and_realease_expr(exp, 0);

      case BT_NIL:
        breturn_and_realease_expr(exp, 1);

      default: breturn_and_realease_expr(exp, 0);
    }

  }

  del_bstate(exp);
  return new_integer(1);
}

drax_state* bb_double_diff(drax_env* benv, drax_state* exp) {
  UNUSED(benv);
  
  del_bstate(bpop(exp, 0));

  drax_state* first = exp->child[0];

  for (int i = 1; i < exp->length; i++)
  {
    if (exp->child[i]->type != first->type) {
      del_bstate(exp);
      return new_integer(1);
    }
    
    switch (exp->child[i]->type)
    {
      case BT_INTEGER:
      case BT_FLOAT: 
        if (exp->child[i]->val != first->val)
          breturn_and_realease_expr(exp, 1);

      case BT_STRING: 
        if (strcmp((char*) exp->child[i]->val, (char*) first->val) != 0)
          breturn_and_realease_expr(exp, 1);
    
      case BT_NIL:
        breturn_and_realease_expr(exp, 0);

    default: breturn_and_realease_expr(exp, 1);
    }

  }

  del_bstate(exp);
  return new_integer(0);
}

drax_state* bb_equal(drax_env* benv, drax_state* exp) {
  UNUSED(benv);  
  del_bstate(bpop(exp, 0));

  drax_state* first = exp->child[0];

  if (exp->child[1]->type != first->type) {
    del_bstate(exp);
    return new_integer(0);
  }
  
  switch (first->type)
  {
    case BT_INTEGER:
    case BT_FLOAT: 
      if (exp->child[1]->val != first->val)
        breturn_and_realease_expr(exp, 0);

    case BT_STRING: 
      if (strcmp((char*) exp->child[1]->val, (char*) first->val) != 0)
        breturn_and_realease_expr(exp, 0);
  
    case BT_NIL:
      breturn_and_realease_expr(exp, 1);

    default: breturn_and_realease_expr(exp, 0);
  }

  del_bstate(exp);
  return new_integer(1);
}

drax_state* bb_system(drax_env* benv, drax_state* exp) {
  UNUSED(benv);
  BASSERT(exp->length != 2,  BTYPE_ERROR, "'system' expected only one arguments.");
  BASSERT(exp->child[1]->type != BT_STRING,  BTYPE_ERROR, "'system' expects string only");

  del_bstate(bpop(exp, 0));
  
  char* lstr = (char*) exp->child[0]->val;

  int out = system(lstr);
  del_bstate(exp);

  return new_integer(out);
}

drax_state* bb_less(drax_env* benv, drax_state* exp) {
  UNUSED(benv);
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'<' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  drax_state* first = exp->child[0];
  bcond_op_default(first, exp, <);
}

drax_state* bb_less_equal(drax_env* benv, drax_state* exp) {
  UNUSED(benv);
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'<' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  drax_state* first = exp->child[0];
  bcond_op_default(first, exp, <=);
}

drax_state* bb_bigger(drax_env* benv, drax_state* exp) {
  UNUSED(benv);
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'>' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  drax_state* first = exp->child[0];
  bcond_op_default(first, exp, >);
}

drax_state* bb_bigger_equal(drax_env* benv, drax_state* exp) {
  UNUSED(benv);
  BASSERT((exp->child[1]->type != BT_INTEGER) && (exp->child[1]->type != BT_FLOAT), BTYPE_ERROR, "'>' not supported to type.");
  
  del_bstate(bpop(exp, 0));
  drax_state* first = exp->child[0];
  bcond_op_default(first, exp, >=);
}

drax_state* bb_diff(drax_env* benv, drax_state* exp) {
  UNUSED(benv);  
  del_bstate(bpop(exp, 0));

  drax_state* first = exp->child[0];

  if (exp->child[1]->type != first->type) {
    del_bstate(exp);
    return new_integer(1);
  }
  
  switch (first->type)
  {
    case BT_INTEGER:
    case BT_FLOAT: 
      if (exp->child[1]->val != first->val)
        breturn_and_realease_expr(exp, 1);

    case BT_STRING: 
      if (strcmp((char*) exp->child[1]->val, (char*)  first->val) != 0)
        breturn_and_realease_expr(exp, 1);
  
      case BT_NIL:
        breturn_and_realease_expr(exp, 0);

    default: breturn_and_realease_expr(exp, 1);
  }

  del_bstate(exp);
  return new_integer(0);
}

drax_state* bb_and(drax_env* benv, drax_state* exp) {
  UNUSED(benv);  
  del_bstate(bpop(exp, 0));

  drax_state* left = exp->child[0];
  drax_state* rigth = exp->child[1];

  int result = (left->val && rigth->val);
  del_bstate(exp);
  return new_integer(result);
}

drax_state* bb_or(drax_env* benv, drax_state* exp) {
  UNUSED(benv);  
  del_bstate(bpop(exp, 0));

  drax_state* left = exp->child[0];
  drax_state* rigth = exp->child[1];

  int result = (left->val || rigth->val);
  del_bstate(exp);
  return new_integer(result);
}

drax_state* bb_print(drax_env* benv, drax_state* exp) {
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

void put_function_env(drax_env** benv, const char* name, drax_func fn) {
  drax_state* fun = new_function(fn);
  bset_env((*benv), new_string(name), fun);
}

drax_state* bb_import(drax_env* benv, drax_state* exp) {
  char * content = 0;
  if(get_file_content((char*) exp->child[1]->val, &content)) {
    char pm[25];
  
    memcpy(pm, &(((char*) exp->child[1]->val)[0]), 21);
    pm[21] = '.';  pm[22] = '.'; pm[23] = '.';
    pm[24] = '\0';
  
    return new_error(BFILE_NOT_FOUND, "Cannot find file: '%s'", pm);
  }
  drax_state* out = drax_parser(content);
  
  __run__(benv, out, 0);

  return new_nil();
}

drax_state* bb_get(drax_env* benv, drax_state* exp) {
  BASSERT(exp->length != 3, BUNSPECTED_TYPE, "Expected two arguments");
  BASSERT(exp->child[1]->type != BT_LIST, BUNSPECTED_TYPE, "Expected list as argument.");

  UNUSED(benv);
  int idx = exp->child[2]->val;
  if (idx < 0) {
    idx = exp->child[1]->length + idx;
  }

  if ((idx >= exp->child[1]->length) || (idx < 0)) {
    return new_nil();
  }

  return exp->child[1]->child[idx];
}

drax_state* bb_put(drax_env* benv, drax_state* exp) {
  BASSERT(exp->length != 3, BUNSPECTED_TYPE, "Expected two arguments");
  BASSERT(exp->child[1]->type != BT_LIST, BUNSPECTED_TYPE, "Expected list as first argument.");
  UNUSED(benv);
  exp->child[1]->length++;
  exp->child[1]->child = (drax_state**) realloc(exp->child[1]->child, (sizeof(drax_state*)) * exp->child[1]->length);
  exp->child[1]->child[exp->child[1]->length -1] = exp->child[2];

  return exp->child[1];
}

drax_state* bb_hd(drax_env* benv, drax_state* exp) {
  BASSERT(exp->length != 2, BUNSPECTED_TYPE, "Expected only one argument");
  BASSERT(exp->child[1]->type != BT_LIST, BUNSPECTED_TYPE, "Expected list as first argument.");
  UNUSED(benv);
  drax_state* first = bpop(exp->child[1], 0);
  del_bstate(exp->child[1]);
  return first;
}

drax_state* bb_tl(drax_env* benv, drax_state* exp) {
  BASSERT(exp->length != 2, BUNSPECTED_TYPE, "Expected only one argument");
  BASSERT(exp->child[1]->type != BT_LIST, BUNSPECTED_TYPE, "Expected list as argument.");
  UNUSED(benv);
  drax_state* first = bpop(exp->child[1], 0);
  del_bstate(first);
  return exp->child[1];
}

void load_builtin_functions(drax_env** benv) {
  drax_env* native = (*benv)->native;

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

  put_function_env(&native, "set",     bb_set); // global
  put_function_env(&native, "let",     bb_let);
  put_function_env(&native, "fun",     bb_fun);

  put_function_env(&native, "and",     bb_and);
  put_function_env(&native, "or",      bb_or);
  put_function_env(&native, "if",      bb_if);

  put_function_env(&native, "import",  bb_import);
  put_function_env(&native, "lambda",  bb_lambda);
  
  // builtin functions
  put_function_env(&native, "concat",  bb_concat);
  put_function_env(&native, "typeof",  bb_typeof);
  put_function_env(&native, "print",   bb_print);
  put_function_env(&native, "get",     bb_get);
  put_function_env(&native, "put",     bb_put);
  put_function_env(&native, "hd",      bb_hd);
  put_function_env(&native, "tl",      bb_tl);

  put_function_env(&native, "system",  bb_system);
}

drax_state* bcall_native_function(drax_env* benv, drax_state* fun, drax_state* exp) {
  BASSERT(fun->type != BT_FUNCTION, BTYPE_ERROR, "Fail to call function.");

  if (fun->bfunc) {
    return fun->bfunc(benv, exp);
  }

  return new_error(BRUNTIME_ERROR, "fail to call function '%s'.", (char*) fun->val);
}

drax_env* get_main_env(drax_env* benv) {
  drax_env* cenv = benv;
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

drax_state* bcall_function(drax_env* benv, drax_state* exp) {
  drax_state* bs = exp->child[0];

  for (int i = 0; i < exp->length; i++) {
    if (exp->child[i]->type == BT_EXPRESSION) {
      exp->child[i] = process(benv, exp->child[i]);
    }
  }

  if (bs->type == BT_SYMBOL) {
    for (int i = 1; i < exp->length; i++) {
      if (!block_process((char*) bs->val))
        exp->child[i] = process(benv, exp->child[i]);
    }
  } 

  /* Call builtn function */
  drax_env* cenv = get_main_env(benv);

  drax_state* bres_func = bget_env_value(cenv->native, bs);

  if (NULL != bres_func) {
      return bcall_native_function(benv, bres_func, exp);
  }

  /* Call dynamic function */
  drax_state* bfun = bget_env_function(benv, exp);

  if (NULL != bfun) {
    bfun->blenv = new_env();
    bfun->blenv->global = benv;
    return bcall_runtime_function(benv, bfun, exp);
  }

  if (benv->global != NULL) {
    return bcall_function(benv->global, exp);
  }
  
  drax_state* err = new_error(BREFERENCE_ERROR, "function '%s' not found.", (char*) bs->val);
  del_bstate(exp);

  return err;
}
