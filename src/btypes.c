#include <string.h>
#include <stdlib.h>
#include "btypes.h"
#include "stdio.h"
#include <stdarg.h>

beorn_state* new_error(berrors_type t, char* s, ...) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->type = BT_ERROR;
  v->cval = malloc(strlen(s) + 1);
  v->et = t;
  v->length = 0;
  v->child = NULL;

  va_list va;
  va_start(va, s);  

  vsnprintf(v->cval, 511, s, va);  
  va_end(va);
  return v;
}

beorn_state* new_integer(long iv) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->type = BT_INTEGER;
  v->ival = iv;
  v->child = NULL;
  return v;
}

beorn_state* new_float(long double fv) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->type = BT_FLOAT;
  v->fval = fv;
  v->child = NULL;
  return v;
}

beorn_state* new_string(char* s) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->type = BT_STRING;
  v->cval = malloc(strlen(s) + 1);
  v->child = NULL;
  strcpy(v->cval, s);
  return v;
}

beorn_state* new_symbol(char* s) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->type = BT_SYMBOL;
  v->cval = malloc(strlen(s) + 1);
  v->child = NULL;
  strcpy(v->cval, s);
  return v;
}

beorn_state* new_pack(char* s) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->type = BT_PACK;
  v->cval = malloc(strlen(s) + 1);
  v->child = NULL;
  v->length = 0;
  v->closed = 0;
  strcpy(v->cval, s);
  return v;
}

beorn_state* new_expression(char* s) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->blenv = new_env();
  v->type = BT_EXPRESSION;
  v->cval = malloc(strlen(s) + 1);
  v->child = NULL;
  v->length = 0;
  v->closed = 0;
  strcpy(v->cval, s);
  return v;
}

beorn_state* new_list(char* s) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->blenv = NULL;
  v->type = BT_LIST;
  v->cval = malloc(strlen(s) + 1);
  v->child = NULL;
  v->length = 0;
  v->closed = 0;
  strcpy(v->cval, s);
  return v;
}

beorn_state* new_function(beorn_func fn) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->type = BT_FUNCTION;
  v->bfunc = fn;
  v->child = NULL;
  return v;
};

beorn_state* new_lambda(beorn_env* global) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->type = BT_LAMBDA;
  v->bfunc = NULL;
  v->blenv = new_env();
  v->blenv->global = global;
  v->child = malloc(sizeof(beorn_state*) * 2);
  return v;
};

beorn_env* new_env() {
  beorn_env* blenv = malloc(sizeof(beorn_env));
  blenv->length = 0;
  blenv->bval =  malloc(sizeof(beorn_state *));
  blenv->symbol = malloc(sizeof(char*));

  blenv->native = malloc(sizeof(beorn_env));
  blenv->native->length = 0;
  blenv->native->bval =  malloc(sizeof(beorn_state *));
  blenv->native->symbol = malloc(sizeof(char*));
  blenv->global = NULL;

  return blenv;
}

void del_bstate(beorn_state* curr) {

  switch (curr->type) {
    case BT_PACK:
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

char* berrors_to_str(berrors_type t) {
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

char* btype_to_str(types t) {
  switch (t) {
    case BT_INTEGER:      return "Integer";
    case BT_FLOAT:        return "Float";
    case BT_STRING:       return "String";
    case BT_PACK:         return "Pack";
    case BT_SYMBOL:       return "Symbol";
    case BT_EXPRESSION:   return "Expression";
    case BT_LIST:         return "List";
    default:              return "Unknown";
  }
}

beorn_state* bcopy_state(beorn_state* v) {
  beorn_state* x = malloc(sizeof(beorn_state));
  x->type = v->type;
  switch (v->type) {
    break;
    case BT_INTEGER: x->ival = v->ival; break;
    case BT_FLOAT: x->fval = v->fval; break;
    case BT_ERROR:
      x->et = v->et;
      x->cval = malloc(strlen(v->cval) + 1);
      strcpy(x->cval, v->cval);
    break;
    case BT_STRING:
    case BT_SYMBOL:
      x->cval = malloc(strlen(v->cval) + 1);
      strcpy(x->cval, v->cval);
    break;
    case BT_FUNCTION: x->bfunc = v->bfunc;
    case BT_PACK:
    case BT_LIST:
    case BT_LAMBDA:
    case BT_EXPRESSION:
      x->length = v->length;
      x->blenv = v->blenv;
      x->child = malloc(sizeof(beorn_state*) * x->length);
      for (int i = 0; i < x->length; i++) {
        x->child[i] = bcopy_state(v->child[i]);
      }
    break;
    default: ;
  }
  return x;
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
  e->bval = realloc(e->bval, sizeof(beorn_state*) * e->length);
  e->symbol = realloc(e->symbol, sizeof(char*) * e->length);  

  e->bval[e->length-1] = bcopy_state(value);
  e->symbol[e->length-1] = malloc(strlen(key->cval)+1);
  strcpy(e->symbol[e->length -1], key->cval);
}

void bset_env(beorn_env* e, beorn_state* key, beorn_state* value) {
  bput_env(e, key, value);
}

void blet_env(beorn_env* e, beorn_state* key, beorn_state* value) {
  bput_env(e, key, value);
}
