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

/**
 * Values use NaN boxing: nil, booleans and structs live inside
 * the NaN space. Every NaN is stored as the canonical quiet NaN,
 * so a NaN produced by a computation is always a number.
 */
#define D_CANONICAL_NAN ((drax_value) 0x7ff8000000000000)

drax_value num_to_draxvalue(double num) {
  drax_value value;
  if (num != num) return D_CANONICAL_NAN;

  memcpy(&value, &num, sizeof(double));
  return value;
}

/* Helpers */

d_instructions* new_instructions() {
  d_instructions* i = (d_instructions*) malloc(sizeof(d_instructions));
  i->instr_size = MAX_INSTRUCTIONS;
  i->instr_count = 0;
  i->file = NULL;
  i->lines = (int*) malloc(sizeof(int) * MAX_INSTRUCTIONS);
  i->values = (drax_value*) malloc(sizeof(drax_value) * MAX_INSTRUCTIONS);
  i->local_range = 0;
  i->extrn_ref_count = 0;
  i->extrn_ref_capacity = 4;
  i->extrn_ref = (d_extrn_ref*) malloc(sizeof(d_extrn_ref) * i->extrn_ref_capacity);
  return i;
}

