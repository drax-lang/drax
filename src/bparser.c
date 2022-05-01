#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bparser.h"

/* helpers */
char* get_new_str(char *str, char c) { // FIXME
    size_t len = strlen(str);
    char *nstr = malloc(len + 2);

    strcpy(nstr, str);
    nstr[len] = c;
    nstr[len + 1] = '\0';
    return nstr;
}

void throw_error(char* err) {
  printf("\x1B[31mPARSER_ERROR: %s\x1B[0m\n", err);
  exit(1); // FIXME
}

int is_symbol(char c) {
  char accepted_chars[] = "abcdefghijklmnopqrstuvxwyzABCDEFGHIJKLMNOPQRSTUVXWYZ_-0123456789?";
  
  for (size_t i = 0; i < 65; i++)
  {
    if (c == accepted_chars[i])
      return 1;
  }
  
  return 0;
}

int is_number(char c) {
  char accepted_num[] = ".0123456789";
  
  for (size_t i = 0; i < 11; i++)
  {
    if (c == accepted_num[i])
      return 1;
  }
  
  return 0;
}

int add_child(beorn_state** root, beorn_state* child) {
  if ((*root)->length <= 0) {
    (*root)->length++;
    (*root)->child = malloc(sizeof(beorn_state*));
    (*root)->child[(*root)->length - 1] = child;
    return 1;
  } else {

    beorn_state* crr;
    if ((((*root)->child[(*root)->length - 1]->type == BT_PACK)  || 
         ((*root)->child[(*root)->length - 1]->type == BT_EXPRESSION )) &&
         ((*root)->child[(*root)->length - 1]->closed == 0))
    {
      crr  = (*root)->child[(*root)->length - 1];
      if(add_child(&crr, child)) return 1;
    } else {
      crr = (*root);
    }

    crr->length++;

    if (crr->length <= 1) { // unnecessary
      crr->child = malloc(sizeof(beorn_state*));
    } else {
      crr->child = realloc(crr->child, sizeof(beorn_state*) * crr->length);
    }
    crr->child[crr->length - 1] = child;
    return 1;
  }

}

int close_pack_freeze(beorn_state** root, types ct) {
  if ((*root)->length == 0) return 0;

  if ((((*root)->child[(*root)->length - 1]->type == BT_PACK)  || 
       ((*root)->child[(*root)->length - 1]->type == BT_EXPRESSION )) &&
       ((*root)->child[(*root)->length - 1]->closed == 0))
  {
    if (close_pack_freeze( &((*root)->child[(*root)->length - 1]), ct )) {
      return 1;
    } else {
      if ((*root)->child[(*root)->length - 1]->type == ct) {
        (*root)->child[(*root)->length - 1]->closed = 1;
        return 1;
      }
    }
  }

  return 0;
}

beorn_state* beorn_parser(char *input) {
  beorn_state* bs = malloc(sizeof(beorn_state *));
  bs->type = BT_PROGRAM;
  bs->child = NULL;
  bs->length = 0;

  char* bword = "";
  int b_index = 0;
  while (b_index < strlen(input)) {
    char c = input[b_index];

    switch (c)
    {
      case ' ': break;
      case '\n': break;
      case '\r': break;
      case ',': break;

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
        char* num = "";
        num = get_new_str(num, c);

        int isf = 0;
        while (b_index < strlen(input)) {
          char sc = input[b_index + 1];

          if (!is_number(sc)) break;
          if (sc == '.') isf = 1;

          b_index++;
          num = get_new_str(num, sc);
        }

        if (!isf) {
          int vi = strtol(num, NULL, 10);
          add_child(&bs, new_integer(vi));
        } else {
          long double vf = strtold(num, NULL);
          add_child(&bs, new_float(vf));
        }
        break;
      };

      case '{':
        bword = get_new_str(bword, c);
        add_child(&bs, new_pack_freeze(bword));
        bword = "";
        break;
      case '}':
        if(!close_pack_freeze(&bs, BT_PACK))
          throw_error("pack freeze pair not found.\n");
        break;


      case '+':
      case '-':
      case '*':
      case '/': {
        char* ctmp = get_new_str(bword, c);
        add_child(&bs, new_symbol(ctmp));
        bword = "";
        break;
      }

      case '(': {
        char* ctmp = get_new_str(bword, c);
        add_child(&bs, new_expression(ctmp));
        bword = "";
        break;
      }

      case ')': {
        if(!close_pack_freeze(&bs, BT_EXPRESSION))
          throw_error("expression pair not found.\n");
        break;
      }

      case '[':
      case ']': {
        char* ctmp = get_new_str(bword, c);
        add_child(&bs, new_symbol(ctmp));
        bword = "";
        break;
      }
      
      case '"': {
        while (b_index < strlen(input)) {
          b_index++;
          char c = input[b_index];

          if (c == '"') {
            add_child(&bs, new_string(bword));
            bword = "";
            break;
          };

          bword = get_new_str(bword, c);
        }
        break;
      }

      case '#': {
        while (b_index < strlen(input)) {
          char c = input[b_index];

          b_index++;
          if (c == '\n')
            break;
        }
      }
      default: {
        if (is_symbol(c)) {
          bword = get_new_str(bword, c);
          while (b_index < strlen(input)) {
            char c = input[b_index + 1];

            if(is_symbol(c)) {
              bword = get_new_str(bword, c);
              b_index ++;
            } else {
              add_child(&bs, new_symbol(bword));
              bword = "";
              break;
            }
            
          }
        }
        break;
      }
    }

    b_index++;
  }
  
  return bs;
}
