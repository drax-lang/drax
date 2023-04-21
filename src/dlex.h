/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __DLEX
#define __DLEX


typedef enum dlex_types {
  DTK_PAR_OPEN, DTK_PAR_CLOSE,
  DTK_BKT_OPEN, DTK_BKT_CLOSE,

  /* str. op. */
  DTK_CONCAT,

  /* arith op */
  DTK_SUB, DTK_ADD, DTK_DIV, DTK_MUL,

  /* bool op */
  DTK_LS, DTK_BG, DTK_LE, DTK_BE,
  DTK_EQ, DTK_DEQ, DTK_BNG, DTK_BNG_EQ,
  
  /* Keywords */
  DTK_AND, DTK_DO, DTK_ELSE, DTK_END, 
  DTK_FALSE, DTK_FUN, DTK_IF, DTK_NIL, 
  DTK_OR, DTK_PRINT, DTK_TRUE, DTK_LET, 

  DTK_ID, DTK_STRING, DTK_NUMBER, DTK_COMMA, DTK_DOT, 

  DTK_ERROR,
  
  DTK_BREAK_LINE,

  /* last element */
  DTK_EOF
} dlex_types;

typedef enum dfstap_errors_type {
  DCE_NONE,
  DCE_LEX,
  DCE_PARSE,
  DCE_RUNTIME,
} dfstap_errors_type;


typedef struct drax_tokens {
  const char* str;
  int dtk; 
} drax_tokens;

typedef struct d_token {
  dlex_types type;
  dfstap_errors_type error_type;
  const char* first;
  int length;
  int line;
} d_token;

typedef struct curr_lex_state {
  const char* first;
  const char* current;
  int line;
} curr_lex_state;

void init_lexan(const char* source);

d_token next_token();

#endif