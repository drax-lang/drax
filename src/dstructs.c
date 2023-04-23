#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dstructs.h"
#include "dhandler.h"
#include "dtypes.h"
#include "ddefs.h"
#include "dvm.h"

#define ALLOCATE_DSTRUCT(b, type, d_structType) \
    (type*) allocate_struct(b, sizeof(type), d_structType)

static d_struct* allocate_struct(d_vm* vm, size_t size, dstruct_type type) {
  UNUSED(vm);
  d_struct* d = (d_struct*) malloc(sizeof(d_struct) * size);
  d->type = type;
  d->checked = false;

  return d;
}

/* Drax list helpers
 */

#define LIST_PRE_SIZE 10

drax_list* new_dlist(d_vm* vm, int cap) {
  drax_list* l = ALLOCATE_DSTRUCT(vm, drax_list, DS_LIST);
  l->length = 0;
  l->cap = cap == 0 ? cap : LIST_PRE_SIZE;
  l->elems = malloc(sizeof(drax_value) * l->cap);
  return l;
}

void put_value_dlist(drax_list* l, drax_value v) {
  if (l->cap <= l->length) {
    l->cap = (l->cap + LIST_PRE_SIZE);
    l->elems = realloc(l->elems, sizeof(drax_value) * l->cap);
  }

  l->elems[l->length] = v;
  l->length++;
}

drax_error* new_derror(d_vm* vm, char* msg) {
  drax_error* v = ALLOCATE_DSTRUCT(vm, drax_error, DS_FUNCTION);
  v->chars = msg;
  v->length = strlen(msg);
  return v;
}

drax_os_native* new_dllcallback(d_vm* vm, low_level_callback* function) {
  drax_os_native* native = ALLOCATE_DSTRUCT(vm, drax_os_native, DS_NATIVE);
  native->function = function;
  return native;
}

drax_function* new_function(d_vm* vm) {
  drax_function* f = ALLOCATE_DSTRUCT(vm, drax_function, DS_FUNCTION);
  f->arity = 0;
  f->instructions = new_instructions();

  return f;
}

static drax_string* allocate_string(d_vm* vm, char* chars, int length, uint32_t hash) {
  drax_string* string = ALLOCATE_DSTRUCT(vm, drax_string, DS_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  return string;
}

drax_string* new_dstring(d_vm* vm, char* chars, int length) {
  uint32_t hash = fnv1a_hasg(chars, length);

  return allocate_string(vm, chars, length, hash);
}

drax_string* copy_dstring(d_vm* vm, const char* chars, int length) {
  uint32_t hash = fnv1a_hasg(chars, length);

  char* heap_chars = malloc(sizeof(char) * length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';

  return allocate_string(vm, heap_chars, length, hash);
}

