#include "dstring.h"
#include "ddefs.h"

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

drax_value dstr_to_tensor(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_string(a, stat);
  drax_string* ds = CAST_STRING(a);

  drax_tensor* t = new_dtensor(vm, ds->length, DIT_u8);
  t->length = ds->length;
  uint8_t* e = (uint8_t*) t->elems;

  int i;
  for(i = 0; i < ds->length; i++) e[i] = (uint8_t) ds->chars[i];

  DX_SUCESS_FN(stat);
  return DS_VAL(t);
}

drax_value dstr_from_tensor(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_tensor(a, stat);
  drax_tensor* t = CAST_TENSOR(a);

  if(DIT_u8 != t->_stype) {
    return DS_VAL(new_derror(vm, (char*) "Expected a u8 tensor"));
  }

  uint8_t* elems = (uint8_t*) t->elems;
  char* c = (char*) malloc(t->length * sizeof(char));

  int i;
  for(i = 0; i < t->length; i++) c[i] = (char) elems[i];

  drax_string* s = new_dstring(vm, c, t->length);

  DX_SUCESS_FN(stat);
  return DS_VAL(s);
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

char* str_format_output(const char* str) {
  char* tstr = strndup(str, strlen(str));

  if (tstr == NULL) return NULL;

  int i, j;
  for (i = 0, j = 0; tstr[i] != '\0'; i++, j++) {
    if (tstr[i] == '\\') {
      if (tstr[i+1] == 'n') {
        tstr[j] = '\n';
        i++;
      } else if (tstr[i+1] == 'r') {
        tstr[j] = '\r';
        i++;
      }
       else if (tstr[i+1] == '\\') {
        tstr[j] = '\\';
        i++;
      } else {
        tstr[j] = tstr[i];
      }
    } else {
      tstr[j] = tstr[i];
    }
  }
  tstr[j] = '\0';
  return tstr;
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
