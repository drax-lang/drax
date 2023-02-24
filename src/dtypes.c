#include <string.h>
#include <stdlib.h>
#include "stdio.h"
#include <stdarg.h>
#include "dtypes.h"

/* Types implementations */

static drax_state* new_drax_state() {
  drax_state* v = (drax_state*) malloc(sizeof(drax_state));
  v->length = 0;
  v->child = NULL;
  v->trace = NULL;
  v->act = BACT_NONE;

  return v;
}

drax_state* new_error(berrors_type t, const char* s, ...) {
  drax_state* v = new_drax_state();
  v->type = BT_ERROR;
  v->val = (drax_value) calloc(strlen(s) + 1, sizeof(char));
  v->et = t;
  va_list va;
  va_start(va, s);

  vsnprintf((char*) v->val, 511, s, va);
  va_end(va);
  return v;
}

drax_state* new_integer(double fv) {
  drax_state* v = new_drax_state();
  v->type = BT_INTEGER;
  v->val = num_to_draxvalue(fv);
  v->closed = 1;
  return v;
}

drax_state* new_float(double fv) {
  drax_state* v = new_drax_state();
  v->type = BT_FLOAT;
  v->val = num_to_draxvalue(fv);
  v->closed = 1;
  return v;
}

drax_state* new_string(const char* s) {
  int strsize = 0;
  
  if (NULL != s)
    strsize = strlen(s);

  drax_state* v = new_drax_state();
  v->type = BT_STRING;
  v->val = (drax_value) calloc(sizeof(char), strsize + 1);
  v->closed = 1;

  if (NULL != s)
    strcpy((char*) v->val, s);
    
  return v;
}

drax_state* new_symbol(const char* s) {
  drax_state* v = new_drax_state();
  v->type = BT_SYMBOL;

  if (NULL != s) {
    v->val = (drax_value) calloc(sizeof(char), strlen(s) + 1);
    strcpy((char *) v->val, s);
  }

  v->closed = 1;
  return v;
}

drax_state* new_pack() {
  drax_state* v = new_drax_state();
  v->type = BT_PACK;
  v->val = (drax_value) NULL;
  v->closed = 0;
  return v;
}

drax_state* new_expression() {
  drax_state* v = new_drax_state();
  v->blenv = new_env();
  v->type = BT_EXPRESSION;
  v->val = (drax_value) NULL;
  v->closed = 0;
  return v;
}

drax_state* new_list() {
  drax_state* v = new_drax_state();
  v->blenv = NULL;
  v->type = BT_LIST;
  v->val = (drax_value) NULL;
  v->closed = 0;
  return v;
}

drax_state* new_function(drax_func fn) {
  drax_state* v = new_drax_state();
  v->type = BT_FUNCTION;
  v->bfunc = fn;
  v->child = NULL;
  v->act = BACT_NONE;
  return v;
};

drax_state* new_lambda(drax_env* global) {
  drax_state* v = new_drax_state();
  v->type = BT_LAMBDA;
  v->bfunc = NULL;
  v->blenv = new_env();
  v->blenv->global = global;
  v->child = (drax_state**) malloc(sizeof(drax_state*) * 2);
  return v;
};

drax_state* new_nil() {
  drax_state* v = new_drax_state();
  v->type = BT_NIL;
  v->blenv = NULL;
  v->val = (drax_value) NULL;
  v->closed = 0;
  return v;
}

void del_bstate(drax_state* curr) {

  if (curr == NULL) return;

  switch (curr->type) {
    case BT_PACK:
    case BT_LIST:
    case BT_EXPRESSION:
      for (int i = 0; i < curr->length; i++) {
        del_bstate(curr->child[i]);
      }

    default: {
      break;
    }
  }

  free(curr);
}

const char* berrors_to_str(berrors_type t) {
  switch (t) {
    case BSYNTAX_ERROR: return "SYNTAX_ERROR";
    case BTYPE_ERROR: return "TYPE_ERROR";
    case BREFERENCE_ERROR: return "REFERENCE_ERROR";
    case BUNKNOWN_TYPE_ERROR: return "UNKNOWN_TYPE_ERROR";
    case BZERO_DIVISION_ERROR: return "ZERO_DIVISION_ERROR";
    case BUNSPECTED_TYPE: return "UNSPECTED_TYPE";
    default: return "ERROR";
  }
}

const char* btype_to_str(types t) {
  switch (t) {
    case BT_INTEGER:    return "Integer";
    case BT_FLOAT:      return "Float";
    case BT_STRING:     return "String";
    case BT_PACK:       return "Pack";
    case BT_SYMBOL:     return "Symbol";
    case BT_EXPRESSION: return "Expression";
    case BT_LIST:       return "List";
    case BT_NIL:        return "Nil";
    case BT_ERROR:      return "Error";
    default:            return "Unknown";
  }
}

