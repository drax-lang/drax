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
#include "dscheduler.h"

#include "mods/d_mod_os.h"
#include "mods/d_mod_http.h"
#include "mods/d_mod_scalar.h"

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
  char* r = replace_special_char(buff);
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
      case DS_SCALAR: MSR(vm, "scalar");
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
  double status = (double) d_command(CAST_STRING(a)->chars, buf, sizeof(buf));
  char* r = replace_special_char(buf);

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

  char *s = (char *) calloc(50, sizeof(char));
  double num = CAST_NUMBER(a);
  
  snprintf(s, sizeof(s), "%g", num);
  drax_string* str = new_dstring(vm, s, strlen(s));
  str->chars[strlen(s)] = '\0';

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
  n->length = o->length;

  memcpy(n->keys, o->keys, o->length * sizeof(int));
  memcpy(n->literals, o->literals, o->length * sizeof(char*));
  memcpy(n->values, o->values, o->length * sizeof(drax_value));

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
    nf1->length = f1->length;  
    memcpy(nf1->keys, f1->keys, f1->length * sizeof(int));
    memcpy(nf1->literals, f1->literals, f1->length * sizeof(char*));
    memcpy(nf1->values, f1->values, f1->length * sizeof(drax_value));
  }

  int i;
  for(i = 0; i < f2->length; i++) {
    if((nf1->length > i) && (strcmp(f2->literals[i], nf1->literals[i]) == 0)) {
      nf1->keys[i] = f2->keys[i];
      nf1->values[i] = f2->values[i];
    } else {
      char *s = (char *) malloc((strlen(f2->literals[i]) + 1) * sizeof(char));
      strcpy(s, f2->literals[i]);
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
  nf->length = f->length - 1;
  
  memcpy(nf->keys, f->keys, idx * sizeof(int));
  memcpy(nf->literals, f->literals, idx * sizeof(char*));
  memcpy(nf->values, f->values, idx * sizeof(drax_value));

  int bf = (nf->length - idx);
  memcpy(nf->keys + idx, f->keys + idx + 1, bf * sizeof(int));
  memcpy(nf->literals + idx, f->literals + idx + 1, bf * sizeof(char*));
  memcpy(nf->values + idx, f->values + idx + 1, bf * sizeof(drax_value));

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
    char *s = (char *) malloc((CAST_STRING(l1->elems[0])->length + 1) * sizeof(char));
    strcpy(s, CAST_STRING(l1->elems[0])->chars);
    put_value_dframe(f, s, l1->elems[1]);
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
 * List Module
 */

static drax_value __d_list_at(d_vm* vm, int* stat) {
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

static drax_value __d_list_is_empty(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  drax_list* l = CAST_LIST(a);

  DX_SUCESS_FN(stat);
  return l->length ? DRAX_FALSE_VAL : DRAX_TRUE_VAL;
}

static drax_value __d_list_is_present(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  drax_list* l = CAST_LIST(a);

  DX_SUCESS_FN(stat);
  return l->length ? DRAX_TRUE_VAL : DRAX_FALSE_VAL;
}

static drax_value __d_list_remove_at(d_vm* vm, int* stat) {
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

static drax_value __d_list_insert_at(d_vm* vm, int* stat) {
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

static drax_value __d_list_replace_at(d_vm* vm, int* stat) {
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

static drax_value __d_list_slice(d_vm* vm, int* stat) {
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

static drax_value __d_list_sum(d_vm* vm, int* stat) {
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

static drax_value __d_list_sparse(d_vm* vm, int* stat) {
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

static drax_value __d_list_hypot(d_vm* vm, int* stat) {
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

static drax_value __d_list_dot(d_vm* vm, int* stat) {
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

static drax_value __d_list_pop(d_vm* vm, int* stat) {
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

static drax_value __d_list_shift(d_vm* vm, int* stat) {
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

static drax_value __d_list_zip(d_vm* vm, int* stat) {
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

/**
 * TCPServer calls
 */

static drax_value __d_start_server(d_vm* vm, int* stat) {
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
   * get request_timeout_ms as string
   */
  char* host = (char*) "*";
  drax_value drt;
  if(get_value_dframe(ofr, (char*) "host", &drt) != -1) {
    return_if_is_not_string(drt, stat);
    host = CAST_STRING(drt)->chars;
  }

  char *options[] = {
    port_string,
    host,
    NULL
  };

  int fail;
  drax_value res;
  drax_value ctx = start_http_server(vm, options, &fail);

  if (!fail) {
    drax_tid* tid = new_dtid(vm, ctx);
    res = DS_VAL(tid);
    DX_SUCESS_FN(stat);
  } else {
    DX_ERROR_FN(stat);
    res = DS_VAL(ctx);
  }

  return res;
}

static drax_value __d_stop_server(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  return_if_is_not_tid(v, stat);
  drax_tid* tid = CAST_TID(v);
  int status = stop_http_server(vm, tid->value);
  
  tid->value = 0;

  DX_SUCESS_FN(stat);
  return status ? DRAX_TRUE_VAL : DRAX_FALSE_VAL;
}

static drax_value __d_accept_server(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  return_if_is_not_tid(v, stat);
  drax_tid* tid = CAST_TID(v);
  drax_value _err;
  drax_value addrs = tid->value;
  int success = accept_http_server(vm, addrs, &_err);

  if (!success) {
    DX_ERROR_FN(stat);
    return _err;
  }

  DX_SUCESS_FN(stat);
  return DS_VAL(tid);
}

static drax_value __d_receive_server(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  return_if_is_not_tid(v, stat);
  drax_tid* tid = CAST_TID(v);
  char* res = receive_http_server(vm, tid->value);
  drax_string* s = new_dstring(vm, res, strlen(res));

  DX_SUCESS_FN(stat);
  return DS_VAL(s);
}

static drax_value __d_send_server(d_vm* vm, int* stat) {
  drax_value data = pop(vm);
  drax_value v = pop(vm);

  return_if_is_not_tid(v, stat);
  return_if_is_not_string(data, stat);

  drax_tid* tid = CAST_TID(v);
  send_http_server(vm, tid->value, CAST_STRING(data)->chars);

  DX_SUCESS_FN(stat);
  return DRAX_TRUE_VAL;
}

static drax_value __d_disconnect_server(d_vm* vm, int* stat) {
  drax_value v = pop(vm);
  return_if_is_not_tid(v, stat);
  drax_tid* tid = CAST_TID(v);

  disconnect_client_http_server(vm, tid->value);

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
  drax_native_module* string = new_native_module(vm, "String", 8);
  const drax_native_module_helper string_helper[] = {
    {2, "split", dstr_split },
    {1, "length", dstr_length },
    {2, "copy", dstr_copy },
    {3, "copy", dstr_copy2 },
    {2, "at", dstr_at },
    {1, "to_uppercase", dstr_to_uppercase },
    {1, "to_lowercase", dstr_to_lowercase },
    {1, "to_number", dstr_to_number },
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
  drax_native_module* list = new_native_module(vm, "List", 18);
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
  };
  
  put_fun_on_module(list, list_helper, sizeof(list_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(list));

  /**
   * Scalar Module
   */ 
  drax_native_module* scalar = new_native_module(vm, "Scalar", 13);
  const drax_native_module_helper scalar_helper[] = {
    {2, "concat", __d_scalar_concat },
    {1, "head", __d_scalar_head},
    {1, "tail", __d_scalar_tail},
    {1, "length", __d_scalar_length},
    {1, "is_empty", __d_scalar_is_empty},
    {1, "is_present", __d_scalar_is_present},
    {2, "remove_at", __d_scalar_remove_at},
    {3, "insert_at", __d_scalar_insert_at},
    {3, "replace_at", __d_scalar_replace_at},
    {3, "slice", __d_scalar_slice},
    {1, "sum", __d_scalar_sum},
    {2, "at", __d_scalar_at},
    {1, "sparse", __d_scalar_sparse}
  };
  
  put_fun_on_module(scalar, scalar_helper, sizeof(scalar_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(scalar));

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
