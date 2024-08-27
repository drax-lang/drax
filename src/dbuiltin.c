#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>

#include "dbuiltin.h"
#include "ddefs.h"
#include "dtypes.h"
#include "dtime.h"
#include "dvm.h"
#include "deval.h"
#include "dgc.h"
#include "dscheduler.h"

#include "mods/d_mod_os.h"
#include "mods/d_mod_http.h"

static drax_value __d_assert(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  if (!IS_BOOL(a)) {
    DX_SUCESS_FN(stat);
    return DRAX_TRUE_VAL;
  }

  if (CAST_BOOL(a)) {
     DX_SUCESS_FN(stat);
     return DRAX_TRUE_VAL;
  }
  
  DX_ERROR_FN(stat);

  if (!IS_STRING(b)) {
    return DS_VAL(new_derror(vm, (char*) "assert error"));
  }

  return DS_VAL(new_derror(vm, CAST_STRING(b)->chars));
}

static drax_value __d_sleep(d_vm* vm, int* stat) { 
  drax_value val = pop(vm);
  return_if_is_not_number(val, stat);
  
  double t = CAST_NUMBER(val);
  dx_sleep(t);
  DX_SUCESS_FN(stat);
  return DRAX_NIL_VAL;
}

static drax_value __d_read(d_vm* vm, int* stat) {
  drax_value val = pop(vm);
  return_if_is_not_string(val, stat);

  int buffer_size = 4096;
  char* buff = malloc(sizeof(char) * buffer_size);
  printf("%s", CAST_STRING(val)->chars);

  if (fgets(buff, buffer_size, stdin) == NULL) {
    DX_ERROR_FN(stat);
    return DS_VAL(new_derror(vm, (char *) "Fail to read input"));
  }
  char* r = replace_special_char('\n', 'n', buff);
  free(buff);
  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, r, strlen(r)));
}

static drax_value __d_print(d_vm* vm, int* stat) {
  print_drax(pop(vm), 0);
  dbreak_line();
  DX_SUCESS_FN(stat);
  return DRAX_NIL_VAL;
}

static drax_value __d_typeof(d_vm* vm, int* stat) {
  DX_SUCESS_FN(stat);
  drax_value val = pop(vm);
  if (IS_STRUCT(val)) {
    switch (DRAX_STYPEOF(val)) {
      case DS_NATIVE:
      case DS_FUNCTION: MSR(vm, "function");
      case DS_STRING: MSR(vm, "string");
      case DS_LIST: MSR(vm, "list");
      case DS_FRAME: MSR(vm, "frame");
      case DS_MODULE: MSR(vm, "module");
      default: break;
    }
  }
  
  if (IS_BOOL(val)) { MSR(vm, "boolean"); }
  if (IS_NIL(val)) { MSR(vm, "nil"); }
  if (IS_NUMBER(val)) { MSR(vm, "number"); }

  DX_SUCESS_FN(stat);
  MSR(vm, "none");
}

/* Module Math */

static drax_value __d_cos(d_vm* vm, int* stat) { 
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(cos(num));
}

static drax_value __d_acos(d_vm* vm, int* stat) { 
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(acos(num));
}

static drax_value __d_floor(d_vm* vm, int* stat) { 
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(floor(num));
}

static drax_value __d_ceil(d_vm* vm, int* stat) { 
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(ceil(num));
}

static drax_value __d_pow(d_vm* vm, int* stat) { 
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);
  return_if_is_not_number(b, stat);

  double n1 = CAST_NUMBER(a);
  double n2 = CAST_NUMBER(b);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(pow(n1, n2));
}

static drax_value __d_atan(d_vm* vm, int* stat) { 
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(atan(num));
}

static drax_value __d_atan2(d_vm* vm, int* stat) { 
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);
  return_if_is_not_number(b, stat);

  double n1 = CAST_NUMBER(a);
  double n2 = CAST_NUMBER(b);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(atan2(n1, n2));
}

static drax_value __d_cosh(d_vm* vm, int* stat) { 
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(cosh(num));
}

static drax_value __d_exp(d_vm* vm, int* stat) { 
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double n = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(exp(n));
}

static drax_value __d_fabs(d_vm* vm, int* stat) { 
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(fabs(num));
}

