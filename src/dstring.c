#include "dstring.h"
#include "ddefs.h"
#include <math.h>

drax_value dstr_to_number(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_string(a, stat);
  drax_string* ds = CAST_STRING(a);

  if (ds->length > 2 && ds->chars[0] == '0') {
    char* endptr = (char*) ds->chars + ds->length;

    if (ds->chars[1] == 'x') {
      DX_SUCESS_FN(stat);
      return AS_VALUE((double) strtol(ds->chars, &endptr, 16));
    }

    if (ds->chars[1] == 'b') {
      DX_SUCESS_FN(stat);
      return AS_VALUE((double) strtol(ds->chars + 2, &endptr, 2));
    }

    if (ds->chars[1] == 'o') {
      DX_SUCESS_FN(stat);
      return AS_VALUE((double) strtol(ds->chars + 2, &endptr, 8));
    }
  }

  DX_SUCESS_FN(stat);
  return AS_VALUE(strtod(ds->chars, NULL));
}

#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200809L
char* strndup(const char *s, size_t n) {
  char *p;
  size_t len = strlen(s);

  if (n < len) len = n;
  p = (char *) malloc(len + 1);
  if (p) {
    memcpy(p, s, len);
    p[len] = '\0';
  }
  return p;
}
#endif

int dnum_to_str(double n, char* out, size_t size) {
  if (n != n) return snprintf(out, size, "nan");
  if (isinf(n)) return snprintf(out, size, n < 0 ? "-inf" : "inf");

  /* the sign of a zero is kept (-0) */
  if (n == floor(n) && fabs(n) < 1e21) return snprintf(out, size, "%.0f", n);

  int p;
  for (p = 15; p <= 17; p++) {
    snprintf(out, size, "%.*g", p, n);
    if (strtod(out, NULL) == n) break;
  }
  return (int) strlen(out);
}

/**
 * escape sequences are converted by the parser (copy_dstring),
 * the string is printed as it is.
 */
char* str_format_output(const char* str) {
  return strndup(str, strlen(str));
}

drax_value dstr_split(d_vm *vm, int* stat) {
  drax_value v1 = pop(vm);
  drax_value a = pop(vm);
  return_if_is_not_string(v1, stat);
  return_if_is_not_string(a, stat);

  drax_string* ds = CAST_STRING(a);
  drax_string* dd = CAST_STRING(v1);
  char *str = ds->chars;
  char *delim = dd->chars;

  drax_list *l = new_dlist(vm, 0);
  int len = strlen(str);

  /* an empty separator splits the string in characters */
  if (dd->length == 0) {
    int i;
    for (i = 0; i < len; i++) {
      char *n = strndup(str + i, 1);
      put_value_dlist(l, DS_VAL(new_dstring(vm, n, 1)));
    }

    DX_SUCESS_FN(stat);
    return DS_VAL(l);
  }

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

  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

drax_value dstr_length(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_string(a, stat);
  drax_string* ds = CAST_STRING(a);
  DX_SUCESS_FN(stat);
  return AS_VALUE(ds->length);
}

drax_value dstr_copy(d_vm* vm, int* stat) {
  drax_value v2 = AS_VALUE(1);
  drax_value v1 = pop(vm);

  drax_value a = pop(vm);
  return_if_is_not_string(a, stat);
  return_if_is_not_number(v1, stat);
  return_if_is_not_number(v2, stat);

  drax_string* ds = CAST_STRING(a);
  
  int a2 = (int) (AS_NUMBER(v2));
  int a1 = (int) (AS_NUMBER(v1));

  char* str = ds->chars;
  if (str == NULL) {
    char* ts = (char*) malloc(1);
    ts[0] = '\0';
    DX_SUCESS_FN(stat);
    return DS_VAL(new_dstring(vm, (char*) ts, 0));
  }

  int str_len = strlen(str);
  if (a1 >= str_len || a2 < 0) {
    char* ts = (char*) malloc(1);
    ts[0] = '\0';
    DX_SUCESS_FN(stat);
    return DS_VAL(new_dstring(vm, (char*) ts, 0));
  }

  a1 = a1 < 0 ? ds->length + a1 : a1;
  int copy_len = str_len - a1;
  if (a2 < copy_len) copy_len = a2;

  char* copy = (char*) malloc(copy_len + 1);
  strncpy(copy, str + a1, copy_len);
  copy[copy_len] = '\0';

  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, copy, copy_len));
}

