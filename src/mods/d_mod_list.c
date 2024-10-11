#include "./d_mod_list.h"


/**
 * List Module
 */

drax_value __d_list_at(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_list(a, stat);
  return_if_is_not_number(b, stat);

  double n = draxvalue_to_num(b);
  drax_list* ll = CAST_LIST(a);

  if (n < 0) { n = ll->length + n; }

  if (n < 0 || n >= ll->length) {
    DX_SUCESS_FN(stat);
    return DRAX_NIL_VAL;
  }

  DX_SUCESS_FN(stat);
  return ll->elems[(int) n];
}

drax_value __d_list_concat(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_list(a, stat);
  return_if_is_not_list(b, stat);

  drax_list* l1 = CAST_LIST(a);
  drax_list* l2 = CAST_LIST(b);

  drax_list* l = new_dlist(vm, l1->length + l2->length);
  l->length = l1->length + l2->length;

  memcpy(l->elems, l1->elems, l1->length * sizeof(drax_value));
  memcpy(l->elems + l1->length, l2->elems, l2->length * sizeof(drax_value));

  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

drax_value __d_list_head(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_list(a, stat);
  drax_list* l1 = CAST_LIST(a);

  DX_SUCESS_FN(stat);
  return l1->length > 0 ? l1->elems[0] : DRAX_NIL_VAL;
}

drax_value __d_list_tail(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_list(a, stat);
  drax_list* l1 = CAST_LIST(a);

  drax_list* l = new_dlist(vm, l1->length -1);
  l->length = l1->length - 1;
  l->elems = l1->elems + 1;
  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

drax_value __d_list_length(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  drax_list* l = CAST_LIST(a);

  DX_SUCESS_FN(stat);
  return AS_VALUE(l->length);
}

drax_value __d_list_is_empty(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  drax_list* l = CAST_LIST(a);

  DX_SUCESS_FN(stat);
  return l->length ? DRAX_FALSE_VAL : DRAX_TRUE_VAL;
}

drax_value __d_list_is_present(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  drax_list* l = CAST_LIST(a);

  DX_SUCESS_FN(stat);
  return l->length ? DRAX_TRUE_VAL : DRAX_FALSE_VAL;
}

drax_value __d_list_remove_at(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  return_if_is_not_number(b, stat);

  drax_list* l = CAST_LIST(a);
  int at = (int) CAST_NUMBER(b);
  at = at < 0 ? l->length + at : at;

  if(at >= l->length) {
    DX_SUCESS_FN(stat);
    return DS_VAL(new_dlist(vm, 0));
  }

  drax_list* nl = new_dlist(vm, l->length - 1);
  nl->length = l->length - 1;

  memcpy(nl->elems, l->elems, at * sizeof(drax_value));
  memcpy(nl->elems + at, l->elems + at + 1, (nl->length - at) * sizeof(drax_value));

  DX_SUCESS_FN(stat);
  return DS_VAL(nl);
}

drax_value __d_list_insert_at(d_vm* vm, int* stat) {
  drax_value c = pop(vm);
  drax_value b = pop(vm);
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  return_if_is_not_number(b, stat);

  drax_list* l = CAST_LIST(a);
  int at = (int) CAST_NUMBER(b);
  at = at < 0 ? l->length + at : at;

  if(at >= l->length) {
    DX_SUCESS_FN(stat);
    return DS_VAL(new_dlist(vm, 0));
  }

  drax_list* nl = new_dlist(vm, l->length + 1);
  nl->length = l->length + 1;

  memcpy(nl->elems, l->elems, at * sizeof(drax_value));
  memcpy(&nl->elems[at], &c, sizeof(drax_value));
  memcpy(nl->elems + at + 1, l->elems + at, (l->length - at) * sizeof(drax_value));

  DX_SUCESS_FN(stat);
  return DS_VAL(nl);
}

drax_value __d_list_replace_at(d_vm* vm, int* stat) {
  drax_value c = pop(vm);
  drax_value b = pop(vm);
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  return_if_is_not_number(b, stat);

  drax_list* l = CAST_LIST(a);
  int at = (int) CAST_NUMBER(b);
  at = at < 0 ? l->length + at : at;

  if(at >= l->length) {
    DX_SUCESS_FN(stat);
    return DS_VAL(new_dlist(vm, 0));
  }
  
  drax_list* nl = new_dlist(vm, l->length);
  nl->length = l->length;

  memcpy(nl->elems, l->elems, at * sizeof(drax_value));
  memcpy(&nl->elems[at], &c, sizeof(drax_value));
  memcpy(nl->elems + at + 1, l->elems + at + 1, (l->length - at - 1) * sizeof(drax_value));
  
  DX_SUCESS_FN(stat);
  return DS_VAL(nl);
}

drax_value __d_list_slice(d_vm* vm, int* stat) {
  drax_value c = pop(vm);
  drax_value b = pop(vm);
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  return_if_is_not_number(b, stat);

  drax_list* l = CAST_LIST(a);
  int from = (int) CAST_NUMBER(b);
  int to = (int) CAST_NUMBER(c);

  from = from < 0 ? l->length + from : from;
  to = to < 0 ? l->length + to : to;

  if(to <= from || from >= l->length || to > l->length) {
    DX_SUCESS_FN(stat);
    return DS_VAL(new_dlist(vm, 0));
  }

  drax_list* nl = new_dlist(vm, abs(to - from));
  nl->length = abs(to - from);

  memcpy(nl->elems, l->elems + from, abs(to - from) * sizeof(drax_value));

  DX_SUCESS_FN(stat);
  return DS_VAL(nl);
}

drax_value __d_list_sum(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);

  drax_list* l = CAST_LIST(a);
  double res = 0;

  int i;
  for(i = 0; i < l->length; i++) {
    return_if_is_not_number(l->elems[i], stat);
    res += CAST_NUMBER(l->elems[i]);
  }

  DX_SUCESS_FN(stat);
  return AS_VALUE(res);
}

drax_value __d_list_sparse(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_number(a, stat);

  int n = (int) CAST_NUMBER(a);

  if(n < 0) {
    DX_SUCESS_FN(stat);
    return DS_VAL(new_dlist(vm, 0));
  }

  drax_list* ll = new_dlist(vm, n);
  ll->length = n;
  drax_value v = num_to_draxvalue(0.0);

  int i;
  for(i = 0; i < ll->length; i++) {
    ll->elems[i] = v;
  }

  DX_SUCESS_FN(stat);
  return DS_VAL(ll);
}

drax_value __d_list_hypot(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);

  double result = 0.0;

  drax_list* ll = CAST_LIST(a);

  int i;
  for(i = 0; i < ll->length; i++) {
    return_if_is_not_number(ll->elems[i], stat);
    double n = CAST_NUMBER(ll->elems[i]);
    result += n * n;
  }

  DX_SUCESS_FN(stat);
  return num_to_draxvalue(sqrt(result));
}

