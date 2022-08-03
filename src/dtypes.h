/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */
 
#ifndef __DTYPES
#define __DTYPES

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

typedef enum bact_type{
  BACT_NONE,
  BACT_CALL_OP,
  BACT_CORE_OP,
} bact_type;

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

typedef struct drax_env drax_env;

typedef struct drax_state drax_state;

typedef drax_state*(*drax_func)(drax_env* e, struct drax_state* s);

/* Vars allocation */
typedef struct bvars_pair {
    size_t length;
    char** key;
    drax_state** val;
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
    drax_state** val;
} bfunc_pair;

typedef struct bfunc_hashs {
    size_t cap;
    bfunc_pair** funs;
} bfunc_hashs;

typedef struct drax_state {
  types type;
  bact_type act;
  berrors_type et;
  bstack_trace* trace;
  int length;
  int closed;
  char* cval;
  long long ival;
  long double fval;
  drax_func bfunc;
  drax_env* blenv;
  drax_state** child;
} drax_state;

typedef struct drax_env {
  bvar_hashs* bval;
  bfunc_hashs* bfuncs;
  drax_env* native;
  drax_env* global;
} drax_env;

drax_state* new_error(berrors_type t, const char* s, ...);

drax_state* new_integer(long iv);

drax_state* new_float(long double fv);

drax_state* new_string(const char* s);

drax_state* new_symbol(const char* s);

drax_state* new_pack();

drax_state* new_expression();

drax_state* new_list();

drax_state* new_function(drax_func fn);

drax_state* new_lambda(drax_env* global);

drax_state* new_nil();

drax_env* new_env();

void del_benv(drax_env* e);

void del_bstate(drax_state* curr);

const char* berrors_to_str(berrors_type t);

const char* btype_to_str(types t);

drax_state* bcopy_state(drax_state* v);

drax_state* bpop(drax_state* v, int idx);

void bput_env(drax_env* e, drax_state* key, drax_state* value);

void bregister_env_function(drax_env* e, drax_state* bfun);

void bset_env(drax_env* e, drax_state* key, drax_state* value);

void blet_env(drax_env* e, drax_state* key, drax_state* value);

drax_state* bget_env_value(drax_env* e, drax_state* key);

drax_state* bget_env_function(drax_env* e, drax_state* key);

#endif
