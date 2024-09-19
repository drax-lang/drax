#include <stdio.h>
#include "dvm.h"
#include "doutopcode.h"

int inspect_opcode(drax_value* _ip, size_t level) {
  UNUSED(_ip);
  UNUSED(level);
#ifdef _AST_INSPECT_OP
  drax_value* ip = _ip;
  size_t i;
  for (i = 0; i < level; i++) {
    putchar(' ');
  }
  printf("op code ===============\n");
  while(true) {
    for (i = 0; i < level; i++) {
      putchar(' ');
    }
    
    switch (*(ip++)) {
      case OP_CONST: {
        printf("OP_CONST, <value>\n");
        ip++;
        break;
      }
      case OP_NIL: {
        printf("OP_NIL\n");
        break;
      }
      case OP_TRUE: {
        printf("OP_TRUE\n");
        break;
      }
      case OP_FALSE: {
        printf("OP_FALSE\n");
        break;
      }
      case OP_LIST: {
        printf("OP_LIST\n");
        break;
      }
      case OP_FRAME: {
        printf("OP_FRAME\n");
        break;
      }
      case OP_DSTR: {
        printf("OP_DSTR, <value>\n");
        ip++;
        break;
      }
      case OP_POP: {
        printf("OP_POP\n");
        break;
      }
      case OP_PUSH: {
        printf("OP_PUSH, <value>\n");
        ip++;
        break;
      }
      case OP_EQUAL: {
        printf("OP_EQUAL\n");
        break;
      }
      case OP_GREATER: {
        printf("OP_GREATER\n");
        break;
      }
      case OP_LESS: {
        printf("OP_LESS\n");
        break;
      }
      case OP_CONCAT: {
        printf("OP_CONCAT\n");
        break;
      }
      case OP_ADD: {
        printf("OP_ADD\n");
        break;
      }
      case OP_SUB: {
        printf("OP_SUB\n");
        break;
      }
      case OP_MUL: {
        printf("OP_MUL\n");
        break;
      }
      case OP_DIV: {
        printf("OP_DIV\n");
        break;
      }
      case OP_NOT: {
        printf("OP_NOT\n");
        break;
      }
      case OP_NEG: {
        printf("OP_NEG\n");
        break;
      }
      case OP_JMP: {
        printf("OP_JMP\n");
        ip++;
        ip++;
        break;
      }
      case OP_JMF: {
        printf("OP_JMF\n");
        ip++;
        ip++;
        break;
      }
      case OP_LOOP: {
        printf("OP_LOOP\n");
        break;
      }
      case OP_D_CALL_P:
      case OP_D_CALL: {
        printf("OP_D_CALL, <fn::value>\n");
        ip++;
        break;
      }
      case OP_CALL_G: {
        printf("OP_CALL_G, <value>\n");
        ip++;
        break;
      }
      case OP_CALL_L: {
        printf("OP_CALL_L, <value>\n");
        ip++;
        break;
      }
      case OP_FUN: {
        drax_function* f = CAST_FUNCTION(*(ip++));
        int bb = (*(ip++)) == DRAX_TRUE_VAL;

        if (f->name) {
          printf("OP_FUN<%s>, extern ref::bool<%d>\n", f->name, bb);
        } else {
          printf("OP_FUN, extern ref::bool<%d>\n", bb);
        }
        inspect_opcode(f->instructions->values, level + 2);
        break;
      }
      case OP_SET_G_ID: {
        char* s = (char*)(*ip);
        printf("OP_SET_G_ID, \"%s\"\n", s);
        ip++;
        break;
      }
      case OP_GET_G_ID: {
        char* s = (char*)(*ip);
        printf("OP_GET_G_ID, \"%s\"\n", s);
        ip++;
        break;
      }
      case OP_SET_L_ID: {
        char* s = (char*)(*ip);
        printf("OP_SET_L_ID, \"%s\"\n", s);
        ip++;
        break;
      }
      case OP_GET_L_ID: {
        char* s = (char*)(*ip);
        printf("OP_GET_L_ID, \"%s\"\n", s);
        ip++;
        break;
      }
      case OP_SET_I_ID: {
        printf("OP_SET_I_ID\n");
        ip++;
        break;
      }
      case OP_GET_I_ID: {
        printf("OP_GET_I_ID\n");
        break;
      }
      case OP_IMPORT: {
        printf("OP_IMPORT\n");
        break;
      }
      case OP_EXPORT: {
        printf("OP_EXPORT\n");
        break;
      }
      case OP_RETURN: {
        printf("OP_RETURN\n");
        if ((*(ip) == 0) && 
            (*(ip + 1) == 0) &&
            (*(ip + 2) == 0)) return 0;
        break;
      }
      case OP_EXIT: {
        for (i = 0; i < level; i++) {
          putchar(' ');
        }
        printf("OP_EXIT\n=======================\n\n");
        return 0;
      }
      default: {
        printf("<OP NOT FOUND>");
        break;
      }
    }

    if (*(ip) == 0) {
      for (i = 0; i < level; i++) {
        putchar(' ');
      }
      printf("\033[31m<Hole here>\033[0m\n");
    }

  }
  printf("=======================\n\n");
 
#endif

  return 0;
}
