#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "deval.h"
#include "dstring.h"
#include "drax.h"

extern int is_teractive_mode;

#define DPRINT_COLOR1(_color, str, arg1) \
  if (is_teractive_mode) {printf(_color str D_COLOR_RESET, arg1);} else {printf(str, arg1);}

static void print_value(drax_value value, int raw, int depth);

static void print_list(drax_list* l, int depth) {
  putchar('[');
  int i;
  for (i = 0; i < l->length; i++) {
    print_value(l->elems[i], 0, depth + 1);

    if ((i+1) < l->length) printf(", ");
  }
  putchar(']');
}

static void print_frame(drax_frame* f, int depth) {
  putchar('{');
  int i;
  for (i = 0; i < f->length; i++) {
    printf("'%s': ", f->literals[i]);
    print_value(f->values[i], 0, depth + 1);

    if ((i+1) < f->length) printf(", ");
  }
  putchar('}');
}

/* a string quoted and escaped for display: \", \\, \n, \t, \r and \xNN for the other control characters */
static void print_quoted(const char* str) {
  putchar('"');
  for (; *str; str++) {
    switch (*str) {
      case '"':  fputs("\\\"", stdout); break;
      case '\\': fputs("\\\\", stdout); break;
      case '\n': fputs("\\n", stdout); break;
      case '\t': fputs("\\t", stdout); break;
      case '\r': fputs("\\r", stdout); break;
      default:
        /* other control characters (ESC, backspace...) would act on the terminal */
        if ((unsigned char) *str < 0x20 || *str == 0x7f) printf("\\x%02x", (unsigned char) *str);
        else putchar(*str);
    }
  }
  putchar('"');
}

/**
 * raw: the text as it is (what print does with a string).
 * Otherwise the string is quoted, so "1" and 1 (or "a, b" and two
 * elements) can not be mistaken in the output of a list, a frame, inspect
 * and the shell.
 */
static void print_string(const char* str, int raw) {
  if (raw) {
    printf("%s", str);
    return;
  }

  if (is_teractive_mode) fputs(D_COLOR_GREEN, stdout);
  print_quoted(str);
  if (is_teractive_mode) fputs(D_COLOR_RESET, stdout);
}

static void print_d_struct(drax_value value, int raw, int depth) {
  switch (DRAX_STYPEOF(value)) {
    case DS_LIST:
      if (depth >= MAX_VALUE_DEPTH) { printf("..."); break; }
      print_list(CAST_LIST(value), depth);
      break;

    case DS_FRAME:
      if (depth >= MAX_VALUE_DEPTH) { printf("..."); break; }
      print_frame(CAST_FRAME(value), depth);
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
      print_string(CAST_STRING(value)->chars, raw);
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

static void print_value(drax_value value, int raw, int depth) {
  if (IS_BOOL(value)) {
    printf(CAST_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_NUMBER(value)) {
    char num[32];
    dnum_to_str(CAST_NUMBER(value), num, sizeof(num));
    DPRINT_COLOR1(D_COLOR_BLUE, "%s", num);
  } else if(IS_MODULE(value)) {
    printf("<module:%s>", CAST_MODULE(value)->name);
  } else if (IS_STRUCT(value)) {
    print_d_struct(value, raw, depth);
  }
}

void print_drax(drax_value value, int raw) {
  print_value(value, raw, 0);
}

void dbreak_line() { putchar('\n'); }