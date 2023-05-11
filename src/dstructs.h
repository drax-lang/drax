#ifndef __DSTRUCTS
#define __DSTRUCTS

#include "dhandler.h"
#include "dtypes.h"
#include "dvm.h"

typedef struct d_vm d_vm;

#define DRAX_STYPEOF(v)   (CAST_STRUCT(v)->type)

#define IS_ST_TYPE(v, t)  (IS_STRUCT(v) && DRAX_STYPEOF(v) == t)

#define IS_ERROR(v)       IS_ST_TYPE(v, DS_ERROR)
#define IS_FRAME(v)       IS_ST_TYPE(v, DS_FRAME)
#define IS_LIST(v)        IS_ST_TYPE(v, DS_LIST)
#define IS_BLOCK(v)       IS_ST_TYPE(v, DS_BLOCK)
#define IS_FUNCTION(v)    IS_ST_TYPE(v, DS_FUNCTION)
#define IS_NATIVE(v)      IS_ST_TYPE(v, DS_NATIVE)
#define IS_MODULE(v)      IS_ST_TYPE(v, DS_MODULE)
#define IS_STRING(v)      IS_ST_TYPE(v, DS_STRING)

#define CAST_ERROR(v)     ((drax_error*) CAST_STRUCT(v))
#define CAST_FUNCTION(v)  ((drax_function*) CAST_STRUCT(v))
#define CAST_NATIVE(v)    (((drax_os_native*) CAST_STRUCT(v)))
#define CAST_MODULE(v)    ((drax_native_module*) CAST_STRUCT(v))
#define CAST_STRING(v)    ((drax_string*) CAST_STRUCT(v))
#define CAST_LIST(v)      ((drax_list*) CAST_STRUCT(v))
#define CAST_FRAME(v)      ((drax_frame*) CAST_STRUCT(v))

typedef drax_value (low_level_callback) (d_vm* g, int* stat);

/**
 * Modules definitions
 * 
 * Internals/native modules is not a frame.
 */

typedef struct drax_native_module_helper {
  int arity;
  const char* name;
  low_level_callback* fun;
} drax_native_module_helper;

typedef struct drax_native_module {
  d_struct d_struct;
  int count;
  const char* name;
  const char** fn_names;
  int* arity;
  low_level_callback** fun;
} drax_native_module;

typedef struct drax_os_native {
  d_struct d_struct;
  const char* name;
  int arity;
  low_level_callback* function;
} drax_os_native;

typedef struct drax_list {
  d_struct d_struct;
  int length;
  int cap;
  drax_value* elems;
} drax_list;

typedef struct drax_frame {
  d_struct d_struct;
  int length;
  int cap;
  int* keys;
  char** literals;
  drax_value* values;
} drax_frame;

typedef struct drax_error {
  d_struct d_struct;
  int length;
  char* chars;
} drax_error;

typedef struct drax_string {
  d_struct d_struct;
  int length;
  char* chars;
  uint32_t hash;
} drax_string;

typedef struct drax_function {
  d_struct d_struct;
  int arity;
  d_instructions* instructions;
  char* name;
} drax_function;

void put_value_dlist(drax_list* l, drax_value v);

drax_error* new_derror(d_vm* vm, char* msg);

drax_list* new_dlist(d_vm* vm, int cap);

drax_function* new_function(d_vm* vm);

drax_os_native* new_dllcallback(d_vm* vm, low_level_callback* f, const char* name, int arity);

drax_string* new_dstring(d_vm* vm, char* chars, int length);

drax_string* copy_dstring(d_vm* vm, const char* chars, int length);

drax_frame* new_dframe(d_vm* vm, int cap);

int get_value_dframe(drax_frame* l, char* name, drax_value* value);

void put_value_dframe(drax_frame* l, char* k, drax_value v);

drax_native_module* new_native_module(d_vm* vm, const char* name, int cap);

void put_fun_on_module(drax_native_module* m, const drax_native_module_helper helper[], int size);

low_level_callback* get_fun_on_module(drax_native_module* m, const char* n, int a);

#endif