drax_value dstr_copy2(d_vm* vm, int* stat) {
  drax_value v2 = pop(vm);
  drax_value v1 = pop(vm);

  drax_value a = pop(vm);
  return_if_is_not_string(a, stat);
  return_if_is_not_number(v1, stat);
  return_if_is_not_number(v2, stat);

  drax_string* ds = CAST_STRING(a);
  
  int a2 = (int) (AS_NUMBER(v2));
  int a1 = (int) (AS_NUMBER(v1));

  char* str = ds->chars;
  if (str == NULL) {
    char* ts = (char*) malloc(1);
    ts[0] = '\0';
    DX_SUCESS_FN(stat);
    return DS_VAL(new_dstring(vm, (char*) ts, 0));
  }

  int str_len = strlen(str);
  if (a1 >= str_len || a2 < 0) {
    char* ts = (char*) malloc(1);
    ts[0] = '\0';
    DX_SUCESS_FN(stat);
    return DS_VAL(new_dstring(vm, (char*) ts, 0));
  }

  a1 = a1 < 0 ? ds->length + a1 : a1;
  int copy_len = str_len - a1;
  if (a2 < copy_len) copy_len = a2;

  char* copy = (char*) malloc(copy_len + 1);
  strncpy(copy, str + a1, copy_len);
  copy[copy_len] = '\0';

  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, copy, copy_len));
}

drax_value dstr_at(d_vm* vm, int* stat) {
  drax_value v1 = pop(vm);
  drax_value a = pop(vm);
  return_if_is_not_string(a, stat);
  return_if_is_not_number(v1, stat);

  double n = draxvalue_to_num(v1);
  drax_string* ds = CAST_STRING(a);

  if (n < 0) { n = ds->length + n; }

  if (n < 0 || n >= ds->length) {
    char* ts = (char*) malloc(1);
    ts[0] = '\0';
    
    DX_SUCESS_FN(stat);
    return DS_VAL(new_dstring(vm, (char*) ts, 0));
  }

  char* str = (char*) malloc(2);
  str[0] = ds->chars[(int) n];
  str[1] = '\0';

  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, str, 1));
}

drax_value dstr_to_uppercase(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_string(a, stat);
  drax_string* ds = CAST_STRING(a);

  char* upper = (char*) malloc(ds->length + 1);

  int i;
  for(i = 0; i < ds->length; i++) {
    upper[i] = toupper(ds->chars[i]);
  }
  upper[ds->length] = '\0';

  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, upper, strlen(upper)));
}

drax_value dstr_to_lowercase(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_string(a, stat);
  drax_string* ds = CAST_STRING(a);

  char* lower = (char*) malloc(ds->length + 1);

  int i;
  for(i = 0; i < ds->length; i++) {
    lower[i] = tolower(ds->chars[i]);
  }
  lower[ds->length] = '\0';

  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, lower, strlen(lower)));
}

drax_value dstr_contains(d_vm *vm, int* stat) {
  drax_value v1 = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_string(v1, stat);
  return_if_is_not_string(a, stat);

  drax_string* ds_haystack = CAST_STRING(a);
  drax_string* ds_needle = CAST_STRING(v1);

  if (ds_needle->length == 0) {
    DX_SUCESS_FN(stat);
    return DRAX_TRUE_VAL;
  }

  char* result = strstr(ds_haystack->chars, ds_needle->chars);

  DX_SUCESS_FN(stat);
  return (result != NULL) ? DRAX_TRUE_VAL : DRAX_FALSE_VAL;
}
