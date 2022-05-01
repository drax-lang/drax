#include <string.h>
#include <stdlib.h>
#include "btypes.h"

beorn_state* new_error(char* s) {
  beorn_state* v = malloc(sizeof(beorn_state));
  v->type = BT_ERROR;
  v->cval = malloc(strlen(s) + 1);
  strcpy(v->cval, s);
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
  v->type = BT_PACK_FREEZE;
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
    case BT_PACK_FREEZE:
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