drax_state* bcopy_state(drax_state* v) {
  drax_state* x = (drax_state*) malloc(sizeof(drax_state));
  x->type = v->type;
  switch (v->type) {
    break;
    case BT_INTEGER: x->val = v->val; break;
    case BT_FLOAT: x->val = v->val; break;
    case BT_ERROR:
      x->et = v->et;
      x->val = (drax_value) malloc((strlen((char*) v->val) + 1) * sizeof(char));
      strcpy((char*) x->val, (char*) v->val);
    break;
    case BT_STRING:
    case BT_SYMBOL:
      x->val = (drax_value) malloc((strlen((char*) v->val) + 1) * sizeof(char));
      strcpy((char*) x->val, (char*) v->val);
      x->act = v->act;
    break;

    case BT_FUNCTION:
      x->blenv = v->blenv;
      x->bfunc = v->bfunc;
      break;

    case BT_PACK:
    case BT_LIST:
    case BT_LAMBDA:
    case BT_EXPRESSION:
      x->length = v->length;
      x->blenv = v->blenv;
      x->child = (drax_state**) malloc(sizeof(drax_state*) * x->length);
      for (int i = 0; i < x->length; i++) {
        x->child[i] = bcopy_state(v->child[i]);
      }
    break;
    default: ;
  }
  return x;
}

drax_state* bpop(drax_state* curr, int i){
  drax_state* ele = curr->child[i];

  memmove(&curr->child[i], &curr->child[i+1],
    sizeof(drax_state*) * (curr->length -i -1));

  curr->length--;
  curr->child = (drax_state**) realloc(curr->child, sizeof(drax_state*) * curr->length);
  return ele;
}

/* Environments configs */

static size_t gen_hash_idx(size_t cap, char* key) {
    size_t idx;
    for (idx = 0; *key != '\0'; key++) {
        idx = *key + 31 * idx;
    }
    return idx % (cap);
}

static int init_environment(drax_env* e) {
  e->bval = (bvar_hashs*) malloc(sizeof(bvar_hashs));
  e->bval->cap = BENV_HASH_SIZE;
  e->bval->vars = (bvars_pair**) malloc(sizeof(bvars_pair *) * BENV_HASH_SIZE);

  e->bfuncs = (bfunc_hashs*) malloc(sizeof(bfunc_hashs));
  e->bfuncs->cap = BENV_HASH_SIZE;
  e->bfuncs->funs = (bfunc_pair**) malloc(sizeof(bfunc_pair *) * BENV_HASH_SIZE);

  for (size_t i = 0; i < BENV_HASH_SIZE; i++) {
    e->bval->vars[i] = NULL;
    e->bfuncs->funs[i] = NULL;
  }
  return 0;
}

drax_env* new_env() {
  drax_env* blenv = (drax_env*) malloc(sizeof(drax_env));
  init_environment(blenv);

  /* Only main environment */
  blenv->native = (drax_env*) malloc(sizeof(drax_env));
  init_environment(blenv->native);
  blenv->global = NULL;

  return blenv;
}

void del_benv(drax_env* e) {
  if ((NULL != e) && (NULL != e->global)) {
    for (size_t i = 0; i < e->bval->cap; i++) {
      if (e->bval->vars[i]) {

        for (size_t j = 0; j < e->bval->vars[i]->length; j++) {
          if (e->bval->vars[i]->val[j]) {
            free(e->bval->vars[i]->val[j]);
          }
        }

        free(e->bval->vars[i]);
      }
    }
    
    for (size_t i = 0; i < e->bfuncs->cap; i++) {
      if (e->bfuncs->funs[i]) {
        for (size_t j = 0; j < e->bfuncs->funs[i]->length; j++) {
          if (e->bfuncs->funs[i]->val[j]) {
            free(e->bfuncs->funs[i]->val[j]);
          }
        }

        free(e->bfuncs->funs[i]);
      }
    }
    free(e);
  }
}

void bput_env(drax_env* e, drax_state* key, drax_state* value) {
  size_t idx = gen_hash_idx(e->bval->cap, (char*) key->val);

  if (e->bval->vars[idx]) {
    for (size_t i = 0; i < e->bval->vars[idx]->length; i++) {
      if (strcmp(e->bval->vars[idx]->key[i], (char*) key->val) == 0) {
        del_bstate(e->bval->vars[idx]->val[i]);
        e->bval->vars[idx]->val[i] = bcopy_state(value);
      }
    }
  }

  if (NULL == e->bval->vars[idx]) {
    e->bval->vars[idx] = (bvars_pair*) malloc(sizeof(bvars_pair));
    e->bval->vars[idx]->length = 0;
  }

  e->bval->vars[idx]->length++;
  if (e->bval->vars[idx]->length <= 1) {
    e->bval->vars[idx]->key = (char**) malloc(sizeof(char*));
    e->bval->vars[idx]->val = (drax_state**) malloc(sizeof(drax_state*));
  } else {
    e->bval->vars[idx]->key = (char**) realloc(e->bval->vars[idx]->key, sizeof(char*) * e->bval->vars[idx]->length);
    e->bval->vars[idx]->val = (drax_state**) realloc(e->bval->vars[idx]->val, sizeof(drax_state*) * e->bval->vars[idx]->length);
  }

  e->bval->vars[idx]->val[e->bval->vars[idx]->length - 1] = bcopy_state(value);
  e->bval->vars[idx]->key[e->bval->vars[idx]->length - 1] = (char*) malloc((strlen((char*) key->val) + 1) * sizeof(char));
  strcpy(e->bval->vars[idx]->key[e->bval->vars[idx]->length - 1], (char*) key->val);
  free((char*) key->val);
}

