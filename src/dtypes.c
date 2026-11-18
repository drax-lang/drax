#include <string.h>
#include <stdlib.h>
#include "stdio.h"
#include <stdarg.h>
#include "dtypes.h"

/* Helpers */

d_instructions* new_instructions_sized(int cap) {
  d_instructions* i = (d_instructions*) malloc(sizeof(d_instructions));
  i->instr_size = cap;
  i->instr_count = 0;
  i->file = NULL;
  i->lines = (int*) malloc(sizeof(int) * (size_t) cap);
  i->values = (drax_value*) malloc(sizeof(drax_value) * (size_t) cap);
  i->local_range = 0;
  i->local_names = NULL;
  i->local_names_cap = 0;
  i->gc_epoch = 0;
  i->gc_from = 0;
  i->extrn_ref_count = 0;
  i->extrn_ref_capacity = 0;
  i->extrn_ref = NULL;
  return i;
}

d_instructions* new_instructions() {
  d_instructions* i = new_instructions_sized(MAX_INSTRUCTIONS);
  i->extrn_ref_capacity = 4;
  i->extrn_ref = (d_extrn_ref*) malloc(sizeof(d_extrn_ref) * i->extrn_ref_capacity);
  return i;
}

