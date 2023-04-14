#ifndef __DSTRUCTS
#define __DSTRUCTS

#include "dhandler.h"
#include "dtypes.h"
#include "dvm.h"

typedef struct d_vm d_vm;

#define DRAX_STYPEOF(v)   (CAST_STRUCT(v)->type)

#define IS_ST_TYPE(v, t)  (IS_STRUCT(v) && DRAX_STYPEOF(v) == t)

#define IS_ERROR(v)       IS_ST_TYPE(v, DS_ERROR)
#define IS_LIST(v)        IS_ST_TYPE(v, DS_LIST)
#define IS_BLOCK(v)       IS_ST_TYPE(v, DS_BLOCK)
#define IS_FUNCTION(v)    IS_ST_TYPE(v, DS_FUNCTION)
#define IS_NATIVE(v)      IS_ST_TYPE(v, DS_NATIVE)
#define IS_STRING(v)      IS_ST_TYPE(v, DS_STRING)

#define CAST_ERROR(v)     ((drax_error*) CAST_STRUCT(v))
#define CAST_BLOCK(v)     ((drax_block*) CAST_STRUCT(v))
#define CAST_FUNCTION(v)  ((drax_function*) CAST_STRUCT(v))
#define CAST_NATIVE(v)    (((drax_os_native*) CAST_STRUCT(v))->function)
#define CAST_STRING(v)    ((drax_string*) CAST_STRUCT(v))
#define CAST_LIST(v)      ((drax_list*) CAST_STRUCT(v))

typedef drax_value (low_level_callback) (d_vm* g, int argc, drax_value* argv, bool* stat);

typedef struct drax_os_native {
  d_struct d_struct;
  low_level_callback* function;
} drax_os_native;

typedef struct drax_list {
  d_struct d_struct;
  int length;
  int cap;
  drax_value* elems;
} drax_list;

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

drax_error* new_derror(d_vm* vm, char* msg);

drax_list* new_dlist(d_vm* vm, int cap);

void put_value_dlist(drax_list* l, drax_value v);

drax_os_native* new_dllcallback(d_vm* vm, low_level_callback* function);

drax_string* new_dstring(d_vm* vm, char* chars, int length);

drax_string* copy_dstring(d_vm* vm, const char* chars, int length);

#endif
