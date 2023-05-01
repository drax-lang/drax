#include "dstring.h"

#define fail_if_unspected_args(a, b) if (a != b) { return 0; };

#define match_dfunction(n, v, f, vm, a, s) if ((strcmp(n, v) == 0)) { return f(vm, a, s); }

/**
 * Connvert string to number
 * 
 *  "123".to_number() => 123
 */
static int dstr_to_number(d_vm* vm, int a, drax_string* ds) {
  fail_if_unspected_args(a, 0);
  push(vm, AS_VALUE(strtod(ds->chars, NULL)));
  return 1;
}

static char *strndup(const char *s, size_t n) {
  char *p;
  size_t len = strlen(s);

  if (n < len) len = n;
  p = (char *)malloc(len + 1);
  if (p) {
    memcpy(p, s, len);
    p[len] = '\0';
  }
  return p;
}

/**
 * returns a list of strings split by the delimiter
 * 
 *   "foo\nbar".split("\n") => ["foo", "bar"]
 */

static drax_value dstr_split(d_vm *vm, int a, drax_string* ds) {
  fail_if_unspected_args(a, 1);

  drax_string* dd = CAST_STRING(pop(vm));
  char *str = ds->chars;
  char *delim = dd->chars;

  drax_list *l = new_dlist(vm, 0);
  int len = strlen(str);
  char* p = str;
  char* s = str;
  char* e = s + len;

  while ((p = strstr(p, delim)) != NULL) {
    char *n = strndup(s, p - s);
    put_value_dlist(l, DS_VAL(new_dstring(vm, n, strlen(n))));
    p += strlen(delim);
    s = p;
  }

  if (s != e) {
    char *n = strndup(s, e - s);
    put_value_dlist(l, DS_VAL(new_dstring(vm, n, strlen(n))));
  }

  push(vm, DS_VAL(l));
  return 1;
}


/**
 * returns the length of the string
 * 
 *  "foo".length => 3
 */
static int dstr_length(d_vm* vm, int a, drax_string* ds) {
  fail_if_unspected_args(a, 0);
  push(vm, AS_VALUE(ds->length));;
  return 1;
}

static int dstr_slice(d_vm* vm, int a, drax_string* ds) {
    fail_if_unspected_args(a, 2); /* TEMP */

    int a2 = (int) (AS_NUMBER(pop(vm)));
    int a1 = (int) (AS_NUMBER(pop(vm)));

    char* str = ds->chars;
    if (str == NULL) return 0;

    int str_len = strlen(str);
    if (a1 < 0 || a1 >= str_len || a2 < 0) return 0;

    int slice_len = str_len - a1;
    if (a2 < slice_len) slice_len = a2;

    char* slice = (char*) malloc(slice_len + 1);
    strncpy(slice, str + a1, slice_len);
    slice[slice_len] = '\0';

    push(vm, DS_VAL(new_dstring(vm, slice, slice_len)));
    return 1;
}

int dstr_handle_str_call(d_vm* vm, char* n, int a, drax_value o) {
  drax_string* s = CAST_STRING(o);
  
  match_dfunction(n, "split",     dstr_split,     vm, a, s);
  match_dfunction(n, "length",    dstr_length,    vm, a, s);
  match_dfunction(n, "to_number", dstr_to_number, vm, a, s);
  match_dfunction(n, "slice",     dstr_slice,     vm, a, s);

  raise_drax_error(vm, "error: function '%s/%d' is not defined", n, a);

  return 0;
}
