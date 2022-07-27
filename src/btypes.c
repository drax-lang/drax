#include <string.h>
#include <stdlib.h>
#include "btypes.h"
#include "stdio.h"
#include <stdarg.h>

/* Types implementations */

static beorn_state* new_beorn_state() {
  beorn_state* v = (beorn_state*) malloc(sizeof(beorn_state));
  v->length = 0;
  v->child = NULL;
  v->trace = NULL;
  v->call_definition = 0;

  return v;
}

beorn_state* new_error(berrors_type t, const char* s, ...) {
  beorn_state* v = new_beorn_state();
  v->type = BT_ERROR;
  v->cval = (char *) calloc(strlen(s) + 1, sizeof(char));
  v->et = t;
  va_list va;
  va_start(va, s);

  vsnprintf(v->cval, 511, s, va);
  va_end(va);
  return v;
}

beorn_state* new_integer(long iv) {
  beorn_state* v = new_beorn_state();
  v->type = BT_INTEGER;
  v->ival = iv;
  v->closed = 1;
  return v;
}

beorn_state* new_float(long double fv) {
  beorn_state* v = new_beorn_state();
  v->type = BT_FLOAT;
  v->fval = fv;
  v->closed = 1;
  return v;
}

beorn_state* new_string(const char* s) {
  int strsize = 0;
  
  if (NULL != s)
    strsize = strlen(s);

  beorn_state* v = new_beorn_state();
  v->type = BT_STRING;
  v->cval = (char *) calloc(sizeof(char), strsize + 1);
  v->closed = 1;

  if (NULL != s)
    strcpy(v->cval, s);
    
  return v;
}

beorn_state* new_symbol(const char* s) {
  beorn_state* v = new_beorn_state();
  v->type = BT_SYMBOL;

  if (NULL != s) {
    v->cval = (char *) calloc(sizeof(char), strlen(s) + 1);
    strcpy(v->cval, s);
  }

  v->closed = 1;
  return v;
}

beorn_state* new_pack() {
  beorn_state* v = new_beorn_state();
  v->type = BT_PACK;
  v->cval = NULL;
  v->closed = 0;
  return v;
}

beorn_state* new_expression() {
  beorn_state* v = new_beorn_state();
  v->blenv = new_env();
  v->type = BT_EXPRESSION;
  v->cval = NULL;
  v->closed = 0;
  return v;
}

beorn_state* new_list() {
  beorn_state* v = new_beorn_state();
  v->blenv = NULL;
  v->type = BT_LIST;
  v->cval = NULL;
  v->closed = 0;
  return v;
}

beorn_state* new_function(beorn_func fn) {
  beorn_state* v = new_beorn_state();
  v->type = BT_FUNCTION;
  v->bfunc = fn;
  v->child = NULL;
  v->call_definition = 0;
  return v;
};

beorn_state* new_lambda(beorn_env* global) {
  beorn_state* v = new_beorn_state();
  v->type = BT_LAMBDA;
  v->bfunc = NULL;
  v->blenv = new_env();
  v->blenv->global = global;
  v->child = (beorn_state**) malloc(sizeof(beorn_state*) * 2);
  return v;
};

beorn_state* new_nil() {
  beorn_state* v = new_beorn_state();
  v->type = BT_NIL;
  v->blenv = NULL;
  v->cval = NULL;
  v->closed = 0;
  return v;
}

