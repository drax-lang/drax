#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>

#include "dbuiltin.h"
#include "ddefs.h"
#include "dstring.h"
#include "dtypes.h"
#include "dtime.h"
#include "dvm.h"
#include "deval.h"
#include "dgc.h"

#include "mods/d_mod_os.h"
#include "mods/d_mod_http.h"
#include "mods/d_mod_list.h"

static drax_value __d_assert(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  /* only nil and false are falsy */
  if (!IS_NIL(a) && (!IS_BOOL(a) || CAST_BOOL(a))) {
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
  char* r = replace_special_char(buff);
  free(buff);
  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, r, strlen(r)));
}

static drax_value __d_print(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  /* a string is written as it is; inspect, the shell, lists and frames show it quoted */
  print_drax(v, IS_STRING(v) ? 1 : 0);
  dbreak_line();
  DX_SUCESS_FN(stat);
  return DRAX_NIL_VAL;
}

static drax_value __d_inspect(d_vm* vm, int* stat) {
  drax_value val = pop(vm);
  print_drax(val, 0);
  dbreak_line();
  DX_SUCESS_FN(stat);
  return val;
}

static drax_value __d_help(d_vm* vm, int* stat) {
  drax_value val = pop(vm);
  return_if_is_not_module(val, stat);
  print_funcs_on_module(CAST_MODULE(val));
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
      case DS_TID: MSR(vm, "tid");
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

static drax_value __d_hypot(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);
  return_if_is_not_number(b, stat);

  double n1 = CAST_NUMBER(a);
  double n2 = CAST_NUMBER(b);

  DX_SUCESS_FN(stat);
  return num_to_draxvalue(sqrt(pow(n1, 2) + pow(n2, 2)));
}

static drax_value __d_number_is_even(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  int n1 = (int) CAST_NUMBER(a);

  DX_SUCESS_FN(stat);
  return (n1 & 1) == 0 ? DRAX_TRUE_VAL : DRAX_FALSE_VAL;
}

static drax_value __d_number_is_odd(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);

  int n1 = (int) CAST_NUMBER(a);

  DX_SUCESS_FN(stat);
  return (n1 & 1) == 1 ? DRAX_TRUE_VAL : DRAX_FALSE_VAL;
}

static drax_value __d_number_floor_div(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_number(a, stat);
  return_if_is_not_number(b, stat);

  double n1 = CAST_NUMBER(a);
  double n2 = CAST_NUMBER(b);

  DX_SUCESS_FN(stat);
  return num_to_draxvalue(floor(n1 / n2));
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
  buf[0] = '\0'; /* d_command does not write on failure */
  double status = (double) d_command(CAST_STRING(a)->chars, buf, sizeof(buf));

  if (status != 0) {
    DX_ERROR_FN(stat);
    return DS_VAL(new_derror(vm, (char *) "Fail to execute command"));
  }

  char* r = replace_special_char(buf);
  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, r, strlen(r)));
}

