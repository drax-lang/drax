#include "dstring.h"

#define fail_if_unspected_args(a, b) if (a != b) { return 0; };

#define define_dmethod(n, v, f, vm, a, s) if ((strcmp(n, v) == 0)) { return f(vm, a, s); }

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
 */
static int dstr_length(d_vm* vm, int a, drax_string* ds) {
  fail_if_unspected_args(a, 0);
  push(vm, AS_VALUE(ds->length));;
  return 1;
}

int dstr_handle_str_call(d_vm* vm, char* n, int a, drax_value o) {
  drax_string* s = CAST_STRING(o);
  
  define_dmethod(n, "split",     dstr_split,     vm, a, s);
  define_dmethod(n, "length",    dstr_length,    vm, a, s);
  define_dmethod(n, "to_number", dstr_to_number, vm, a, s);

  return 0;
}