void del_bstate(beorn_state* curr) {

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

beorn_state* bcopy_state(beorn_state* v) {
  beorn_state* x = (beorn_state*) malloc(sizeof(beorn_state));
  x->type = v->type;
  switch (v->type) {
    break;
    case BT_INTEGER: x->ival = v->ival; break;
    case BT_FLOAT: x->fval = v->fval; break;
    case BT_ERROR:
      x->et = v->et;
      x->cval = (char *) malloc((strlen(v->cval) + 1) * sizeof(char));
      strcpy(x->cval, v->cval);
    break;
    case BT_STRING:
    case BT_SYMBOL:
      x->cval = (char *) malloc((strlen(v->cval) + 1) * sizeof(char));
      strcpy(x->cval, v->cval);
    break;

    case BT_FUNCTION:
      x->bfunc = v->bfunc;
      break;

    case BT_PACK:
    case BT_LIST:
    case BT_LAMBDA:
    case BT_EXPRESSION:
      x->length = v->length;
      x->blenv = v->blenv;
      x->child = (beorn_state**) malloc(sizeof(beorn_state*) * x->length);
      for (int i = 0; i < x->length; i++) {
        x->child[i] = bcopy_state(v->child[i]);
      }
    break;
    default: ;
  }
  return x;
}

beorn_state* bpop(beorn_state* curr, int i){
  beorn_state* ele = curr->child[i];

  memmove(&curr->child[i], &curr->child[i+1],
    sizeof(beorn_state*) * (curr->length -i -1));

  curr->length--;
  curr->child = (beorn_state**) realloc(curr->child, sizeof(beorn_state*) * curr->length);
  return ele;
}

/* Environments configs */

static size_t gen_hash_idx(bvar_hashs* hsh, char* key) {
    size_t idx;
    for (idx = 0; *key != '\0'; key++) {
        idx = *key + 31 * idx;
    }
    return idx % (hsh->cap);
}

static int init_environment(beorn_env* e) {
  e->bval = (bvar_hashs*) malloc(sizeof(bvar_hashs));
  e->bval->cap = BENV_HASH_SIZE;
  e->bval->vars = (bvars_pair**) malloc(sizeof(bvars_pair *) * BENV_HASH_SIZE);
  return 0;
}

beorn_env* new_env() {
  beorn_env* blenv = (beorn_env*) malloc(sizeof(beorn_env));
  init_environment(blenv);

  /* Only main environment */
  blenv->native = (beorn_env*) malloc(sizeof(beorn_env));
  init_environment(blenv->native);
  blenv->global = NULL;

  return blenv;
}

void bput_env(beorn_env* e, beorn_state* key, beorn_state* value) {
  size_t idx = gen_hash_idx(e->bval, key->cval);

  if (e->bval->vars[idx]) {
    for (size_t i = 0; i < e->bval->vars[idx]->length; i++) {
      if (strcmp(e->bval->vars[idx]->key[i], key->cval) == 0) {
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
    e->bval->vars[idx]->val = (beorn_state**) malloc(sizeof(beorn_state*));
  } else {
    e->bval->vars[idx]->key = (char**) realloc(e->bval->vars[idx]->key, sizeof(char*) * e->bval->vars[idx]->length);
    e->bval->vars[idx]->val = (beorn_state**) realloc(e->bval->vars[idx]->val, sizeof(beorn_state*) * e->bval->vars[idx]->length);
  }

  e->bval->vars[idx]->val[e->bval->vars[idx]->length - 1] = bcopy_state(value);
  e->bval->vars[idx]->key[e->bval->vars[idx]->length - 1] = (char*) malloc((strlen(key->cval) + 1) * sizeof(char));
  strcpy(e->bval->vars[idx]->key[e->bval->vars[idx]->length - 1], key->cval);
  free(key->cval);
}

void bset_env(beorn_env* e, beorn_state* key, beorn_state* value) {
  bput_env(e, key, value);
}

void blet_env(beorn_env* e, beorn_state* key, beorn_state* value) {
  bput_env(e, key, value);
}

beorn_state* bget_env_value(beorn_env* e, beorn_state* key) {
  size_t idx = gen_hash_idx(e->bval, key->cval);
  
  if (NULL == e->bval->vars[idx]) {
    return NULL;
  }

  for (size_t i = 0; i < e->bval->vars[idx]->length; i++) {
    if (strcmp(e->bval->vars[idx]->key[i], key->cval) == 0) {
      return bcopy_state(e->bval->vars[idx]->val[i]);
    }
  }

  return NULL;
}
