#include <stdio.h>
#include "dvm.h"
#include "doutopcode.h"

int inspect_opcode(d_vm* vm) {
  UNUSED(vm);
#ifdef _AST_INSPECT_OP

  drax_value* ip = vm->ip;
  printf("op code ===============\n");
  while(true) {
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
        break;
      }
      case OP_JMF: {
        printf("OP_JMF\n");
        break;
      }
      case OP_LOOP: {
        printf("OP_LOOP\n");
        break;
      }
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
      case OP_CALL_I: {
        printf("OP_CALL_I, <value>\n");
        ip++;
        break;
      }
      case OP_CALL_IP: {
        printf("OP_CALL_IP, <value>\n");
        ip++;
        break;
      }
      case OP_FUN: {
        printf("OP_FUN, <value>, <value::bool>\n");
        ip++;
        ip++;
        break;
      }
      case OP_AFUN: {
        printf("AOP_FUN, <value>, <value::bool>\n");
        ip++;
        ip++;
        break;
      }
      case OP_SET_G_ID: {
        printf("OP_SET_G_ID\n");
        ip++;
        break;
      }
      case OP_GET_G_ID: {
        printf("OP_GET_G_ID\n");
        ip++;
        break;
      }
      case OP_SET_L_ID: {
        printf("OP_SET_L_ID\n");
        ip++;
        break;
      }
      case OP_GET_L_ID: {
        printf("OP_GET_L_ID\n");
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
      case OP_GET_REF: {
        printf("OP_GET_REF\n");
        ip++;
        break;
      }
      case OP_GET_REFI: {
        printf("OP_GET_REFI\n");
        ip++;
        break;
      }
      case OP_IMPORT: {
        printf("OP_IMPORT, <value::path>, <value::alias>\n");
        ip++;
        ip++;
        break;
      }
      case OP_EXPORT: {
        printf("OP_EXPORT\n");
        break;
      }
      case OP_RETURN: {
        printf("OP_RETURN\n");
        break;
      }
      case OP_EXIT: {
        printf("OP_EXIT\n");
        printf("=======================\n");
        return 0;
      }
      default: {
        printf("<OP NOT FOUND>");
        break;
      }
    }

    if (*(ip) == 0) return 0;
  }
  printf("=======================\n");
 
#endif

  return 0;
}
