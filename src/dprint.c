#include <stdlib.h>
#include <stdio.h>
#include "dtypes.h"
#include "dprint.h"

void bshow_error(beorn_state* curr) {
  const char * et = berrors_to_str(curr->et);
  if (NULL != curr->trace) {
    printf("%sline: %ld, %s: %s%s", BRED, curr->trace->line, et, curr->cval, BDEF);    
    return;
  }
  printf("%s%s: %s%s", BRED, et, curr->cval, BDEF);
}

void bprint_pack(beorn_state* curr) {
  putchar('{');
  for (int i = 0; i < curr->length; i++) {
    bprint(curr->child[i]);
    if (i + 1 < curr->length) putchar(' ');
  }
  putchar('}');
}

void bprint_list(beorn_state* curr) {
  putchar('[');
  for (int i = 0; i < curr->length; i++) {
    bprint(curr->child[i]);
    if (i + 1 < curr->length) {
      putchar(',');
      putchar(' ');
    }
  }
  putchar(']');
}

void bprint_expression(beorn_state* curr) {
    putchar('(');
    for (int i = 0; i < curr->length; i++) {
      bprint(curr->child[i]);
      if (i + 1 < curr->length) putchar(' ');
    }
    putchar(')');
}

void bprint(beorn_state* curr) {
  bprint_default(curr, 1);
}

static int prtstr(int sstr, char* cval) {
  if (sstr) {
    printf("\"%s\"", cval);
  } else {
    printf("%s", cval);
  }
  return 1;
}

void bprint_default(beorn_state* curr, int sstr) {
  switch (curr->type) {
    case BT_INTEGER:      printf("%lld", curr->ival);    break;
    case BT_FLOAT:        printf("%.20Lg", curr->fval); break;
    case BT_STRING:       prtstr(sstr,  curr->cval);    break;
    case BT_SYMBOL:       printf("%s",  curr->cval);    break;
    case BT_FUNCTION:     printf("#function<>");        break;
    case BT_LAMBDA:       printf("#lambda<>");          break;
    case BT_PACK:         bprint_pack(curr);            break;
    case BT_LIST:         bprint_list(curr);            break;
    case BT_EXPRESSION:   bprint_expression(curr);      break;
    case BT_NIL:          printf("nil");                break;
    case BT_ERROR:        bshow_error(curr);            break;
    
    default:
      bshow_error(new_error(BRUNTIME_ERROR, "unspected token."));
      break;
  }

  free(curr);
}

void bbreak_line() { putchar('\n'); }
void bspace_line() { putchar(' '); }
