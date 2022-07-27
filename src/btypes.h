/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */
 
#ifndef __BTYPES
#define __BTYPES

#define BASSERT(cond, t, msg, ...) if (cond) { return new_error(t, msg, ##__VA_ARGS__); }

#define UNUSED(x) (void)(x)

#define BENV_HASH_SIZE 8

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
  BT_FUNCTION,
  BT_LAMBDA,
  BT_NIL
} types;

typedef enum bimode {
  BI_NONE,
  BI_PROCESS_DEFAULT,
  BI_INTERACTIVE_DEFAULT
} bimode;

typedef enum berrors_type {
  BSYNTAX_ERROR,
  BRUNTIME_ERROR,
  BTYPE_ERROR,
  BREFERENCE_ERROR,
  BUNKNOWN_TYPE_ERROR,
  BZERO_DIVISION_ERROR,
  BUNSPECTED_TYPE,
  BPARSER_ERROR,
  BFILE_NOT_FOUND
} berrors_type;

typedef struct bstack_trace {
  size_t line;
  char* file;
} bstack_trace;

typedef struct beorn_env beorn_env;

typedef struct beorn_state beorn_state;

typedef beorn_state*(*beorn_func)(beorn_env* e, struct beorn_state* s);

/* Vars allocation */
typedef struct bvars_pair {
    size_t length;
    char** key;
    beorn_state** val;
} bvars_pair;

typedef struct bvar_hashs {
    size_t cap;
    bvars_pair** vars;
} bvar_hashs;

/* Functions allocation */
typedef struct bfunc_pair {
    size_t length;
    char** fname;
    int* arity;
    beorn_state** val;
} bfunc_pair;

typedef struct bfunc_hashs {
    size_t cap;
    bfunc_pair** funs;
} bfunc_hashs;

typedef struct beorn_state {
  types type;
  berrors_type et;
  bstack_trace* trace;
  int length;
  int closed;
  int call_definition;
  char* cval;
  long int ival;
  long double fval;
  beorn_func bfunc;
  beorn_env* blenv;
  beorn_state** child;
} beorn_state;

typedef struct beorn_env {
  bvar_hashs* bval;
  bfunc_hashs* bfuncs;
  beorn_env* native;
  beorn_env* global;
} beorn_env;

beorn_state* new_error(berrors_type t, const char* s, ...);

beorn_state* new_integer(long iv);

beorn_state* new_float(long double fv);

beorn_state* new_string(const char* s);

beorn_state* new_symbol(const char* s);

beorn_state* new_pack();

beorn_state* new_expression();

beorn_state* new_list();

beorn_state* new_function(beorn_func fn);

beorn_state* new_lambda(beorn_env* global);

beorn_state* new_nil();

beorn_env* new_env();

void del_benv(beorn_env* e);

void del_bstate(beorn_state* curr);

const char* berrors_to_str(berrors_type t);

const char* btype_to_str(types t);

beorn_state* bcopy_state(beorn_state* v);

beorn_state* bpop(beorn_state* v, int idx);

void bput_env(beorn_env* e, beorn_state* key, beorn_state* value);

void bregister_env_function(beorn_env* e, beorn_state* bfun);

void bset_env(beorn_env* e, beorn_state* key, beorn_state* value);

void blet_env(beorn_env* e, beorn_state* key, beorn_state* value);

beorn_state* bget_env_value(beorn_env* e, beorn_state* key);

beorn_state* bget_env_function(beorn_env* e, beorn_state* key);

#endif
