#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "deval.h"
#include "dstring.h"

#define D_NUMBER_STR_PRINT "%.17g"

static void print_list(drax_list* l) {
  putchar('[');
  int i;
  for (i = 0; i < l->length; i++) {
    print_drax(l->elems[i], 1);

    if ((i+1) < l->length) printf(", ");
  }
  putchar(']');
}

static void print_scalar_type(d_internal_types v) {
  switch (v) {
    case DIT_UNDEFINED: {
      printf("undefined");
      break;
    }
    case DIT_f64: {
      printf("f64");
      break;
    }
    case DIT_LIST: {
      printf("list");
      break;
    }
    case DIT_SCALAR: {
      printf("scalar");
      break;
    }
    case DIT_FUNCTION:
    case DIT_NATIVE: {
      printf("function");
      break;
    }
    case DIT_FRAME: {
      printf("frame");
      break;
    }
    case DIT_MODULE: {
      printf("module");
      break;
    }
    case DIT_TID: {
      printf("tid");
      break;
    }
    case DIT_STRING: {
      printf("string");
      break;
    }
    default: {
      printf("undefined");
      break;
    }
  }
}

static void print_scalar(drax_scalar* l) {
  printf("<<");
  print_scalar_type(l->_stype);
  printf(" :: ");

  int i;
  double* _d_el;

  if (DIT_f64 == l->_stype) {
    _d_el = (double*) l->elems;
  }
  for (i = 0; i < l->length; i++) {
    if (DIT_f64 == l->_stype) {
      printf("%.15f", _d_el[i]);
    } else {
      print_drax(l->elems[i], 1);
    }

    if ((i+1) < l->length) printf(", ");
  }
  putchar('>');
  putchar('>');
}

static void print_frame(drax_frame* f) {
  putchar('{');
  int i;
  for (i = 0; i < f->length; i++) {
    printf("'%s': ", f->literals[i]);
    print_drax(f->values[i], 1);

    if ((i+1) < f->length) printf(", ");
  }
  putchar('}');
}

static void print_string(const char* str, int formated) {
  if (!formated) {
    char* tmpstr = str_format_output(str);
    if (tmpstr == NULL) {
      fprintf(stderr, D_COLOR_RED"runtime error: cannot format string"D_COLOR_RESET);
      return;
    }
    printf(formated ? "\"%s\"" : "%s", tmpstr);
    free(tmpstr);
  } else {
    printf(formated ? "\"%s\"" : "%s", str);
  }
}

static void print_d_struct(drax_value value, int formated) {
  switch (DRAX_STYPEOF(value)) {
    case DS_LIST:
      print_list(CAST_LIST(value));
      break;

    case DS_SCALAR:
      print_scalar(CAST_SCALAR(value));
      break;

    case DS_FRAME:
      print_frame(CAST_FRAME(value));
      break;

    case DS_FUNCTION:
      printf("<function/%d>", CAST_FUNCTION(value)->arity);
      break;

    case DS_NATIVE:
      printf("<function:native>");
      break;

    case DS_MODULE:
      printf("<module>");
      break;

    case DS_STRING:
      print_string(CAST_STRING(value)->chars, formated);
      break;

    case DS_ERROR:
      printf("error");
      break;

    case DS_TID:
      printf("<tid::%ld>", CAST_TID(value)->value);
      break;
  }
}

void drax_print_error(const char* format, va_list args) {
  vfprintf(stderr, format, args);
}

void print_drax(drax_value value, int formated) {
  if (IS_BOOL(value)) {
    printf(CAST_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_NUMBER(value)) {
    printf(D_NUMBER_STR_PRINT, CAST_NUMBER(value));
  } else if(IS_MODULE(value)) {
    printf("<module:%s>", CAST_MODULE(value)->name);
  } else if (IS_STRUCT(value)) {
    print_d_struct(value, formated);
  }
}

void dbreak_line() { putchar('\n'); }