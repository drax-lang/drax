#include <string.h>
#include <stdlib.h>
#include "stdio.h"
#include <stdarg.h>
#include "dtypes.h"

/* Conversions */

double draxvalue_to_num(drax_value value) {
  double num;
  memcpy(&num, &value, sizeof(drax_value));
  return num;
}

drax_value num_to_draxvalue(double num) {
  drax_value value;
  memcpy(&value, &num, sizeof(double));
  return value;
}

/* Helpers */

d_instructions* new_instructions() {
  d_instructions* i = (d_instructions*) malloc(sizeof(d_instructions));
  i->instr_size = MAX_INSTRUCTIONS;
  i->instr_count = 0;
  i->lines = (int*) malloc(sizeof(int) * MAX_INSTRUCTIONS);
  i->values = (drax_value*) malloc(sizeof(drax_value) * MAX_INSTRUCTIONS);
  i->local_range = 0;
  return i;
}

