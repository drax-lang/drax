#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "dbuiltin.h"
#include "ddefs.h"
#include "dtypes.h"
#include "dtime.h"
#include "dvm.h"

#include "mods/d_mod_os.h"

/* Make String as Return */
#define MSR(g, c)  \
    return DS_VAL(copy_dstring(g, c, strlen(c)));

#define DX_SUCESS_FN(v) *v = 1;

#define DX_ERROR_FN(v) *v = 0;

#define return_if_is_not_string(v, s) \
  if (!IS_STRING(v)) { \
    DX_ERROR_FN(s); \
    return DS_VAL(new_derror(vm, (char *) "Expected string as argument")); \
  }

#define return_if_is_not_number(v, s) \
  if (!IS_NUMBER(v)) { \
    DX_ERROR_FN(s); \
    return DS_VAL(new_derror(vm, (char *) "Expected number as argument")); \
  }

#define return_if_is_not_frame(v, s) \
  if (!IS_FRAME(v)) { \
    DX_ERROR_FN(s); \
    return DS_VAL(new_derror(vm, (char *) "Expected frame as argument")); \
  }

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
  UNUSED(vm);
  drax_value val = pop(vm);
  return_if_is_not_number(val, stat);
  
  double t = CAST_NUMBER(val);
  dx_sleep(t);
  DX_SUCESS_FN(stat);
  return DRAX_NIL_VAL;
}

static drax_value __d_read(d_vm* vm, int* stat) {
  UNUSED(vm);
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

void load_callback_fn(d_vm* vm, vm_builtin_setter* reg) {
  reg(vm, "assert", 2, __d_assert);
  reg(vm, "typeof", 1, __d_typeof);
  reg(vm, "sleep", 1, __d_sleep);
  reg(vm, "read", 1, __d_read);
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

void create_native_modules(d_vm* vm) {
  UNUSED(vm);  
  /**
   * OS module
  */
  drax_native_module* mos = new_native_module(vm, "os", 5);
  const drax_native_module_helper os_helper[] = {
    {1, "cmd", __d_cmd },
    {1, "cmd_with_status", __d_cmd_with_status },
    {1, "get_env", __d_get_env },
    {1, "mkdir", __d_mkdir1 },
    {2, "mkdir", __d_mkdir2 },
  };

  put_fun_on_module(mos, os_helper, sizeof(os_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(mos));

  /**
   * Core module
  */
  drax_native_module* mcore = new_native_module(vm, "core", 1);
  const drax_native_module_helper core_helper[] = {
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

}
