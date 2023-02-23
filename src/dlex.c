#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dlex.h"

size_t b_index = 0;
size_t b_prev_index = 0;
char* buffer;

static const char *const drax_tokens [] = {
  "}", "{", "]", "[", ",", "do", 
  "end", "float", "fun", "if", "else", "import", 
  "integer", "lambda", ")", "(", "string", "<symbol>", 

  "and", "or",
  
  /* cmp */ 
  "!=", "!==", "!===",
  "=", "==", "===",

  /* arith op */
  "+", "/", "*", "-",

  /* bool op */
  "ls", "bg", "le", "be", 
  
  "nil", "\n","<EOF>"
};


char* append_char(const char *str, const char c) {
  size_t size = 0;
  if (str != 0) size = strlen(str);

  char *s = (char *) calloc(size + 2, sizeof(char));

  for (size_t i = 0; i < size; i++) {
    s[i] = str[i];
  }

  s[size] = c;
  s[size + 1] = '\0';
  return s;
}

int is_symbol(const char c) {
  char accepted_chars[] = "abcdefghijklmnopqrstuvxwyzABCDEFGHIJKLMNOPQRSTUVXWYZ_0123456789?";
  
  for (size_t i = 0; i < 65; i++)
  {
    if (c == accepted_chars[i])
      return 1;
  }
  
  return 0;
}

int is_number(const char c) {
  char accepted_num[] = ".0123456789";
  
  for (size_t i = 0; i < 11; i++) {
    if (c == accepted_num[i])
      return 1;
  }
  
  return 0;
}

d_token* bmake_string(char* val) {
  d_token* v =(d_token*) malloc(sizeof(d_token));
  v->type = TK_STRING;
  v->cval = val;
  b_index++;
  return v;
}

d_token* bmake_symbol(dlex_types type) {
  d_token* v =(d_token*) malloc(sizeof(d_token));
  v->type = type;
  v->cval = NULL;
  b_index++;
  return v;
};

d_token* bmake_return(char* keyword) {
  dlex_types t = TK_SYMBOL;

  for (int i = 0; i < TK_EOF; i++)
  {
    if (strcmp(keyword, drax_tokens[i]) == 0) {
      t = i;
      break;
    }
  }

  d_token* v =(d_token*) malloc(sizeof(d_token));
  v->type = t;

  if (TK_SYMBOL == t)
    v->cval = keyword;

  b_index++;
  return v;
};


d_token* bmake_int(dlex_types type, long long val) {
  d_token* v =(d_token*) malloc(sizeof(d_token));
  v->type = type;
  v->ival = val;
  b_index++;
  return v;
}

d_token* bmake_float(dlex_types type, long double val) {
  d_token* v =(d_token*) malloc(sizeof(d_token));
  v->type = type;
  v->fval = val;
  b_index++;
  return v;
}

int init_lexan(char* b) {
  b_index = 0;
  buffer = b;
  return b_index;
}

d_token* b_check_next(int* jump) {
  size_t i_bk = b_index;
  if ((NULL != jump) && (*jump > 0)) {
    b_index = *jump;
  }

  d_token* r = lexan();

  if (NULL != jump) { *jump = b_index; }

  b_index = i_bk;
  return r;
}

d_token* b_check_prev() {
  size_t i_bk = b_index;
  b_index = b_prev_index;
  d_token* r = lexan();
  b_index = i_bk;
  return r;
}

d_token* lexan() {
  b_prev_index = b_index;
  char* bword = 0;
  while (b_index < strlen(buffer)) {
   char c = buffer[b_index];

    switch (c)
    {
      case ' ': break;
      case '\r': break;
      case '\n': return bmake_symbol(TK_BREAK_LINE);

      case '-':
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
        if ((c == '-') && (!is_number(buffer[b_index + 1]))) {
          return bmake_symbol(TK_SUB);
        }

        char* num = append_char("", c);

        int isf = 0;
        while (b_index < strlen(buffer)) {
          char sc = buffer[b_index + 1];

          if (!is_number(sc)) break;
          if (sc == '.') isf = 1;

          b_index++;
          num = append_char(num, sc);
        }

        if (!isf) {
          long long vi = strtoll(num, NULL, 10);
          return bmake_int(TK_INTEGER, vi);
        } else {
          double vf = strtod(num, NULL);
          return bmake_float(TK_FLOAT, vf);
        }
        break;
      };

      case '#': {
        while (b_index < strlen(buffer)) {
          char sc = buffer[b_index];

          if (sc == '\n')
            return bmake_symbol(TK_BREAK_LINE);
            
          b_index++;
        }
        break;
      }
      case '+': return bmake_symbol(TK_ADD);
      case '*': return bmake_symbol(TK_MUL);
      case '/': return bmake_symbol(TK_DIV);

      case '{': return bmake_symbol(TK_BRACE_OPEN);
      case '}': return bmake_symbol(TK_BRACE_CLOSE);

      case '(': return bmake_symbol(TK_PAR_OPEN);
      case ')': return bmake_symbol(TK_PAR_CLOSE);
      case '[': return bmake_symbol(TK_BRACKET_OPEN);
      case ']': return bmake_symbol(TK_BRACKET_CLOSE);
      case '>': 
        if ('=' == buffer[b_index+1]) {
          b_index++;
          return bmake_symbol(TK_BE);
        }
        return bmake_symbol(TK_BG);

      case '<':
        if ('=' == buffer[b_index+1]) {
          b_index++;
          return bmake_symbol(TK_LE);
        }
        return bmake_symbol(TK_LS);
      
      case '!':
        if ('=' == buffer[b_index+1] && '=' == buffer[b_index+2]) {
          b_index+= 2;
          return bmake_symbol(TK_NOT_DEQ);
        }

        if ('=' == buffer[b_index+1]) {
          b_index++;
          return bmake_symbol(TK_NOT_EQ);
        }
        break; /* unspected */

      case '=':
        if ('=' == buffer[b_index+1] && '=' == buffer[b_index+2]) {
          b_index += 2;
          return bmake_symbol(TK_TEQ); 
        }

        if ('=' == buffer[b_index+1]) {
          b_index++;
          return bmake_symbol(TK_DEQ);
        }
        return bmake_symbol(TK_EQ); 

      case ',': return bmake_symbol(TK_COMMA);
      case '"': {
        while (b_index < strlen(buffer)) {
          b_index++;
          char sc = buffer[b_index];

          if (sc == '"') {
            return bmake_string(bword);
          };

          bword = append_char(bword, sc);
        }
        break;
      }

      default: {
        if (is_symbol(c)) {
          bword = append_char(bword, c);
          while (b_index < strlen(buffer)) {
            char sc = buffer[b_index + 1];

            if(is_symbol(sc)) {
              bword = append_char(bword, sc);
              b_index ++;
            } else {
              return bmake_return(bword);
            }
          }
          return bmake_return(bword);
        }
      }
    }

    b_index++;
  }
  
  b_index = 0;
  return bmake_symbol(TK_EOF);
}
