#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bparser.h"

/* state handler */
stack_bpsm* create_stack_bpsm() {
  stack_bpsm* gsb = (stack_bpsm*) malloc(sizeof(stack_bpsm));
  gsb->size = 0;
  gsb->bpsm = (bpsm**) malloc(sizeof(bpsm*));
  return gsb;
}

int increment_stack_bpsm(stack_bpsm* gsb) {
  if ((NULL == gsb) || (gsb->size == 0)) return 0;
  gsb->bpsm[gsb->size - 1]->count++;
  return 0;
}

int del_first_stack_bpsm(stack_bpsm* gsb) {
  if ((NULL == gsb) || (gsb->size == 0)) return 0;

  free(gsb->bpsm[gsb->size -1]);
  gsb->size--;
  gsb->bpsm = (bpsm**) realloc(gsb->bpsm, sizeof(bpsm*) * gsb->size);
  return 0;
}

int add_elem_stack_bpsm(stack_bpsm* gsb) {
  if (NULL == gsb) return 0;

  gsb->size++;
  gsb->bpsm = (bpsm**) realloc(gsb->bpsm, sizeof(bpsm*) * gsb->size);
  gsb->bpsm[gsb->size -1] = (bpsm*) malloc(sizeof(bpsm));
  gsb->bpsm[gsb->size -1]->mode = BP_NONE;
  gsb->bpsm[gsb->size -1]->count = 0;
  return 0;
}

/* helpers */
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

beorn_state* new_definition() {
  beorn_state* bdef = new_expression("(");
  return bdef;
}

beorn_state* new_parser_error(const char* msg) {
  beorn_state* err = new_error(BPARSER_ERROR, msg);
  return err;
}