static drax_value __d_frexp(d_vm* vm, int* stat) { 
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  int exponent;
  double fraction = frexp(num, &exponent);

  drax_list* dl = new_dlist(vm, 2);

  put_value_dlist(dl, num_to_draxvalue(fraction));
  put_value_dlist(dl, num_to_draxvalue((double) exponent));

  DX_SUCESS_FN(stat);

  return DS_VAL(dl);
}

static drax_value __d_ldexp(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);
  return_if_is_not_number(b, stat);

  double n1 = CAST_NUMBER(a);
  double n2 = CAST_NUMBER(b);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(ldexp(n1, (int) n2));
}

static drax_value __d_log(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(log(num));
}

static drax_value __d_log10(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(log10(num));
}

static drax_value __d_modf(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  double integer_part;
  double fraction_part = modf(num, &integer_part);

  drax_list* dl = new_dlist(vm, 2);

  put_value_dlist(dl, num_to_draxvalue(fraction_part));
  put_value_dlist(dl, num_to_draxvalue(integer_part));

  DX_SUCESS_FN(stat);

  return DS_VAL(dl);
}

static drax_value __d_sin(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(sin(num));
}

static drax_value __d_asin(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(asin(num));
}

static drax_value __d_sinh(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(sinh(num));
}

static drax_value __d_sqrt(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(sqrt(num));
}


static drax_value __d_tan(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(tan(num));
}


static drax_value __d_tanh(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  double num = CAST_NUMBER(a);

  DX_SUCESS_FN(stat);

  return num_to_draxvalue(tanh(num));
}

/* Module OS */

static drax_value __d_get_env(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_string(a, stat);

  char* env = getenv(CAST_STRING(a)->chars);
  if (env == NULL) {
    DX_SUCESS_FN(stat);
    return DRAX_NIL_VAL;
  }

  DX_SUCESS_FN(stat);
  MSR(vm, env);
}

static drax_value __d_cmd(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_string(a, stat);

  char buf[4096];
  double status = (double) d_command(CAST_STRING(a)->chars, buf, sizeof(buf));
  char* r = replace_special_char('\n', 'n', buf);

  if (status != 0) {
    DX_ERROR_FN(stat);
    return DS_VAL(new_derror(vm, (char *) "Fail to execute command"));
  }
  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, r, strlen(r)));
}

