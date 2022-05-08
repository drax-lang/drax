/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */
 
#ifndef __BTYPES
#define __BTYPES

#define BASSERT(cond, t, msg, ...) if (cond) { return new_error(t, msg, ##__VA_ARGS__); }

typedef enum types {
  BT_UNKNOWN,
  BT_ERROR,
  BT_PROGRAM,
  BT_INTEGER,
  BT_FLOAT,
  BT_STRING,
  BT_SYMBOL,
  BT_EXPRESSION,
  BT_PACK,
  BT_LIST,
  BT_FUNCTION
} types;

typedef enum berrors_type {
  BSYNTAX_ERROR,
  BTYPE_ERROR,
  BREFERENCE_ERROR,
  BUNKNOWN_TYPE_ERROR,
  BZERO_DIVISION_ERROR,
  BUNSPECTED_TYPE,
  BPARSER_ERROR
} berrors_type;

typedef struct beorn_env beorn_env;
typedef struct beorn_state beorn_state;

typedef beorn_state*(*beorn_func)(beorn_env* e, struct beorn_state* s);
typedef struct beorn_state {
  types type;
  berrors_type et;
  int length;
  int closed;
  char* cval;
  long ival;
  long double fval;
  beorn_func bfunc;
  beorn_env* blenv;
  beorn_state** child;
} beorn_state;

typedef struct beorn_env {
  int length;
  char** symbol;
  beorn_state** bval;
} beorn_env;

beorn_state* new_error(berrors_type t, char* s, ...);
beorn_state* new_integer(long iv);
beorn_state* new_float(long double fv);
beorn_state* new_string(char* s);
beorn_state* new_symbol(char* s);
beorn_state* new_pack(char* s);
beorn_state* new_expression(char* s);
beorn_state* new_list(char* s);
beorn_state* new_function(beorn_func fn);

beorn_env* new_env();

void del_bstate(beorn_state* curr);

char* berrors_to_str(berrors_type t);

char* btype_to_str(types t);

beorn_state* bcopy_state(beorn_state* v);

void bput_env(beorn_env* e, beorn_state* key, beorn_state* value);
void bset_env(beorn_env* e, beorn_state* key, beorn_state* value);
void blet_env(beorn_env* e, beorn_state* key, beorn_state* value);

#endif
