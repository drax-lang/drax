/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */
 
#ifndef __BTYPES
#define __BTYPES

typedef enum types {
  BT_UNKNOWN,
  BT_ERROR,
  BT_PROGRAM,
  BT_INTEGER,
  BT_FLOAT,
  BT_STRING,
  BT_SYMBOL,
  BT_EXPRESSION,
  BT_PACK_FREEZE
} types;

typedef struct bexprt {
  long double val;
  struct expression* lf;
  struct expression* rg;
} bexprt;

typedef struct beorn_state {
  types type;
  int length;
  int closed;
  char* cval;
  long ival;
  long double fval;
  bexprt expr;
  struct beorn_state** child;
} beorn_state;


beorn_state* new_error(char* s);

beorn_state* new_integer(long iv);

beorn_state* new_float(long double fv);

beorn_state* new_string(char* s);

beorn_state* new_symbol(char* s);

beorn_state* new_pack_freeze(char* s);

beorn_state* new_expression(char* s);

void del_bstate(beorn_state* curr);

#endif