drax_value __d_list_dot(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  return_if_is_not_list(b, stat);

  double result = 0.0;

  drax_list* l1 = CAST_LIST(a);
  drax_list* l2 = CAST_LIST(b);

  if(l1->length != l2->length) {
    DX_ERROR_FN(stat);
    return DS_VAL(new_derror(vm, (char*) "expected lists with the same size"));
  }

  int i;
  for(i = 0; i < l1->length; i++) {
    return_if_is_not_number(l1->elems[i], stat);
    return_if_is_not_number(l2->elems[i], stat);
    result += CAST_NUMBER(l1->elems[i]) * CAST_NUMBER(l2->elems[i]);
  }

  DX_SUCESS_FN(stat);
  return num_to_draxvalue(result);
}

drax_value __d_list_pop(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);

  drax_list* l1 = CAST_LIST(a);

  if(l1->length == 0) {
    DX_SUCESS_FN(stat);
    return DS_VAL(l1);
  }

  drax_list* nl = new_dlist(vm, l1->length - 1);
  nl->length = l1->length - 1;

  memcpy(nl->elems, l1->elems, sizeof(drax_value) * nl->length);

  DX_SUCESS_FN(stat);
  return DS_VAL(nl);
}

drax_value __d_list_shift(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);

  drax_list* l1 = CAST_LIST(a);

  if(l1->length == 0) {
    DX_SUCESS_FN(stat);
    return DS_VAL(l1);
  }

  drax_list* nl = new_dlist(vm, l1->length - 1);
  nl->length = l1->length - 1;

  memcpy(nl->elems, l1->elems + 1, sizeof(drax_value) * nl->length);

  DX_SUCESS_FN(stat);
  return DS_VAL(nl);
}

drax_value __d_list_zip(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_list(a, stat);
  return_if_is_not_list(b, stat);

  drax_list* l1 = CAST_LIST(a);
  drax_list* l2 = CAST_LIST(b);

  if(l1->length != l2->length) {
    DX_ERROR_FN(stat);
    return DS_VAL(new_derror(vm, (char*) "expected lists with the same size"));
  }

  drax_list* nl = new_dlist(vm, l1->length);

  int i;
  for(i = 0; i < l1->length; i++) {
    drax_list* in = new_dlist(vm, 2);
    put_value_dlist(in, l1->elems[i]);
    put_value_dlist(in, l2->elems[i]);

    put_value_dlist(nl, DS_VAL(in));
  }

  DX_SUCESS_FN(stat);
  return DS_VAL(nl);
}
