/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __DLEX
#define __DLEX

typedef enum dlex_types {
  TK_BRACE_CLOSE = 0, TK_BRACE_OPEN, TK_BRACKET_CLOSE, 
  TK_BRACKET_OPEN, TK_COMMA, TK_DO, TK_END, TK_FLOAT,
  TK_FUN, TK_IF, TK_ELSE, TK_IMPORT, TK_INTEGER, TK_LAMBDA,
  TK_PAR_CLOSE, TK_PAR_OPEN, TK_STRING, TK_SYMBOL, 
  
  /* logic */
  TK_AND, TK_OR,

  /* cmp */ 
  TK_NOT_EQ, TK_NOT_DEQ, TK_NOT_TEQ,
  TK_EQ, TK_DEQ, TK_TEQ,

  /* arith op */
  TK_ADD, TK_DIV, TK_MUL, TK_SUB,

  /* bool op */
  TK_LS, TK_BG, TK_LE, TK_BE,

  TK_NIL,
  
  TK_BREAK_LINE,

  /* last element */
  TK_EOF,

} dlex_types;

typedef struct d_token {
  dlex_types type;
  long long ival;
  long double fval;
  char* cval;
} d_token;

typedef struct blex_tokens {
  d_token** child;
  int line;
  int pos;
  int length;
} blex_tokens;

/* global */

char* append_char(const char *str, const char c);

int is_number(const char c);

int is_symbol(const char c);

d_token* bmake_string(char* val);

d_token* bmake_int(dlex_types type, long long val);

d_token* bmake_float(dlex_types type, long double val);

d_token* bmake_symbol(dlex_types type);

d_token* bmake_return(char* keyword);

int init_lexan(char* b);

d_token* b_check_next(int* nump);

d_token* b_check_prev();

d_token* lexan();

#endif