static drax_value __d_cmd_with_status(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_string(a, stat);

  char buf[4096];
  buf[0] = '\0'; /* d_command does not write on failure */
  double status = (double) d_command(CAST_STRING(a)->chars, buf, sizeof(buf));
  char* r = replace_special_char(buf);

  drax_list* l = new_dlist(vm, 2);
  put_value_dlist(l, AS_VALUE(status));
  put_value_dlist(l, DS_VAL(new_dstring(vm, r, strlen(r))));
  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

static int create_directory(const char* path, mode_t mode) {
#ifdef _WIN32
    (void) mode;
    return CreateDirectory(path, NULL) ? 0 : -1;
#else
    return mkdir(path, mode);
#endif
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

  int r = create_directory(CAST_STRING(a)->chars, mode);

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
  reg(vm, "inspect", 1, __d_inspect);
  reg(vm, "help", 1, __d_help);
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

static drax_value __gc_meta_info(d_vm* vm, int* stat) {
  drax_frame* nf = new_dframe(vm, 2);
  put_value_dframe(nf, (char*) "num_cycles", NUMBER_VAL(vm->gc_meta->n_cycles));
  put_value_dframe(nf, (char*) "num_free_structs", NUMBER_VAL(vm->gc_meta->n_free_structs));
  DX_SUCESS_FN(stat);
  return DS_VAL(nf);
}

/**
  * Number module
*/

static drax_value __d_number_to_string(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_number(a, stat);

  char *s = (char *) calloc(32, sizeof(char));
  int len = dnum_to_str(CAST_NUMBER(a), s, 32);
  drax_string* str = new_dstring(vm, s, len);

  DX_SUCESS_FN(stat);
  return DS_VAL(str);
}

static drax_value __d_number_rand(d_vm* vm, int* stat) {
  UNUSED(vm);
  DX_SUCESS_FN(stat);
  return num_to_draxvalue((double) rand());
}

static drax_value __d_number_max(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);
  return_if_is_not_number(a, stat);
  return_if_is_not_number(b, stat);

  double n1 = CAST_NUMBER(a);
  double n2 = CAST_NUMBER(b);

  DX_SUCESS_FN(stat);

  /* IEEE 754 maximum: nan propagates and +0 is bigger than -0 */
  if (n1 != n1 || n2 != n2) return num_to_draxvalue(NAN);
  if (n1 == 0 && n2 == 0) return num_to_draxvalue(signbit(n1) ? n2 : n1);
  return num_to_draxvalue(n1 > n2 ? n1 : n2);
}

static drax_value __d_number_min(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);
  return_if_is_not_number(a, stat);
  return_if_is_not_number(b, stat);

  double n1 = CAST_NUMBER(a);
  double n2 = CAST_NUMBER(b);

  DX_SUCESS_FN(stat);

  /* IEEE 754 minimum: nan propagates and -0 is smaller than +0 */
  if (n1 != n1 || n2 != n2) return num_to_draxvalue(NAN);
  if (n1 == 0 && n2 == 0) return num_to_draxvalue(signbit(n1) ? n1 : n2);
  return num_to_draxvalue(n1 < n2 ? n1 : n2);
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
  copy_dframe_entries(n, 0, o, 0, o->length);
  n->length = o->length;

  put_value_dframe(n, CAST_STRING(b)->chars, c);

  DX_SUCESS_FN(stat);
  return DS_VAL(n);
}

static drax_value __d_frame_merge(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  return_if_is_not_frame(b, stat);
  
  drax_frame* f1 = CAST_FRAME(a);
  drax_frame* f2 = CAST_FRAME(b);

  drax_frame* nf1 = new_dframe(vm, f1->length);
  
  if (f1->length > 0) {
    copy_dframe_entries(nf1, 0, f1, 0, f1->length);
    nf1->length = f1->length;  
  }

  int i;
  for(i = 0; i < f2->length; i++) {
    if((nf1->length > i) && (strcmp(f2->literals[i], nf1->literals[i]) == 0)) {
      nf1->keys[i] = f2->keys[i];
      nf1->values[i] = f2->values[i];
    } else {
      put_value_dframe(nf1, f2->literals[i], f2->values[i]);
    }
  }

  DX_SUCESS_FN(stat);
  return DS_VAL(nf1);
}

