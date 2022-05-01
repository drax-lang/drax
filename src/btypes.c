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
  strcpy(v->cval, s);
  return v;
}

beorn_state* new_symbol(char* s) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->type = BT_SYMBOL;
  v->cval = malloc(strlen(s) + 1);
  strcpy(v->cval, s);
  return v;
}

beorn_state* new_pack_freeze(char* s) {
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
  v->type = BT_EXPRESSION;
  v->cval = malloc(strlen(s) + 1);
  v->child = NULL;
  v->length = 0;
  v->closed = 0;
  strcpy(v->cval, s);
  return v;
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
    default:              return "Unknown";
  }
}
