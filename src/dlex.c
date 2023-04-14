#include "dlex.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

curr_lex_state clexs;

#define CURR_TOKEN()  (*clexs.current)

#define IS_EOF()      (*clexs.current == '\0')

#define NUM_ELEMS(x)  (sizeof(x) / sizeof((x)[0]))

static const drax_tokens drax_t[] = {
  {"and",    DTK_AND},
  {"do",     DTK_DO},
  {"else",   DTK_ELSE},
  {"end",    DTK_END},
  {"false",  DTK_FALSE},
  {"fun",    DTK_FUN},
  {"if",     DTK_IF},
  {"nil",    DTK_NIL},
  {"or",     DTK_OR},
  {"print",  DTK_PRINT},
  {"true",   DTK_TRUE},
 };

/* Helpers */
static bool is_alpha(const char c) {
  if (c == 0) return false;

  char accepted_chars[] = "abcdefghijklmnopqrstuvxwyzABCDEFGHIJKLMNOPQRSTUVXWYZ_?";
  
  for (size_t i = 0; i < 55; i++) {
    if (c == accepted_chars[i])
      return true;
  }
  
  return false;
}

static bool is_number(const char c) {
  if (c == 0) return false;

  char accepted_num[] = "0123456789";
  
  for (size_t i = 0; i < 11; i++) {
    if (c == accepted_num[i])
      return true;
  }
  
  return false;
}

/* Lexer State Handlers */

void init_lexan(const char* b) {
  clexs.first = b;
  clexs.current = clexs.first;
  clexs.line = 1;
}

static char next_char() {
  if (*clexs.current != '\0') {
    clexs.current++;
    return clexs.current[-1];
  }

  return '\0';
}

static char check_next() {
  if (IS_EOF()) return '\0';
  return clexs.current[0];
}

static d_token dmake_symbol(dlex_types type) {
  d_token token;
  token.type = type;
  token.first = clexs.first;
  token.length = (int) (clexs.current - clexs.first);
  token.line = clexs.line;
  return token;
}

static d_token make_error(const char* reason) {
  d_token token;
  int size = (int) strlen(reason);
  token.type = DTK_ERROR;
  token.error_type = DCE_LEX;
  token.first = reason;
  token.length = size;
  token.line = clexs.line;
  return token;
}

d_token next_token() {
  clexs.first = clexs.current;

  while (*clexs.current != '\0') {
    char c = next_char();
    switch (c) {
      case ' ':
      case '\t':
      case '\r':
        clexs.first = clexs.current;
        break;
      
      case '\n': {
        clexs.line++;
        clexs.first = clexs.current;
        break;
      }

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        while (is_number(CURR_TOKEN())) {
          next_char();
        }

        if (CURR_TOKEN() == '.' && is_number(check_next())) {
          next_char();
          while (is_number(CURR_TOKEN())) {
            next_char();
          }
        }

        return dmake_symbol(DTK_NUMBER);
      }

      case '+': {
        if(check_next() == '+') {
          next_char();
          return dmake_symbol(DTK_CONCAT);
        }
        return dmake_symbol(DTK_ADD);
      }
      case '-': return dmake_symbol(DTK_SUB);
      case '*': return dmake_symbol(DTK_MUL);
      case '/': return dmake_symbol(DTK_DIV);
      case '(': return dmake_symbol(DTK_PAR_OPEN);
      case ')': return dmake_symbol(DTK_PAR_CLOSE);
      case '[': return dmake_symbol(DTK_BKT_OPEN);
      case ']': return dmake_symbol(DTK_BKT_CLOSE);
      case ',': return dmake_symbol(DTK_COMMA);
      case '.': {
        return dmake_symbol(DTK_DOT);
      }

      case '!':
        if(check_next() == '=') {
          next_char();
          return dmake_symbol(DTK_BNG_EQ);
        }
        return dmake_symbol(DTK_BNG);

      case '=':
        if(check_next() == '=') {
          next_char();
          return dmake_symbol(DTK_DEQ);
        }
        return dmake_symbol(DTK_EQ);

      case '<':
        if(check_next() == '=') {
          next_char();
          return dmake_symbol(DTK_LE);
        }
        return dmake_symbol(DTK_LS);

      case '>':
        if(check_next() == '=') {
          next_char();
          return dmake_symbol(DTK_BE);
        }
        return dmake_symbol(DTK_BG);

      case '"': {
        while (CURR_TOKEN() != '"' && !IS_EOF()) {
          if (CURR_TOKEN() == '\n') clexs.line++;
          next_char();
        }
        
        if (IS_EOF()) return make_error("Unterminated string.");

        next_char();
        return dmake_symbol(DTK_STRING);
      }

      case '#':
          while (CURR_TOKEN() != '\n' && !IS_EOF()) {
            next_char();
          }
        break;

      default: {
        if (is_alpha(c)) {
          while (is_alpha(CURR_TOKEN()) || is_number(CURR_TOKEN())) {
            next_char();
          }

          /* Check if is keyword */
          int ne = NUM_ELEMS(drax_t);
          for (int i = 0; i < ne; i++) {
            int size = strlen(drax_t[i].str);
            bool eql = true;
            for (int j = 0; j < size; j++) {
              if (drax_t[i].str[j] != clexs.first[j]) {
                eql = false;
                break;
              }
            }

            if (eql) {
              return dmake_symbol(drax_t[i].dtk);
            }
          }

          return dmake_symbol(DTK_ID);
        }
        return make_error("Unexpected token.");
      }
    }
  }

  return dmake_symbol(DTK_EOF);
}
