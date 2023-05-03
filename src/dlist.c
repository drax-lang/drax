#include "dlist.h"


#define args_fail_required_size(a, b, msg) \
   if (a != b) { { raise_drax_error(vm, msg); return 0; } };

#define args_fail_required_bigger_than(a, b) if (a < b) { return 0; };

#define args_fail_required_less_than(a, b) if (a > b) { return 0; };

#define match_dfunction(n, v, f, vm, a, s) if ((strcmp(n, v) == 0)) { return f(vm, a, s); }

/**
 * returns the length of the list
 * 
 *  ["foo", "bar"].length => 2
 */
static int dlist_length(d_vm* vm, int a, drax_list* dl) {
  args_fail_required_size(a, 0, "this function does not expect arguments");
  push(vm, AS_VALUE(dl->length));;
  return 1;
}

/**
 * returns the element of the index
 * 
 *  ["foo", "bar"].get(0) => "foo"
 *  ["foo", "bar"][0]  => "foo"
 *  ["foo", "bar"][1]  => "bar"
 *  ["foo", "bar"][-1] => nil
 */
static int dlist_get(d_vm* vm, int a, drax_list* dl) {
  args_fail_required_size(a, 1, "expected one argument.");
  drax_value v1 = pop(vm);
  dvalidate_number(vm, v1, "error: copy arguments must be numbers");
  double n = draxvalue_to_num(v1);

  if (n < 0) { n = dl->length + n; }

  if (n < 0 || n >= dl->length) {
    push(vm, DRAX_NIL_VAL);
    return 1;
  }

  push(vm, dl->elems[(int) n]);
  return 1;
}

int dstr_handle_list_call(d_vm* vm, char* n, int a, drax_value o) {
  drax_list* l = CAST_LIST(o);
  
  match_dfunction(n, "get",    dlist_get,    vm, a, l);
  match_dfunction(n, "length", dlist_length, vm, a, l);
  
  raise_drax_error(vm, "error: function '%s/%d' is not defined", n, a);

  return 0;
}