static drax_value __d_frame_to_list(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  
  drax_frame* f = CAST_FRAME(a);
  drax_list* l = new_dlist(vm, f->length);

  int i;
  for(i = 0; i < f->length; i++) {
    if(f->literals[i]) {
      drax_list* fl = new_dlist(vm, 2);
      int sz = strlen(f->literals[i]) + 1;
      char *s = (char *) malloc(sz * sizeof(char));
      strcpy(s, f->literals[i]);
      put_value_dlist(fl, DS_VAL(new_dstring(vm, s, strlen(f->literals[i]))));
      put_value_dlist(fl, f->values[i]);
      put_value_dlist(l, DS_VAL(fl));
    }
  }

  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

static drax_value __d_frame_keys(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  
  drax_frame* f = CAST_FRAME(a);
  drax_list* l = new_dlist(vm, f->length);

  int i;
  for(i = 0; i < f->length; i++) {
    if(f->literals[i]) {
      int sz = strlen(f->literals[i]) + 1;
      char *s = (char *) malloc(sz * sizeof(char));
      strcpy(s, f->literals[i]);
      put_value_dlist(l, DS_VAL(new_dstring(vm, s, strlen(f->literals[i]))));
    }
  }

  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

static drax_value __d_frame_values(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  
  drax_frame* f = CAST_FRAME(a);
  drax_list* l = new_dlist(vm, f->length);

  int i;
  for(i = 0; i < f->length; i++) {
    if(f->literals[i]) {
      put_value_dlist(l, f->values[i]);
    }
  }

  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

static drax_value __d_frame_has_key(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  return_if_is_not_string(b, stat);
  
  drax_frame* f = CAST_FRAME(a);
  drax_string* key = CAST_STRING(b);
  
  drax_value v;
  int idx = get_value_dframe(f, key->chars, &v);

  DX_SUCESS_FN(stat);
  return idx != -1 ? DRAX_TRUE_VAL : DRAX_FALSE_VAL;
}

static drax_value __d_frame_remove(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  return_if_is_not_string(b, stat);
  
  drax_frame* f = CAST_FRAME(a);
  drax_string* key = CAST_STRING(b);

  drax_value v;
  int idx = get_value_dframe(f, key->chars, &v);
  if(idx < 0) {
    DX_SUCESS_FN(stat);
    return a;
  }

  drax_frame* nf = new_dframe(vm, f->length - 1);
  int bf = (f->length - 1 - idx);

  copy_dframe_entries(nf, 0, f, 0, idx);
  copy_dframe_entries(nf, idx, f, idx + 1, bf);
  nf->length = f->length - 1;

  DX_SUCESS_FN(stat);
  return DS_VAL(nf);
}

static drax_value __d_frame_get(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  return_if_is_not_string(b, stat);
  
  drax_frame* f = CAST_FRAME(a);
  drax_string* key = CAST_STRING(b);
  
  drax_value v;
  int idx = get_value_dframe(f, key->chars, &v);

  DX_SUCESS_FN(stat);
  return idx != -1 ? f->values[idx] : DRAX_NIL_VAL;
}

static drax_value __d_frame_get_or_else(d_vm* vm, int* stat) {
  drax_value c = pop(vm);
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  return_if_is_not_string(b, stat);
  
  drax_frame* f = CAST_FRAME(a);
  drax_string* key = CAST_STRING(b);
  
  drax_value v;
  int idx = get_value_dframe(f, key->chars, &v);

  DX_SUCESS_FN(stat);
  return idx != -1 ? f->values[idx] : c;
}

static drax_value __d_frame_is_empty(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  
  drax_frame* f = CAST_FRAME(a);

  DX_SUCESS_FN(stat);
  return f->length ? DRAX_FALSE_VAL : DRAX_TRUE_VAL;
}

static drax_value __d_frame_is_present(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  
  drax_frame* f = CAST_FRAME(a);

  DX_SUCESS_FN(stat);
  return f->length ? DRAX_TRUE_VAL : DRAX_FALSE_VAL;
}

static drax_value __d_frame_new(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  drax_list* l = CAST_LIST(a);
  drax_frame* f = new_dframe(vm, l->length);

  int i;
  for(i = 0; i < l->length; i++) {
    return_if_is_not_list(l->elems[i], stat);

    drax_list* l1 = CAST_LIST(l->elems[i]);

    return_if_is_not_string(l1->elems[0], stat);
    /* the frame keeps its own copy of the key */
    put_value_dframe(f, CAST_STRING(l1->elems[0])->chars, l1->elems[1]);
  }
  
  DX_SUCESS_FN(stat);
  return DS_VAL(f);
}

static drax_value __d_frame_new_empty(d_vm* vm, int* stat) {
  drax_frame* f = new_dframe(vm, 0);
  
  DX_SUCESS_FN(stat);
  return DS_VAL(f);
}

/**
 * TCPServer and Http calls
 *
 * Everything is blocking and sequential, in the order it is written:
 *
 *   server = TCPServer.new({ port: '8080' })
 *   conn = TCPServer.accept(server)
 *   req = Http.read_request(conn)
 *   TCPServer.send(conn, Http.response(200, {}, 'ok'))
 *   TCPServer.disconnect(conn)
 *
 * The server and each connection are different values (see d_mod_http.h).
 */

static drax_value http_type_error(d_vm* vm, int* stat, const char* msg) {
  DX_ERROR_FN(stat);
  return DS_VAL(new_derror(vm, (char*) msg));
}

#define IS_HTTP_SERVER(v) \
  (IS_TID(v) && CAST_TID(v)->destroy == dhttp_destroy && dhttp_is_listener(CAST_TID(v)->value))

#define IS_HTTP_CONN(v) \
  (IS_TID(v) && CAST_TID(v)->destroy == dhttp_destroy && dhttp_is_conn(CAST_TID(v)->value))

/* an option as text: a string or an integer. 0: absent, 1: ok, -1: wrong type */
static int http_opt_text(drax_frame* f, const char* key, char* out, size_t sz) {
  drax_value v;
  if (get_value_dframe(f, (char*) key, &v) == -1) return 0;
  if (IS_STRING(v)) { snprintf(out, sz, "%s", CAST_STRING(v)->chars); return 1; }
  if (IS_NUMBER(v)) { snprintf(out, sz, "%.0f", CAST_NUMBER(v)); return 1; }
  return -1;
}

/* a non negative integer option. 0: absent, 1: ok, -1: invalid */
static int http_opt_size(drax_frame* f, const char* key, long* out) {
  char buf[48];
  int r = http_opt_text(f, key, buf, sizeof(buf));
  if (r != 1) return r;
  char* end;
  long v = strtol(buf, &end, 10);
  if (end == buf || *end != '\0' || v < 0) return -1;
  *out = v;
  return 1;
}

static int http_opt_bool(drax_frame* f, const char* key, int* out) {
  drax_value v;
  if (get_value_dframe(f, (char*) key, &v) == -1) return 0;
  if (IS_BOOL(v)) { *out = v == DRAX_TRUE_VAL; return 1; }
  if (IS_STRING(v)) {
    if (strcmp(CAST_STRING(v)->chars, "true") == 0) { *out = 1; return 1; }
    if (strcmp(CAST_STRING(v)->chars, "false") == 0) { *out = 0; return 1; }
  }
  return -1;
}

/**
 * TCPServer.new({ host, port, backlog, timeout_ms, reuse_port,
 *                 max_header_bytes, max_body_bytes })
 */
static drax_value __d_start_server(d_vm* vm, int* stat) {
  drax_value opt = pop(vm);
  return_if_is_not_frame(opt, stat);
  drax_frame* ofr = CAST_FRAME(opt);

  char host[256] = "";
  char port[16] = "5000";
  long backlog = 0, timeout = DHTTP_DEFAULT_TIMEOUT_MS;
  long max_header = DHTTP_DEFAULT_MAX_HEADER, max_body = DHTTP_DEFAULT_MAX_BODY;
  int reuse_port = 0;
  char msg[96];

  #define BAD_OPTION(k) \
    do { \
      snprintf(msg, sizeof(msg), "server error, invalid option '%s'!", k); \
      DX_ERROR_FN(stat); \
      return DS_VAL(new_derror(vm, msg)); \
    } while (0)

  if (http_opt_text(ofr, "host", host, sizeof(host)) < 0) BAD_OPTION("host");
  if (http_opt_text(ofr, "port", port, sizeof(port)) < 0) BAD_OPTION("port");
  if (http_opt_size(ofr, "backlog", &backlog) < 0) BAD_OPTION("backlog");
  if (http_opt_size(ofr, "timeout_ms", &timeout) < 0) BAD_OPTION("timeout_ms");
  if (http_opt_size(ofr, "max_header_bytes", &max_header) < 0 || max_header < 64) BAD_OPTION("max_header_bytes");
  if (http_opt_size(ofr, "max_body_bytes", &max_body) < 0) BAD_OPTION("max_body_bytes");
  if (http_opt_bool(ofr, "reuse_port", &reuse_port) < 0) BAD_OPTION("reuse_port");

  #undef BAD_OPTION

  dhttp_opts o;
  o.host = host;
  o.port = port;
  o.backlog = (int) backlog;
  o.reuse_port = reuse_port;
  o.timeout_ms = (int) timeout;
  o.max_header = (size_t) max_header;
  o.max_body = (size_t) max_body;

  int fail;
  drax_value ctx = dhttp_listen(vm, &o, &fail);

  if (fail) {
    DX_ERROR_FN(stat);
    return ctx;
  }

  drax_tid* tid = new_dtid(vm, ctx);
  tid->destroy = dhttp_destroy;
  DX_SUCESS_FN(stat);
  return DS_VAL(tid);
}

static drax_value __d_stop_server(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  if (!IS_HTTP_SERVER(v)) return http_type_error(vm, stat, "Expected a server as argument");

  int status = dhttp_stop(CAST_TID(v)->value);
  DX_SUCESS_FN(stat);
  return status ? DRAX_TRUE_VAL : DRAX_FALSE_VAL;
}

/* waits for a client and returns its connection, a value of its own */
static drax_value __d_accept_server(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  if (!IS_HTTP_SERVER(v)) return http_type_error(vm, stat, "Expected a server as argument");

  drax_value conn;
  if (!dhttp_accept(vm, CAST_TID(v)->value, &conn)) {
    DX_ERROR_FN(stat);
    return conn;
  }

  drax_tid* tid = new_dtid(vm, conn);
  tid->destroy = dhttp_destroy;
  DX_SUCESS_FN(stat);
  return DS_VAL(tid);
}

/* the bytes that arrived (a single read), or nil when the client is gone */
static drax_value __d_receive_server(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  if (!IS_HTTP_CONN(v)) return http_type_error(vm, stat, "Expected a connection as argument");

  drax_value res = dhttp_recv(vm, CAST_TID(v)->value);
  DX_SUCESS_FN(stat);
  return res;
}

/* writes the whole string (any bytes). false: the client is gone or too slow */
static drax_value __d_send_server(d_vm* vm, int* stat) {
  drax_value data = pop(vm);
  drax_value v = pop(vm);

  if (!IS_HTTP_CONN(v)) return http_type_error(vm, stat, "Expected a connection as argument");
  return_if_is_not_string(data, stat);

  drax_string* s = CAST_STRING(data);
  int ok = dhttp_send(CAST_TID(v)->value, s->chars, (size_t) s->length);

  DX_SUCESS_FN(stat);
  return ok ? DRAX_TRUE_VAL : DRAX_FALSE_VAL;
}

static drax_value __d_disconnect_server(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  if (!IS_HTTP_CONN(v)) return http_type_error(vm, stat, "Expected a connection as argument");

  dhttp_disconnect(CAST_TID(v)->value);
  DX_SUCESS_FN(stat);
  return DRAX_TRUE_VAL;
}

/**
 * Http.read_request(conn): a whole request as a frame, or nil when the
 * client closed the connection (or the timeout ended).
 */
static drax_value __d_http_read_request(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  if (!IS_HTTP_CONN(v)) return http_type_error(vm, stat, "Expected a connection as argument");

  drax_value res = dhttp_read_request(vm, CAST_TID(v)->value);
  DX_SUCESS_FN(stat);
  return res;
}

/* Http.response(status, headers, body): the text to send */
static drax_value __d_http_response(d_vm* vm, int* stat) {
  drax_value body = pop(vm);
  drax_value headers = pop(vm);
  drax_value status = pop(vm);

  int fail;
  drax_value res = dhttp_response(vm, status, headers, body, &fail);
  if (fail) {
    DX_ERROR_FN(stat);
    return res;
  }

  DX_SUCESS_FN(stat);
  return res;
}

/**
 * Entry point for native modules
 */

void create_native_modules(d_vm* vm) {
  /**
   * OS module
  */
  drax_native_module* mos = new_native_module(vm, "Os", 6);
  const drax_native_module_helper os_helper[] = {
    {1, "cmd", __d_cmd },
    {1, "cmd_with_status", __d_cmd_with_status },
    {1, "system", __d_system},
    {1, "get_env", __d_get_env },
    {1, "mkdir", __d_mkdir1 },
    {2, "pmkdir", __d_mkdir2 },
  };

  put_fun_on_module(mos, os_helper, sizeof(os_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(mos));

  /**
   * Core module
  */
  drax_native_module* mcore = new_native_module(vm, "Core", 3);
  const drax_native_module_helper core_helper[] = {
    /**
     * Garbage collector
     */
    {0, "gc_swap", __d_gc_swap },
    {0, "gc_meta_info", __gc_meta_info },
    {1, "exit", __d_exit },
  };

  put_fun_on_module(mcore, core_helper, sizeof(core_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(mcore));

  /**
   * Number module
   */
  drax_native_module* number = new_native_module(vm, "Number", 7);
  const drax_native_module_helper number_helper[] = {
    {1, "to_string", __d_number_to_string },
    {0, "rand", __d_number_rand },
    {2, "max", __d_number_max },
    {2, "min", __d_number_min },
    {1, "is_even", __d_number_is_even },
    {1, "is_odd", __d_number_is_odd },
    {2, "floor_div", __d_number_floor_div },
  };

  put_fun_on_module(number, number_helper, sizeof(number_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(number));

  /**
   * String module
   */
  drax_native_module* string = new_native_module(vm, "String", 11);
  const drax_native_module_helper string_helper[] = {
    {2, "split", dstr_split },
    {1, "length", dstr_length },
    {2, "copy", dstr_copy },
    {3, "copy", dstr_copy2 },
    {2, "at", dstr_at },
    {1, "to_uppercase", dstr_to_uppercase },
    {1, "to_lowercase", dstr_to_lowercase },
    {1, "to_number", dstr_to_number },
    {2, "contains", dstr_contains },
  };

  put_fun_on_module(string, string_helper, sizeof(string_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(string));

  /**
   * Frame Module
   */ 
  drax_native_module* frame = new_native_module(vm, "Frame", 13);
  const drax_native_module_helper frame_helper[] = {
    {3, "put", __d_frame_put },
    {2, "merge", __d_frame_merge },
    {1, "to_list", __d_frame_to_list },
    {1, "keys", __d_frame_keys },
    {1, "values", __d_frame_values },
    {2, "has_key", __d_frame_has_key },
    {2, "remove", __d_frame_remove },
    {2, "get", __d_frame_get },
    {3, "get_or_else", __d_frame_get_or_else },
    {1, "is_empty", __d_frame_is_empty },
    {1, "is_present", __d_frame_is_present },
    {0, "new", __d_frame_new_empty },
    {1, "new_from_list", __d_frame_new },
  };

  put_fun_on_module(frame, frame_helper, sizeof(frame_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(frame));

  /**
   * List Module
   */ 
  drax_native_module* list = new_native_module(vm, "List", 21);
  const drax_native_module_helper list_helper[] = {
    {2, "concat", __d_list_concat },
    {1, "head", __d_list_head},
    {1, "tail", __d_list_tail},
    {1, "length", __d_list_length},
    {1, "is_empty", __d_list_is_empty},
    {1, "is_present", __d_list_is_present},
    {2, "remove_at", __d_list_remove_at},
    {3, "insert_at", __d_list_insert_at},
    {3, "replace_at", __d_list_replace_at},
    {3, "slice", __d_list_slice},
    {1, "sum", __d_list_sum},
    {2, "at", __d_list_at},
    {1, "sparse", __d_list_sparse},
    {1, "hypot", __d_list_hypot},
    {2, "dot", __d_list_dot},
    {2, "zip", __d_list_zip},
    {1, "pop", __d_list_pop},
    {1, "shift", __d_list_shift},
    {2, "map", __d_list_map},
    {2, "filter", __d_list_filter},
    {2, "tap", __d_list_tap}
  };
  
  put_fun_on_module(list, list_helper, sizeof(list_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(list));

  /**
   * Socket Module
   */ 
  drax_native_module* http = new_native_module(vm, "TCPServer", 6);
  const drax_native_module_helper http_helper[] = {
    {1, "new",   __d_start_server },
    {1, "close", __d_stop_server},
    {1, "accept", __d_accept_server},
    {1, "receive", __d_receive_server},
    {2, "send", __d_send_server},
    {1, "disconnect", __d_disconnect_server},
  };
  
  put_fun_on_module(http, http_helper, sizeof(http_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(http));

  /**
   * Http Module: requests and responses on top of a connection
   */
  drax_native_module* httpm = new_native_module(vm, "Http", 2);
  const drax_native_module_helper httpm_helper[] = {
    {1, "read_request", __d_http_read_request},
    {3, "response", __d_http_response},
  };

  put_fun_on_module(httpm, httpm_helper, sizeof(httpm_helper) / sizeof(drax_native_module_helper));
  put_mod_table(vm->envs->modules, DS_VAL(httpm));

  drax_native_module* math = new_native_module(vm, "Math", 24);
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
      {2, "hypot", __d_hypot}
  };

  put_fun_on_module(math, math_helper, sizeof(math_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(math));
}
