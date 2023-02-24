#ifdef _AST_INSPECT

#include "dinspect.h"
#include "dtypes.h"
#include <stdio.h>

void __ast_inspect__(drax_state* curr, int ident) {
  if (ident == 1) { printf("PROGRAM\n"); }
  for (int i = 0; i < curr->length; i++) {

    for (int j = 0; j < ident; j++) {
      printf("|    ");
    }

    switch (curr->child[i]->type) {
      case BT_PROGRAM: {
        printf("[PROGRAM]\n");
        __ast_inspect__(curr->child[i], ident + 1);
        break;
      }
      case BT_EXPRESSION:  {
        printf("[EXPRESSION]\n");
        __ast_inspect__(curr->child[i], ident + 1);
        break;
      }
      case BT_INTEGER: {
        printf("[INTEGER | %li]\n", curr->child[i]->val);
        break;
      }
      case BT_FLOAT: {
        printf("[FLOAT]\n");
        break;
      }
      case BT_STRING: {
        printf("[STRING]\n");
        break;
      }
      case BT_ERROR: {
        printf("[ERROR]\n");
        break;
      }
      case BT_FUNCTION: {
        printf("[FUNCTION]\n");
        break;
      }
      case BT_LAMBDA: {
        printf("[LAMBDA]\n");
        break;
      }
      case BT_NIL: {
        printf("[NIL]\n");
        break;
      }
      case BT_PACK:        {
        printf("[PACK]\n");
        break;
      }
      case BT_LIST:        {
        printf("[LIST]\n");
        break;
      }
      case BT_SYMBOL:      {
        printf("[SYMBOL | %s]\n", (char*) curr->child[i]->val);
        break;
      }      
      default:  {
        printf("undefined\n");
        break;
      }
    }
  }
}
#endif