static drax_value __d_cmd_with_status(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_string(a, stat);

  char buf[4096];
  double status = (double) d_command(CAST_STRING(a)->chars, buf, sizeof(buf));
  char* r = replace_special_char('\n', 'n', buf);

  drax_list* l = new_dlist(vm, 2);
  put_value_dlist(l, AS_VALUE(status));
  put_value_dlist(l, DS_VAL(new_dstring(vm, r, strlen(r))));
  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

static drax_value __d_mkdir(d_vm* vm, int* stat, int permission) {
  drax_value b = permission ? pop(vm) : DRAX_NIL_VAL;
  drax_value a = pop(vm);

  if (permission) { return_if_is_not_number(b, stat); }

  return_if_is_not_string(a, stat);
  
  mode_t mode = 0;
  if (permission) {
    double d = CAST_NUMBER(b);
    mode = (d >= 0 && d <= MODE_T_MAX) ? (mode_t) d : 0;
    
    if (mode == 0 && d != 0) {
      DX_ERROR_FN(stat);
      return DS_VAL(new_derror(vm, (char *) "Invalid mode"));
    }
  } else {
    mode = umask(0);
    /**
     * sets the permissions for creating the directory, ignoring the umask
     * mode = S_IRWXU | S_IRWXG | S_IRWXO;
     */
  }

  int r = mkdir(CAST_STRING(a)->chars, mode);

  if (r == -1) {
    DX_SUCESS_FN(stat);
    return DRAX_FALSE_VAL;
  }

  DX_SUCESS_FN(stat);
  return DRAX_TRUE_VAL;
}

static drax_value __d_mkdir1(d_vm* v, int* s) { return __d_mkdir(v, s, 0); }
static drax_value __d_mkdir2(d_vm* v, int* s) { return __d_mkdir(v, s, 1); }

static drax_value __d_system(d_vm* vm, int* stat) {
  drax_value a = pop(vm);  
  return_if_is_not_string(a, stat);
  int r = system(CAST_STRING(a)->chars);

  DX_SUCESS_FN(stat);
  return AS_VALUE((double) r);
}

void load_callback_fn(d_vm* vm, vm_builtin_setter* reg) {
  reg(vm, "assert", 2, __d_assert);
  reg(vm, "typeof", 1, __d_typeof);
  reg(vm, "sleep", 1, __d_sleep);
  reg(vm, "read", 1, __d_read);
  reg(vm, "print", 1, __d_print);
}

/**
 * Core module
 */

static drax_value __d_exit(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_number(a, stat);
  int ext_stat = (int) CAST_NUMBER(a);
  exit(ext_stat);
}

static drax_value __d_gc_swap(d_vm* vm, int* stat) {
  dgc_swap(vm);
  DX_SUCESS_FN(stat);
  return DRAX_NIL_VAL;
}

/**
 * Frame module
 */

static drax_value __d_frame_put(d_vm* vm, int* stat) {
  drax_value c = pop(vm);
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  return_if_is_not_string(b, stat);
  
  drax_frame* o = CAST_FRAME(a);
  drax_frame* n = new_dframe(vm, o->length + 1);
  n->length = o->length;

  memcpy(n->keys, o->keys, o->length * sizeof(int));
  memcpy(n->literals, o->literals, o->length * sizeof(char*));
  memcpy(n->values, o->values, o->length * sizeof(drax_value));

  put_value_dframe(n, CAST_STRING(b)->chars, c);

  DX_SUCESS_FN(stat);
  return DS_VAL(n);
}

static drax_value __d_list_concat(d_vm* vm, int* stat) {
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

static drax_value __d_list_head(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_list(a, stat);
  drax_list* l1 = CAST_LIST(a);

  DX_SUCESS_FN(stat);
  return l1->length > 0 ? l1->elems[0] : DRAX_NIL_VAL;
}

static drax_value __d_list_tail(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_list(a, stat);
  drax_list* l1 = CAST_LIST(a);

  drax_list* l = new_dlist(vm, l1->length -1);
  l->length = l1->length - 1;
  l->elems = l1->elems + 1;
  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

static drax_value __d_list_length(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  drax_list* l = CAST_LIST(a);

  DX_SUCESS_FN(stat);
  return AS_VALUE(l->length);
}


/**
 * Http calls
 */
static void callback_caller(d_vm* vm, drax_value call) {
  if (IS_NIL(call)) return;
  
  if (IS_NATIVE(call)) { 
    do_dcall_native(vm, call);
  }

  if (IS_FUNCTION(call)) {
   run_instruction_on_vm_pool(call, 0);
  }
}

static drax_value __d_start_server(d_vm* vm, int* stat) {
  drax_value call = pop(vm);
  drax_value opt = pop(vm);

  return_if_is_not_frame(opt, stat);
  drax_frame* ofr = CAST_FRAME(opt);
  
  /**
   * get port as string
   */
  char* port_string = (char*) "5000";
  drax_value port;
  if(get_value_dframe(ofr, (char*) "port", &port) != -1) {
    return_if_is_not_string(port, stat);
    port_string = CAST_STRING(port)->chars;
  }

  /**
   * get document_root as string
   */
  char* droot_string = (char*) ".";
  drax_value droot;
  if(get_value_dframe(ofr, (char*) "document_root", &droot) != -1) {
    return_if_is_not_string(droot, stat);
    droot_string = CAST_STRING(droot)->chars;
  }

  /**
   * get document_root as string
   */
  drax_value call_back_handler = DRAX_NIL_VAL;
  drax_value gfn;
  if(get_value_dframe(ofr, (char*) "request_handler", &gfn) != -1) {
    return_if_is_not_function(gfn, stat);

    if (CAST_FUNCTION(gfn)->arity != 1) {
      DX_ERROR_FN(stat);
      return DS_VAL(new_derror(vm, "Expected one argumento to 'request_handler' callback."));
    }

    call_back_handler = gfn;
  }

  char *options[] = {
    (char*) "document_root", droot_string, 
    (char*) "listening_ports", port_string, 
    NULL
  };

  #define IVALID_ARGS_CALLB (char *) "Do not expect arguments on callback function."

  if (!IS_NIL(call)) {
    if ((IS_NATIVE(call)) && (CAST_NATIVE(call)->arity != 0)) {
      DX_ERROR_FN(stat);
      return DS_VAL(new_derror(vm, IVALID_ARGS_CALLB));
    }

    if ((IS_FUNCTION(call)) && (CAST_FUNCTION(call)->arity != 0)) {
      DX_ERROR_FN(stat);
      return DS_VAL(new_derror(vm, IVALID_ARGS_CALLB));
    }
  }

  drax_value ctx = start_http_server(vm, options, callback_caller, call, call_back_handler);
  DX_SUCESS_FN(stat);

  drax_tid* tid = new_dtid(vm, ctx);

  /**
   * Fix callback execution bug
   */
  return DS_VAL(tid);
}

static drax_value __d_stop_server(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  drax_tid* tid = CAST_TID(v);
  stop_http_server(tid->value);

  DX_SUCESS_FN(stat);
  return DRAX_TRUE_VAL;
}

/**
 * Entry point for native modules
 */

void create_native_modules(d_vm* vm) {
  /**
   * OS module
  */
  drax_native_module* mos = new_native_module(vm, "os", 6);
  const drax_native_module_helper os_helper[] = {
    {1, "cmd", __d_cmd },
    {1, "cmd_with_status", __d_cmd_with_status },
    {1, "system", __d_system},
    {1, "get_env", __d_get_env },
    {1, "mkdir", __d_mkdir1 },
    {2, "mkdir", __d_mkdir2 },
  };

  put_fun_on_module(mos, os_helper, sizeof(os_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(mos));

  /**
   * Core module
  */
  drax_native_module* mcore = new_native_module(vm, "core", 2);
  const drax_native_module_helper core_helper[] = {
    {0, "gc_swap", __d_gc_swap },
    {1, "exit", __d_exit },
  };

  put_fun_on_module(mcore, core_helper, sizeof(core_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(mcore));

  /**
   * Frame Module
   */ 
  drax_native_module* frame = new_native_module(vm, "frame", 1);
  const drax_native_module_helper frame_helper[] = {
    {3, "put", __d_frame_put },
  };

  put_fun_on_module(frame, frame_helper, sizeof(frame_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(frame));

  /**
   * List Module
   */ 
  drax_native_module* list = new_native_module(vm, "list", 4);
  const drax_native_module_helper list_helper[] = {
    {2, "concat", __d_list_concat },
    {1, "head", __d_list_head},
    {1, "tail", __d_list_tail},
    {1, "length", __d_list_length},
  };
  
  put_fun_on_module(list, list_helper, sizeof(list_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(list));

  /**
   * Http Module
   */ 
  drax_native_module* http = new_native_module(vm, "http", 2);
  const drax_native_module_helper http_helper[] = {
    {2, "start", __d_start_server },
    {1, "stop", __d_stop_server},
  };
  
  put_fun_on_module(http, http_helper, sizeof(http_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(http));

  drax_native_module* math = new_native_module(vm, "math", 22);
    const drax_native_module_helper math_helper[] = {
      {1, "cos", __d_cos},
      {1, "cosh", __d_cosh},
      {1, "acos", __d_acos},
      {1, "floor", __d_floor},
      {1, "ceil", __d_ceil},
      {2, "pow", __d_pow},
      {1, "tan", __d_tan},
      {1, "tanh", __d_tanh},
      {1, "sqrt", __d_sqrt},
      {1, "atan", __d_atan},
      {2, "atan2", __d_atan2},
      {1, "exp", __d_exp},
      {1, "fabs", __d_fabs}, 
      {1, "frexp", __d_frexp},
      {2, "ldexp", __d_ldexp},
      {1, "log", __d_log},
      {1, "log10", __d_log10},
      {1, "modf", __d_modf},
      {1, "sin", __d_sin},
      {1, "sinh", __d_sinh},
      {1, "asin", __d_asin},
  };

  put_fun_on_module(math, math_helper, sizeof(math_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(math));

}
