#include <string.h>

#include "dbuiltin.h"
#include "ddefs.h"
#include "dtypes.h"
#include "dtime.h"
#include "dvm.h"

#define MAKE_STRING_RETURN(g, c)  \
    return DS_VAL(copy_dstring(g, c, strlen(c)));

#define DX_SUCESS_FN(v) *v = 1;

#define DX_ERROR_FN(v) *v = 0;

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
    DX_ERROR_FN(stat);
    return DS_VAL(new_derror(vm, (char*) "assert error"));
  }

  return DS_VAL(new_derror(vm, CAST_STRING(b)->chars));
}

static drax_value __d_sleep(d_vm* vm, int* stat) { 
  UNUSED(vm);
  drax_value val = pop(vm);

  if (!(IS_NUMBER(val))) {
    DX_ERROR_FN(stat);
    return DS_VAL(new_derror(vm, (char *) "Expected number as argument"));
  }
  
  double t = CAST_NUMBER(val);
  dx_sleep(t);
  DX_SUCESS_FN(stat);
  return DRAX_NIL_VAL;
}

static drax_value __d_typeof(d_vm* vm, int* stat) {
  DX_SUCESS_FN(stat);
  drax_value val = pop(vm);
  if (IS_STRUCT(val)) {
    switch (DRAX_STYPEOF(val)) {
      case DS_NATIVE:
      case DS_FUNCTION: 
        MAKE_STRING_RETURN(vm, "function");

      case DS_STRING: 
        MAKE_STRING_RETURN(vm, "string");
      
      case DS_LIST:
        MAKE_STRING_RETURN(vm, "list");

      case DS_FRAME:
        MAKE_STRING_RETURN(vm, "frame");

      default: break;
    }
  }
  
  if (IS_BOOL(val))   { MAKE_STRING_RETURN(vm, "boolean"); }

  if (IS_NIL(val))    { MAKE_STRING_RETURN(vm, "nil"); }

  if (IS_NUMBER(val)) { MAKE_STRING_RETURN(vm, "number"); }

  MAKE_STRING_RETURN(vm, "none");
}

void load_callback_fn(d_vm* vm, vm_builtin_setter* reg) {
  reg(vm, "assert", 2, __d_assert);
  reg(vm, "typeof", 1, __d_typeof);
  reg(vm, "sleep", 1, __d_sleep);
}