static void bput_env_function(drax_env* e, drax_state* value) {
  char* fname = (char*) value->child[0]->val;
  int arity = value->child[1]->length;
  size_t idx = gen_hash_idx(e->bval->cap, fname);

  if (e->bfuncs->funs[idx]) {
    for (size_t i = 0; i < e->bfuncs->funs[idx]->length; i++) {
      if (
        (strcmp(e->bfuncs->funs[idx]->fname[i], fname) == 0) && 
        (e->bfuncs->funs[idx]->arity[i] == arity)
      ) {
        del_bstate(e->bfuncs->funs[idx]->val[i]);
        e->bfuncs->funs[idx]->val[i] = bcopy_state(value);
      }
    }
  }

  if (NULL == e->bfuncs->funs[idx]) {
    e->bfuncs->funs[idx] = (bfunc_pair*) malloc(sizeof(bfunc_pair));
    e->bfuncs->funs[idx]->length = 0;
  }

  e->bfuncs->funs[idx]->length++;
  if (e->bfuncs->funs[idx]->length <= 1) {
    e->bfuncs->funs[idx]->arity = (int*) malloc(sizeof(int));
    e->bfuncs->funs[idx]->fname = (char**) malloc(sizeof(char*));
    e->bfuncs->funs[idx]->val = (drax_state**) malloc(sizeof(drax_state*));
  } else {
    e->bfuncs->funs[idx]->arity = (int*) realloc(e->bfuncs->funs[idx]->arity, sizeof(int) * e->bfuncs->funs[idx]->length);
    e->bfuncs->funs[idx]->fname = (char**) realloc(e->bfuncs->funs[idx]->fname, sizeof(char*) * e->bfuncs->funs[idx]->length);
    e->bfuncs->funs[idx]->val = (drax_state**) realloc(e->bfuncs->funs[idx]->val, sizeof(drax_state*) * e->bfuncs->funs[idx]->length);
  }

  e->bfuncs->funs[idx]->arity[e->bfuncs->funs[idx]->length - 1] = arity;
  e->bfuncs->funs[idx]->val[e->bfuncs->funs[idx]->length - 1] = bcopy_state(value);
  e->bfuncs->funs[idx]->fname[e->bfuncs->funs[idx]->length - 1] = (char*) malloc((strlen(fname) + 1) * sizeof(char));
  strcpy(e->bfuncs->funs[idx]->fname[e->bfuncs->funs[idx]->length - 1], fname);
  free(fname);
}

void bregister_env_function(drax_env* e, drax_state* bfun) {
  bput_env_function(e, bfun);
}

void bset_env(drax_env* e, drax_state* key, drax_state* value) {
  bput_env(e, key, value);
}

void blet_env(drax_env* e, drax_state* key, drax_state* value) {
  bput_env(e, key, value);
}

drax_state* bget_env_value(drax_env* e, drax_state* key) {
  size_t idx = gen_hash_idx(e->bval->cap, (char*) key->val);
  
  if (NULL == e->bval->vars[idx]) {
    return NULL;
  }

  for (size_t i = 0; i < e->bval->vars[idx]->length; i++) {
    if (strcmp(e->bval->vars[idx]->key[i], (char*) key->val) == 0) {
      return bcopy_state(e->bval->vars[idx]->val[i]);
    }
  }

  return NULL;
}

drax_state* bget_env_function(drax_env* e, drax_state* exp) {
  char* fname = (char*) exp->child[0]->val;

  size_t idx = gen_hash_idx(e->bfuncs->cap, fname);
  
  if (NULL == e->bfuncs->funs[idx]) {
    return NULL;
  }

  for (size_t i = 0; i < e->bfuncs->funs[idx]->length; i++) {
    if ((strcmp(e->bfuncs->funs[idx]->fname[i], fname) == 0) &&
        (e->bfuncs->funs[idx]->arity[i] == exp->length -1)
    ) {
      return bcopy_state(e->bfuncs->funs[idx]->val[i]);
    }
  }

  return NULL;
}

double draxvalue_to_num(drax_value value) {
  double num;
  memcpy(&num, &value, sizeof(drax_value));
  return num;
}

drax_value num_to_draxvalue(double num) {
  drax_value value;
  memcpy(&value, &num, sizeof(double));
  return value;
}

