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

typedef enum berrors_type {
  BSYNTAX_ERROR,
  BTYPE_ERROR,
  BREFERENCE_ERROR,
  BUNKNOWN_TYPE_ERROR,
  BZERO_DIVISION_ERROR
} berrors_type;

typedef struct beorn_state {
  types type;
  berrors_type et;
  int length;
  int closed;
  char* cval;
  long ival;
  long double fval;
  struct beorn_state** child;
} beorn_state;


beorn_state* new_error(berrors_type t, char* s);

beorn_state* new_integer(long iv);

beorn_state* new_float(long double fv);

beorn_state* new_string(char* s);

beorn_state* new_symbol(char* s);

beorn_state* new_pack_freeze(char* s);

beorn_state* new_expression(char* s);

void del_bstate(beorn_state* curr);

char* berrors_to_str(berrors_type t);

#endif
