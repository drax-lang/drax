#include "dlex.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

curr_lex_state clexs;

#define PREV_TOKEN()  (clexs.current[-1])

#define CURR_TOKEN()  (*clexs.current)

#define IS_EOF()      (*clexs.current == '\0')

#define NUM_ELEMS(x)  (sizeof(x) / sizeof((x)[0]))

static const drax_tokens drax_t[] = {
  {"and",    DTK_AND},
  {"as",     DTK_AS},
  {"do",     DTK_DO},
  {"else",   DTK_ELSE},
  {"end",    DTK_END},
  {"false",  DTK_FALSE},
  {"fun",    DTK_FUN},
  {"if",     DTK_IF},
  {"nil",    DTK_NIL},
  {"or",     DTK_OR},
  {"true",   DTK_TRUE},
  {"import", DTK_IMPORT},
  {"export", DTK_EXPORT},
 };

/* Helpers */
static bool is_alpha(const char c) {
  if (c == 0) return false;

  char accepted_chars[] = "abcdefghijklmnopqrstuvxwyzABCDEFGHIJKLMNOPQRSTUVXWYZ_?";
  
  size_t i = 0;
  for (i = 0; i < 55; i++) {
    if (c == accepted_chars[i])
      return true;
  }
  
  return false;
}

static bool is_number(const char c) {
  if (c == 0) return false;

  char accepted_num[] = "0123456789";
  
  size_t i = 0;
  for (i = 0; i < 11; i++) {
    if (c == accepted_num[i])
      return true;
  }
  
  return false;
}

static bool is_hex_number(const char c) {
  if (c == 0) return false;

  char accepted_num[] = "0123456789abcdefABCDEF";
  
  size_t i = 0;
  for (i = 0; i < 23; i++) {
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
  return clexs.current[1];
}

static d_token dmake_symbol(dlex_types type) {
  d_token token;
  token.type = type;
  token.first = clexs.first;
  token.length = (int) (clexs.current - clexs.first);
  token.line = clexs.line;
  return token;
}

static d_token dmake_number(d_num_type num_type) {
  d_token token = dmake_symbol(DTK_NUMBER);
  token.num_type = num_type;
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

static d_num_type get_number_type(char c) {
  switch (c) {
  case 'x':
    return DNT_HEX;

  case 'b':
    return DNT_BIN;
  
  case 'o':
    return DNT_OCT;
  
  default:
    return DNT_DECIMAL;
  }
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
        d_num_type nt = get_number_type(CURR_TOKEN());

        if (nt > 0) {
          next_char();
        }

        while (is_number(CURR_TOKEN()) || (is_hex_number(CURR_TOKEN()) && nt == DNT_HEX)) {
          next_char();
        }

        if (CURR_TOKEN() == '.' && nt != DNT_DECIMAL) {
          return make_error("Unexpected number.");
        }

        if (CURR_TOKEN() == '.' && is_number(check_next())) {
          next_char();
          while (is_number(CURR_TOKEN())) {
            next_char();
          }
        }

        return dmake_number(nt);
      }

      case '+': {
        if(CURR_TOKEN() == '+') {
          next_char();
          return dmake_symbol(DTK_CONCAT);
        }
        return dmake_symbol(DTK_ADD);
      }
      case '-': return dmake_symbol(DTK_SUB);
      case '&': return dmake_symbol(DTK_AMP);
      case '*': return dmake_symbol(DTK_MUL);
      case '/': return dmake_symbol(DTK_DIV);
      case '(': return dmake_symbol(DTK_PAR_OPEN);
      case ')': return dmake_symbol(DTK_PAR_CLOSE);
      case '[': return dmake_symbol(DTK_BKT_OPEN);
      case ']': return dmake_symbol(DTK_BKT_CLOSE);
      case '{': return dmake_symbol(DTK_CBR_OPEN);
      case '}': return dmake_symbol(DTK_CBR_CLOSE);
      case ',': return dmake_symbol(DTK_COMMA);
      case ':': return dmake_symbol(DTK_COLON);
      case '|': {
        if(CURR_TOKEN() == '>') {
          next_char();
          return dmake_symbol(DTK_PIPE);
        }
        return make_error("Invalid token.");
      }
      case '.': {
        return dmake_symbol(DTK_DOT);
      }

      case '!':
        if(CURR_TOKEN() == '=') {
          next_char();
          return dmake_symbol(DTK_BNG_EQ);
        }
        return dmake_symbol(DTK_BNG);

      case '=':
        if(CURR_TOKEN() == '=') {
          next_char();
          return dmake_symbol(DTK_DEQ);
        }
        return dmake_symbol(DTK_EQ);

      case '<':
        if(CURR_TOKEN() == '=') {
          next_char();
          return dmake_symbol(DTK_LE);
        }
        return dmake_symbol(DTK_LS);

      case '>':
        if(CURR_TOKEN() == '=') {
          next_char();
          return dmake_symbol(DTK_BE);
        }
        return dmake_symbol(DTK_BG);
        
      case '\'': {
        int is_scaped = 0;
        while (
          !((CURR_TOKEN() == '\'') && (!is_scaped)) &&
          (!IS_EOF())
        ) {
          is_scaped = 0;
          if (CURR_TOKEN() == '\n') clexs.line++;
          if (CURR_TOKEN() == '\\') {
            char sc = check_next();

            if (sc == '\\') {
              next_char();
            } else if (sc == 't') {
              next_char();
            } else {
              is_scaped = 1;
            }
          }

          next_char();
        }
        
        if (IS_EOF()) return make_error("Unterminated string.");

        next_char();
        return dmake_symbol(DTK_STRING);
      }

      case '"': {
        if (CURR_TOKEN() == '"') {
          next_char();

          if (CURR_TOKEN() == '"') {
            int process_string = 1;
            while (process_string && !IS_EOF()) {
              next_char();
              if (CURR_TOKEN() == '"') {
                next_char();
                if (CURR_TOKEN() == '"') {
                  next_char();
                  if (CURR_TOKEN() == '"') {
                    next_char();
                    return dmake_symbol(DTK_MSTRING);
                  } 
                }     
              }
              
            }

          }
          return dmake_symbol(DTK_DSTRING); /**new mark */
        }

        int is_scaped = 0;
        while (!(CURR_TOKEN() == '"' && !is_scaped) && !IS_EOF()) {
          is_scaped = 0;
          
          if (CURR_TOKEN() == '\n') clexs.line++;
          if (CURR_TOKEN() == '\\') {
            char sc = check_next();

            if (sc == '\\') {
              next_char();
            } else if (sc == 't') {
              next_char();
            } else {
              is_scaped = 1;
            }
          }

          next_char();
        }
        
        if (IS_EOF()) return make_error("Unterminated string.");

        next_char();
        return dmake_symbol(DTK_DSTRING);
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
          int i, j;
          for (i = 0; i < ne; i++) {
            int sz1 = strlen(drax_t[i].str);
            int sz2 = (int) (clexs.current - clexs.first);

            if (sz1 != sz2) continue;

            bool eql = true;
            for (j = 0; j < sz1; j++) {
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
