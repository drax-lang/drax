#include "dstring.h"

#define args_fail_required_size(a, b, msg) \
   if (a != b) { { raise_drax_error(vm, msg); return 0; } };

#define args_fail_required_bigger_than(a, b) if (a < b) { return 0; };

#define args_fail_required_less_than(a, b) if (a > b) { return 0; };

#define match_dfunction(n, v, f, vm, a, s) if ((strcmp(n, v) == 0)) { return f(vm, a, s); }

/**
 * Connvert string to number
 * 
 *  "123".to_number() => 123
 */
static int dstr_to_number(d_vm* vm, int a, drax_string* ds) {
  args_fail_required_size(a, 0, "this function does not expect arguments");
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
  args_fail_required_size(a, 1, "only one argument is expected");

  drax_value v1 = pop(vm);
  dvalidate_string(vm, v1, "error: expected string as argument");

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
  args_fail_required_size(a, 0, "this function does not expect arguments");
  push(vm, AS_VALUE(ds->length));;
  return 1;
}

/**
 * returns the character at the given index
 * 
 * "drax lang".copy(0) => "d"
 * "drax lang".copy(0, 4) => "drax"
 * "drax lang".copy(-4, 4) => "lang"
 * "drax lang".copy(-4, -4) => ""
 * "drax lang".copy(0, 0) => ""
 */

static int dstr_copy(d_vm* vm, int a, drax_string* ds) {
    args_fail_required_bigger_than(a, 1);
    args_fail_required_less_than(a, 3);

    drax_value v2 = a > 1 ? pop(vm) : AS_VALUE(1);
    drax_value v1 = pop(vm);

    dvalidate_number(vm, v1, "error: copy arguments must be numbers");
    dvalidate_number(vm, v2, "error: copy arguments must be numbers");
    
    int a2 = (int) (AS_NUMBER(v2));
    int a1 = (int) (AS_NUMBER(v1));

    char* str = ds->chars;
    if (str == NULL) {
      push(vm, DS_VAL(new_dstring(vm, (char*) "", 0)));
      return 1;
    }

    int str_len = strlen(str);
    if (a1 >= str_len || a2 < 0) {
      push(vm, DS_VAL(new_dstring(vm, (char*) "", 0)));
      return 1;
    }

    a1 = a1 < 0 ? ds->length + a1 : a1;
    int copy_len = str_len - a1;
    if (a2 < copy_len) copy_len = a2;

    char* copy = (char*) malloc(copy_len + 1);
    strncpy(copy, str + a1, copy_len);
    copy[copy_len] = '\0';

    push(vm, DS_VAL(new_dstring(vm, copy, copy_len)));
    return 1;
}

int dstr_handle_str_call(d_vm* vm, char* n, int a, drax_value o) {
  drax_string* s = CAST_STRING(o);
  
  match_dfunction(n, "split",     dstr_split,     vm, a, s);
  match_dfunction(n, "length",    dstr_length,    vm, a, s);
  match_dfunction(n, "to_number", dstr_to_number, vm, a, s);
  match_dfunction(n, "copy",      dstr_copy,      vm, a, s);

  raise_drax_error(vm, "error: function '%s/%d' is not defined", n, a);

  return 0;
}
