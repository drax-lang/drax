#include <string.h>
#include <stdlib.h>
#include "btypes.h"
#include "stdio.h"
#include <stdarg.h>

beorn_state* new_error(berrors_type t, const char* s, ...) {
  beorn_state* v = (beorn_state*) malloc(sizeof(beorn_state));
  v->type = BT_ERROR;
  v->cval = (char *) calloc(strlen(s) + 1, sizeof(char));
  v->et = t;
  v->length = 0;
  v->child = NULL;
  v->call_definition = 0;

  va_list va;
  va_start(va, s);

  vsnprintf(v->cval, 511, s, va);
  va_end(va);
  return v;
}

beorn_state* new_integer(long iv) {
  beorn_state* v = (beorn_state*) malloc(sizeof(beorn_state));
  v->type = BT_INTEGER;
  v->ival = iv;
  v->child = NULL;
  v->length = 0;
  v->closed = 1;
  v->call_definition = 0;
  return v;
}

beorn_state* new_float(long double fv) {
  beorn_state* v = (beorn_state*) malloc(sizeof(beorn_state));
  v->type = BT_FLOAT;
  v->fval = fv;
  v->child = NULL;
  v->length = 0;
  v->closed = 1;
  v->call_definition = 0;
  return v;
}

beorn_state* new_string(const char* s) {
  int strsize = 0;
  
  if (NULL != s)
    strsize = strlen(s);

  beorn_state* v = (beorn_state*) malloc(sizeof(beorn_state));
  v->type = BT_STRING;
  v->cval = (char *) calloc(sizeof(char), strsize + 1);
  v->child = NULL;
  v->length = 0;
  v->closed = 1;
  v->call_definition = 0;

  if (NULL != s)
    strcpy(v->cval, s);
    
  return v;
}

beorn_state* new_symbol(const char* s) {
  beorn_state* v = (beorn_state*) malloc(sizeof(beorn_state));
  v->type = BT_SYMBOL;

  if (NULL != s) {
    v->cval = (char *) calloc(sizeof(char), strlen(s) + 1);
    strcpy(v->cval, s);
  }

  v->child = NULL;
  v->length = 0;
  v->closed = 1;
  v->call_definition = 0;
  return v;
}

beorn_state* new_pack() {
  beorn_state* v = (beorn_state*) malloc(sizeof(beorn_state));
  v->type = BT_PACK;
  v->cval = NULL;
  v->child = NULL;
  v->length = 0;
  v->closed = 0;
  v->call_definition = 0;
  return v;
}

beorn_state* new_expression() {
  beorn_state* v = (beorn_state*) malloc(sizeof(beorn_state));
  v->blenv = new_env();
  v->type = BT_EXPRESSION;
  v->cval = NULL;
  v->child = NULL;
  v->length = 0;
  v->closed = 0;
  v->call_definition = 0;
  return v;
}

beorn_state* new_list() {
  beorn_state* v = (beorn_state*) malloc(sizeof(beorn_state));
  v->blenv = NULL;
  v->type = BT_LIST;
  v->cval = NULL;
  v->child = NULL;
  v->length = 0;
  v->closed = 0;
  v->call_definition = 0;
  return v;
}

beorn_state* new_function(beorn_func fn) {
  beorn_state* v = (beorn_state*) malloc(sizeof(beorn_state));
  v->type = BT_FUNCTION;
  v->bfunc = fn;
  v->child = NULL;
  v->call_definition = 0;
  return v;
};

beorn_state* new_lambda(beorn_env* global) {
  beorn_state* v = (beorn_state *) malloc(sizeof(beorn_state));
  v->type = BT_LAMBDA;
  v->bfunc = NULL;
  v->blenv = new_env();
  v->blenv->global = global;
  v->child = (beorn_state**) malloc(sizeof(beorn_state*) * 2);
  v->length = 0;
  v->call_definition = 0;
  return v;
};

beorn_env* new_env() {
  beorn_env* blenv = (beorn_env*) malloc(sizeof(beorn_env));
  blenv->length = 0;
  blenv->bval = (beorn_state**) malloc(sizeof(beorn_state *));
  blenv->symbol = (char**) malloc(sizeof(char*));

  blenv->native = (beorn_env*) malloc(sizeof(beorn_env));
  blenv->native->length = 0;
  blenv->native->bval = (beorn_state**) malloc(sizeof(beorn_state *));
  blenv->native->symbol = (char**) malloc(sizeof(char*));
  blenv->global = NULL;

  return blenv;
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

void bput_env(beorn_env* e, beorn_state* key, beorn_state* value) {
  
  for (int i = 0; i < e->length; i++) {
    if (strcmp(e->symbol[i], key->cval) == 0) {
      del_bstate(e->bval[i]);
      e->bval[i] = bcopy_state(value);
      return;
    }
  }

  e->length++;
  e->bval = (beorn_state**) realloc(e->bval, sizeof(beorn_state*) * e->length);
  e->symbol = (char**) realloc(e->symbol, sizeof(char*) * e->length);

  e->bval[e->length -1] = bcopy_state(value);
  e->symbol[e->length -1] = (char*) malloc((strlen(key->cval) + 1) * sizeof(char));
  strcpy(e->symbol[e->length -1], key->cval);
  free(key->cval);
}

void bset_env(beorn_env* e, beorn_state* key, beorn_state* value) {
  bput_env(e, key, value);
}

void blet_env(beorn_env* e, beorn_state* key, beorn_state* value) {
  bput_env(e, key, value);
}
