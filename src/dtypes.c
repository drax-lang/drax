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