int is_symbol(const char c) {
  char accepted_chars[] = "abcdefghijklmnopqrstuvxwyzABCDEFGHIJKLMNOPQRSTUVXWYZ_-0123456789?!=";
  
  for (size_t i = 0; i < 67; i++)
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

int is_simple_expressions(const char* key) {
  return (strcmp("set", key) == 0) || (strcmp("let", key) == 0) ||
         (strcmp("fun", key) == 0) || (strcmp("lambda", key) == 0);
}

esm keyword_to_bpsm(const char* key) {
  if (strcmp("set", key) == 0) {
    return BP_SIMPLE_DEFINITIONS;
  } else if (strcmp("let", key) == 0) {
    return BP_SIMPLE_DEFINITIONS;
  } else if (strcmp("lambda", key) == 0) {
    return BP_LAMBDA_DEFINITION;
  } else if (strcmp("fun", key) == 0) {
    return BP_FUNCTION_DEFINITION;
  }

  return BP_NONE;
}

int initialize_new_state(stack_bpsm* gs, esm s) {
  add_elem_stack_bpsm(gs);

  gs->bpsm[gs->size -1]->mode = s;
  return 1;
}

int bauto_state_update(stack_bpsm* gs, beorn_state* b, esm tp, int lenght) {
  if (gs->size <= 0) return 0;

  bpsm* curr = gs->bpsm[gs->size -1];
  if (curr->mode == tp) {
    if (curr->count == lenght) {
      b->closed = 1;
      close_pending_structs(gs, b, BT_EXPRESSION);   
      del_first_stack_bpsm(gs);
    }
  }
  return 0; 
}

void auto_state_update(stack_bpsm* gs, beorn_state* b) {
  bauto_state_update(gs, b, BP_SIMPLE_DEFINITIONS,  3);
  bauto_state_update(gs, b, BP_FUNCTION_DEFINITION, 4);
  bauto_state_update(gs, b, BP_LAMBDA_DEFINITION,   3);
}

int add_child(stack_bpsm* gs, beorn_state* root, beorn_state* child) {
  if (root->length <= 0) {
    root->length++;
    root->child = (beorn_state**) malloc(sizeof(beorn_state*));
    root->child[0] = child;
    increment_stack_bpsm(gs);
    return 1;
  } else {
    beorn_state* crr;
    if (((root->child[root->length - 1]->type == BT_PACK) || 
         (root->child[root->length - 1]->type == BT_LIST) || 
         (root->child[root->length - 1]->type == BT_EXPRESSION)) &&
         (root->child[root->length - 1]->closed == 0))
    {
      crr  = root->child[root->length - 1];
      if (add_child(gs, crr, child)) return 1;
    } else {
      crr = root;
    }

    crr->length++;

    if (crr->length <= 1) {
      crr->child = (beorn_state**) malloc(sizeof(beorn_state*));
    } else {
      crr->child = (beorn_state**) realloc(crr->child, sizeof(beorn_state*) * crr->length);
    }
    crr->child[crr->length - 1] = child;
    increment_stack_bpsm(gs);
    return 1;
  }

}

int close_pending_structs(stack_bpsm* gs, beorn_state* root, types ct) {
  if (root->length == 0) return 0;

  if (((root->child[root->length - 1]->type == BT_PACK) || 
       (root->child[root->length - 1]->type == BT_LIST) ||
       (root->child[root->length - 1]->type == BT_EXPRESSION)) &&
       (root->child[root->length - 1]->closed == 0))
  {
    if (close_pending_structs(gs, root->child[root->length - 1], ct )) {
      return 1;
    } else {
      if (root->child[root->length - 1]->type == ct) {
        root->child[root->length - 1]->closed = 1;
        del_first_stack_bpsm(gs);
        return 1;
      }
    }
  }

  return 0;
}

beorn_state* beorn_parser(char *input) {
  stack_bpsm* gsb = create_stack_bpsm();

  beorn_state* bs = (beorn_state*) malloc(sizeof(beorn_state *));
  bs->type = BT_PROGRAM;
  bs->child = (beorn_state**) malloc(sizeof(beorn_state*));
  bs->length = 0;

  char* bword = 0;
  size_t b_index = 0;
  int b_parser_error = 0;
  while (b_index < strlen(input) && (!b_parser_error)) {
    char c = input[b_index];
    auto_state_update(gsb, bs);

    switch (c)
    {
      case ' ': break;
      case '\n': break;
      case '\r': break;
      case ',': break;

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
        if ((c == '-') && (!is_number(input[b_index + 1]))) {
          char* ctmp = append_char(bword, c);
          add_child(gsb, bs, new_symbol(ctmp));
          bword = 0;
          break;
        }

        char* num = append_char("", c);

        int isf = 0;
        while (b_index < strlen(input)) {
          char sc = input[b_index + 1];

          if (!is_number(sc)) break;
          if (sc == '.') isf = 1;

          b_index++;
          num = append_char(num, sc);
        }

        if (!isf) {
          int vi = strtol(num, NULL, 10);
          add_child(gsb, bs, new_integer(vi));
        } else {
          long double vf = strtold(num, NULL);
          add_child(gsb, bs, new_float(vf));
        }
        break;
      };

      case '{':
        bword = append_char(bword, c);
        add_child(gsb, bs, new_pack(bword));
        
        if (initialize_new_state(gsb, BP_DINAMIC) == 0)
          return new_error(BPARSER_ERROR, "Invalid format to '%s'", bword);

        bword = 0;
        break;
      case '}':
        if(!close_pending_structs(gsb, bs, BT_PACK))
          return new_parser_error("pack freeze pair not found.");
        break;

      case '+':
      case '*':
      case '/': {
        char* ctmp = append_char(bword, c);
        add_child(gsb, bs, new_symbol(ctmp));
        bword = 0;
        break;
      }

      case '(': {
        char* ctmp = append_char(bword, c);
        add_child(gsb, bs, new_expression(ctmp));

        if (initialize_new_state(gsb, BP_DINAMIC) == 0)
          return new_error(BPARSER_ERROR, "Invalid format to '%s'", bword);

        bword = 0;
        break;
      }

      case ')': {
        if(!close_pending_structs(gsb, bs, BT_EXPRESSION))
          return new_parser_error("expression pair not found.");
        break;
      }

      case '[': {
        char* ctmp = append_char(bword, c);
        add_child(gsb, bs, new_list(ctmp));

        if (initialize_new_state(gsb, BP_DINAMIC) == 0)
          return new_error(BPARSER_ERROR, "Invalid format to '%s'", bword);

        bword = 0;
        break;
      }

      case ']': {
        if(!close_pending_structs(gsb, bs, BT_LIST))
          return new_parser_error("list pair not found.");
        break;
      }
      
      case '"': {
        while (b_index < strlen(input)) {
          b_index++;
          char sc = input[b_index];

          if (sc == '"') {
            add_child(gsb, bs, new_string(bword));
            bword = 0;
            break;
          };

          bword = append_char(bword, sc);
        }
        break;
      }

      case '#': {
        while (b_index < strlen(input)) {
          char sc = input[b_index];

          b_index++;
          if (sc == '\n')
            break;
        }
        break;
      }

      default: {
        if (is_symbol(c)) {
          bword = append_char(bword, c);
          while (b_index < strlen(input)) {
            char sc = input[b_index + 1];

            if(is_symbol(sc)) {
              bword = append_char(bword, sc);
              b_index ++;
            } else {

              if ((is_simple_expressions(bword)) &&
                  ((bs->length == 0) || ((bs->length > 0) && (bs->child[bs->length -1]->closed == 1)))
                ) {
                esm cbpsm = keyword_to_bpsm(bword);

                if (initialize_new_state(gsb, cbpsm) == 0)
                 return new_error(BPARSER_ERROR, "Invalid format to '%s'", bword);

                add_child(0, bs, new_definition());              
              }

              add_child(gsb, bs, new_symbol(bword));
              bword = 0;
              break;
            }   
          }
        }
        break;
      }
    }

    b_index++;
  }
  
  free(gsb);
  return bs;
}
