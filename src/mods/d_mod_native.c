/* drax Lang
 *
 * Native extensions, see d_mod_native.h and drax_ext.h (the API of an extension).
 *
 * How it works:
 *   1. Native.load(path) resolves the path and checks the permission (--safe).
 *   2. The library is opened and must export drax_ext_abi (the version of the
 *      API it was built for) and drax_ext_init.
 *   3. drax_ext_init declares the module and its functions in a registry. The
 *      functions are not run yet.
 *   4. A module is created with those functions. Every one of them is the
 *      same native function (ext_trampoline) with its own data
 *      (vm->native_udata): it reads the arguments from the stack, calls the
 *      code of the extension with the opaque API and returns its value.
 *
 * The interpreter never gives its structures to an extension: it only sees
 * opaque values and the functions of drax_api, which check the types.
 */

/* realpath, strdup, dlfcn: the build uses -std=c99 */
#ifndef _WIN32
  #ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
  #endif
  #ifndef _DEFAULT_SOURCE
    #define _DEFAULT_SOURCE
  #endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
  #include <unistd.h>
#endif

#include "d_mod_native.h"
#include "../drax_ext.h"
#include "../ddefs.h"
#include "../dstructs.h"
#include "../dperm.h"

#define D_EXT_MAX_ARITY 16
#define D_PATH_BUF      4096
#define D_MAX_CANDIDATES 3
#define D_MAX_FN_NAME   128

#ifdef _WIN32
  #define D_PATH_SEP '\\'
  #define D_LIB_SUFFIX ".dll"
  #define D_OS_NAME "Windows"
#elif defined(__APPLE__)
  #define D_PATH_SEP '/'
  #define D_LIB_SUFFIX ".dylib"
  #define D_OS_NAME "macOS"
#else
  #define D_PATH_SEP '/'
  #define D_LIB_SUFFIX ".so"
  #define D_OS_NAME "this operating system"
#endif

/**
 * Registry of the extensions
 */

typedef struct d_ext_fn {
  char* name;
  int arity;
  drax_ext_fn* fn;
} d_ext_fn;

typedef struct d_ext_lib {
  char* path;     /* resolved */
  char* module;
  void* handle;
  d_ext_fn* fns;
  int nfns;
  struct d_ext_lib* next;
} d_ext_lib;

struct d_native_registry {
  d_ext_lib* libs;
};

/* what the start function of an extension fills */
struct drax_registry {
  char* module;
  d_ext_fn* fns;
  int n;
  int cap;
  char error[512];  /* the first problem found while registering */
};

/**
 * The call that is running. It remembers the lists and frames that the
 * extension created during the call: only those can be changed
 * (list_push, frame_set). A list or a frame that came from the script is
 * shared (by other variables, by the lists made with List.tail) and must not
 * change under it. A hash set of pointers: no removals, grows when half full.
 */
struct drax_call {
  d_vm* vm;
  void** made;
  size_t made_cap;    /* a power of 2, 0 until something is created */
  size_t made_count;
};

static size_t made_slot(const void* p, size_t cap) {
  uintptr_t h = (uintptr_t) p;
  h ^= h >> 17;
  h *= (uintptr_t) 0x9E3779B1u;
  return (size_t) (h >> 4) & (cap - 1);
}

static int made_has(const drax_call* c, const void* p) {
  size_t i;
  if (c->made_cap == 0) return 0;
  for (i = made_slot(p, c->made_cap); c->made[i] != NULL; i = (i + 1) & (c->made_cap - 1)) {
    if (c->made[i] == p) return 1;
  }
  return 0;
}

static int made_insert(drax_call* c, void* p) {
  size_t i;

  if (c->made_count * 2 >= c->made_cap) {
    size_t ncap = c->made_cap == 0 ? 16 : c->made_cap * 2;
    void** n = (void**) calloc(ncap, sizeof(void*));
    size_t k;
    if (n == NULL) return 0;
    for (k = 0; k < c->made_cap; k++) {
      if (c->made[k] == NULL) continue;
      for (i = made_slot(c->made[k], ncap); n[i] != NULL; i = (i + 1) & (ncap - 1)) {}
      n[i] = c->made[k];
    }
    free(c->made);
    c->made = n;
    c->made_cap = ncap;
  }

  for (i = made_slot(p, c->made_cap); c->made[i] != NULL; i = (i + 1) & (c->made_cap - 1)) {
    if (c->made[i] == p) return 1;
  }
  c->made[i] = p;
  c->made_count++;
  return 1;
}

static char* dup_str(const char* s) {
  size_t n = strlen(s);
  char* c = (char*) malloc(n + 1);
  if (c == NULL) return NULL;
  memcpy(c, s, n + 1);
  return c;
}

static void reg_error(drax_registry* r, const char* fmt, const char* a, int b) {
  if (r->error[0] != '\0') return; /* the first one is the cause */
  snprintf(r->error, sizeof(r->error), fmt, a, b);
}

/**
 * Names
 */

static int is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static int is_upper(char c) { return c >= 'A' && c <= 'Z'; }
static int is_digit(char c) { return c >= '0' && c <= '9'; }

static int valid_module_name(const char* s) {
  size_t i;
  size_t n = strlen(s);
  if (n == 0 || n > D_MAX_FN_NAME || !is_upper(s[0])) return 0;
  for (i = 1; i < n; i++) {
    if (!is_alpha(s[i]) && !is_digit(s[i]) && s[i] != '_') return 0;
  }
  return 1;
}

static int valid_function_name(const char* s) {
  size_t i;
  size_t n = strlen(s);
  if (n == 0 || n > D_MAX_FN_NAME) return 0;
  if (!(s[0] >= 'a' && s[0] <= 'z') && s[0] != '_') return 0;
  for (i = 1; i < n; i++) {
    if (!is_alpha(s[i]) && !is_digit(s[i]) && s[i] != '_') return 0;
  }
  return 1;
}

/**
 * The API for the start function
 */

static int api_module(drax_registry* r, const char* name) {
  if (name == NULL || !valid_module_name(name)) {
    reg_error(r, "the module name '%s' is not valid: it must start with an upper case letter and have only "
                 "letters, digits and '_' (example: \"Greet\")", name == NULL ? "(null)" : name, 0);
    return 0;
  }

  free(r->module);
  r->module = dup_str(name);
  return r->module != NULL;
}

static int api_function(drax_registry* r, const char* name, int arity, drax_ext_fn* fn) {
  int i;

  if (name == NULL || !valid_function_name(name)) {
    reg_error(r, "the function name '%s' is not valid: it must start with a lower case letter or '_' and have only "
                 "letters, digits and '_' (example: \"read_row\")", name == NULL ? "(null)" : name, 0);
    return 0;
  }

  if (arity < 0 || arity > D_EXT_MAX_ARITY) {
    reg_error(r, "the function '%s' was registered with %d arguments: a function has from 0 to 16 arguments", name, arity);
    return 0;
  }

  if (fn == NULL) {
    reg_error(r, "the function '%s' was registered without code (the function pointer is NULL)", name, 0);
    return 0;
  }

  for (i = 0; i < r->n; i++) {
    if (r->fns[i].arity == arity && strcmp(r->fns[i].name, name) == 0) {
      reg_error(r, "the function '%s' was registered twice with %d arguments", name, arity);
      return 0;
    }
  }

  if (r->n >= r->cap) {
    int cap = r->cap == 0 ? 8 : r->cap * 2;
    d_ext_fn* f = (d_ext_fn*) realloc(r->fns, sizeof(d_ext_fn) * (size_t) cap);
    if (f == NULL) { reg_error(r, "out of memory registering '%s'", name, 0); return 0; }
    r->fns = f;
    r->cap = cap;
  }

  r->fns[r->n].name = dup_str(name);
  r->fns[r->n].arity = arity;
  r->fns[r->n].fn = fn;
  if (r->fns[r->n].name == NULL) { reg_error(r, "out of memory registering '%s'", name, 0); return 0; }
  r->n++;
  return 1;
}

static void api_fail(drax_registry* r, const char* message) {
  if (r->error[0] != '\0') return;
  snprintf(r->error, sizeof(r->error), "%s", message == NULL ? "(no message)" : message);
}

/**
 * The API for the functions: reading values
 */

static int api_type(drax_xval xv) {
  drax_value v = (drax_value) xv;

  if (IS_NIL(v)) return DRAX_T_NIL;
  if (IS_BOOL(v)) return DRAX_T_BOOL;
  if (IS_NUMBER(v)) return DRAX_T_NUMBER;

  if (IS_STRUCT(v)) {
    switch (DRAX_STYPEOF(v)) {
      case DS_STRING:   return DRAX_T_STRING;
      case DS_LIST:     return DRAX_T_LIST;
      case DS_FRAME:    return DRAX_T_FRAME;
      case DS_ERROR:    return DRAX_T_ERROR;
      case DS_FUNCTION:
      case DS_NATIVE:   return DRAX_T_FUNCTION;
      case DS_TID:      return DRAX_T_HANDLE;
      default:          return DRAX_T_OTHER;
    }
  }

  return DRAX_T_OTHER;
}

static const char* api_type_name(drax_xval v) {
  switch (api_type(v)) {
    case DRAX_T_NIL:      return "nil";
    case DRAX_T_BOOL:     return "boolean";
    case DRAX_T_NUMBER:   return "number";
    case DRAX_T_STRING:   return "string";
    case DRAX_T_LIST:     return "list";
    case DRAX_T_FRAME:    return "frame";
    case DRAX_T_ERROR:    return "error";
    case DRAX_T_FUNCTION: return "function";
    case DRAX_T_HANDLE:   return "handle";
    default:              return "value";
  }
}

static int api_to_number(drax_xval xv, double* out) {
  drax_value v = (drax_value) xv;
  if (!IS_NUMBER(v) || out == NULL) return 0;
  *out = CAST_NUMBER(v);
  return 1;
}

static int api_to_bool(drax_xval xv, int* out) {
  drax_value v = (drax_value) xv;
  if (!IS_BOOL(v) || out == NULL) return 0;
  *out = CAST_BOOL(v) ? 1 : 0;
  return 1;
}

static int api_to_string(drax_xval xv, const char** chars, size_t* length) {
  drax_value v = (drax_value) xv;
  if (!IS_STRING(v) || chars == NULL || length == NULL) return 0;
  *chars = CAST_STRING(v)->chars;
  *length = (size_t) CAST_STRING(v)->length;
  return 1;
}

static size_t api_list_length(drax_xval xv) {
  drax_value v = (drax_value) xv;
  return IS_LIST(v) ? (size_t) CAST_LIST(v)->length : 0;
}

static drax_xval api_list_at(drax_xval xv, size_t i) {
  drax_value v = (drax_value) xv;
  if (!IS_LIST(v) || i >= (size_t) CAST_LIST(v)->length) return DRAX_NIL_VAL;
  return CAST_LIST(v)->elems[i];
}

static size_t api_frame_length(drax_xval xv) {
  drax_value v = (drax_value) xv;
  return IS_FRAME(v) ? (size_t) CAST_FRAME(v)->length : 0;
}

static const char* api_frame_key(drax_xval xv, size_t i) {
  drax_value v = (drax_value) xv;
  if (!IS_FRAME(v) || i >= (size_t) CAST_FRAME(v)->length) return NULL;
  return CAST_FRAME(v)->literals[i];
}

static drax_xval api_frame_value(drax_xval xv, size_t i) {
  drax_value v = (drax_value) xv;
  if (!IS_FRAME(v) || i >= (size_t) CAST_FRAME(v)->length) return DRAX_NIL_VAL;
  return CAST_FRAME(v)->values[i];
}

static int api_frame_get(drax_xval xv, const char* key, drax_xval* out) {
  drax_value v = (drax_value) xv;
  drax_value found;
  if (!IS_FRAME(v) || key == NULL || out == NULL) return 0;
  if (get_value_dframe(CAST_FRAME(v), (char*) key, &found) == -1) return 0;
  *out = found;
  return 1;
}

/**
 * Handles: a pointer of the extension that the GC releases
 */

typedef struct d_ext_handle {
  void* ptr;
  void (*release)(void*);
  char* tag;
} d_ext_handle;

static void ext_handle_destroy(drax_value v) {
  d_ext_handle* h = (d_ext_handle*) (uintptr_t) v;
  if (h == NULL) return;
  if (h->release != NULL) h->release(h->ptr);
  free(h->tag);
  free(h);
}

static int api_get_handle(drax_xval xv, const char* tag, void** out) {
  drax_value v = (drax_value) xv;
  if (!IS_TID(v) || CAST_TID(v)->destroy != ext_handle_destroy || out == NULL) return 0;

  d_ext_handle* h = (d_ext_handle*) (uintptr_t) CAST_TID(v)->value;
  if (h == NULL || strcmp(h->tag, tag == NULL ? "" : tag) != 0) return 0;

  *out = h->ptr;
  return 1;
}

/**
 * The API for the functions: creating values
 */

static drax_xval api_nil(drax_call* c) { (void) c; return DRAX_NIL_VAL; }

static drax_xval api_boolean(drax_call* c, int b) { (void) c; return BOOL_VAL(b); }

static drax_xval api_number(drax_call* c, double n) { (void) c; return NUMBER_VAL(n); }

static drax_xval api_error(drax_call* c, const char* message) {
  return DS_VAL(new_derror(c->vm, (char*) (message == NULL ? "error" : message)));
}

static drax_xval api_errorf(drax_call* c, const char* format, ...) {
  char buf[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  return api_error(c, buf);
}

static drax_xval api_string(drax_call* c, const char* chars, size_t length) {
  if (chars == NULL) length = 0;

  if (length >= (size_t) INT_MAX) {
    return api_error(c, "the extension tried to create a string that is too long (2 GB or more)");
  }

  /* the bytes are kept as they are: no escape sequence is converted */
  char* copy = (char*) malloc(length + 1);
  if (copy == NULL) return api_error(c, "out of memory creating a string");
  if (length > 0) memcpy(copy, chars, length);
  copy[length] = '\0';

  return DS_VAL(new_dstring(c->vm, copy, (int) length));
}

static drax_xval api_list(drax_call* c, size_t capacity) {
  if (capacity > 1048576) capacity = 1048576; /* it grows when needed */
  drax_list* l = new_dlist(c->vm, (int) capacity);
  if (!made_insert(c, l)) return api_error(c, "out of memory creating a list");
  return DS_VAL(l);
}

static int api_list_push(drax_call* c, drax_xval list, drax_xval item) {
  drax_value v = (drax_value) list;
  if (!IS_LIST(v) || !made_has(c, CAST_STRUCT(v))) return 0;

  drax_list* l = CAST_LIST(v);

  /* doubles the room (put_value_dlist grows by 10: a long result would be quadratic) */
  if (l->length >= l->cap) {
    int ncap = l->cap < 8 ? 16 : (l->cap > INT_MAX / 2 ? INT_MAX : l->cap * 2);
    if (ncap <= l->length) return 0;
    drax_value* n = (drax_value*) realloc(l->elems, sizeof(drax_value) * (size_t) ncap);
    if (n == NULL) return 0;
    c->vm->gc_meta->bytes += sizeof(drax_value) * (size_t) (ncap - l->cap);
    l->elems = n;
    l->cap = ncap;
  }

  put_value_dlist(l, (drax_value) item);
  return 1;
}

static drax_xval api_frame(drax_call* c) {
  drax_frame* f = new_dframe(c->vm, 8);
  if (!made_insert(c, f)) return api_error(c, "out of memory creating a frame");
  return DS_VAL(f);
}

static int api_frame_set(drax_call* c, drax_xval frame, const char* key, drax_xval value) {
  drax_value v = (drax_value) frame;
  if (!IS_FRAME(v) || key == NULL || !made_has(c, CAST_STRUCT(v))) return 0;
  put_value_dframe(CAST_FRAME(v), (char*) key, (drax_value) value);
  return 1;
}

static drax_xval api_handle(drax_call* c, void* pointer, const char* tag, void (*release)(void*)) {
  d_ext_handle* h = (d_ext_handle*) malloc(sizeof(d_ext_handle));
  if (h == NULL) {
    if (release != NULL) release(pointer);
    return api_error(c, "out of memory creating a handle");
  }

  h->ptr = pointer;
  h->release = release;
  h->tag = dup_str(tag == NULL ? "" : tag);
  if (h->tag == NULL) {
    free(h);
    if (release != NULL) release(pointer);
    return api_error(c, "out of memory creating a handle");
  }

  drax_tid* t = new_dtid(c->vm, (drax_value) (uintptr_t) h);
  t->destroy = ext_handle_destroy;
  return DS_VAL(t);
}

static const drax_api the_api = {
  DRAX_EXT_ABI_VERSION,
  (uint32_t) sizeof(drax_api),

  api_module,
  api_function,
  api_fail,

  api_type,
  api_type_name,
  api_to_number,
  api_to_bool,
  api_to_string,
  api_list_length,
  api_list_at,
  api_frame_length,
  api_frame_key,
  api_frame_value,
  api_frame_get,
  api_get_handle,

  api_nil,
  api_boolean,
  api_number,
  api_string,
  api_list,
  api_list_push,
  api_frame,
  api_frame_set,
  api_error,
  api_errorf,
  api_handle,
};

/**
 * Every function of every extension
 */

static drax_value ext_trampoline(d_vm* vm, int* stat) {
  d_ext_fn* f = (d_ext_fn*) vm->native_udata;
  drax_xval argv[D_EXT_MAX_ARITY];
  int i;

  /* the last argument is on the top of the stack */
  for (i = f->arity - 1; i >= 0; i--) argv[i] = (drax_xval) pop(vm);

  drax_call call;
  call.vm = vm;
  call.made = NULL;
  call.made_cap = 0;
  call.made_count = 0;

  drax_xval r = f->fn(&the_api, &call, argv);

  free(call.made);

  DX_SUCESS_FN(stat);
  return (drax_value) r;
}

/**
 * Paths
 */

static int has_dir_sep(const char* p) {
  if (strchr(p, '/') != NULL) return 1;
#ifdef _WIN32
  if (strchr(p, '\\') != NULL) return 1;
#endif
  return 0;
}

static int is_file(const char* p) {
  struct stat st;
  return stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

static int is_dir(const char* p) {
  struct stat st;
  return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static int ends_with(const char* s, const char* suffix) {
  size_t a = strlen(s);
  size_t b = strlen(suffix);
  return a >= b && strcmp(s + a - b, suffix) == 0;
}

/**
 * The files that Native.load("x") tries, in order:
 *   ./x            (a path without a directory is relative to the current one:
 *                   the system never searches its library directories)
 *   ./x.so         (the suffix of the system, when it is missing)
 *   ./libx.so      (the same with the prefix "lib", not on Windows)
 * Returns the number of candidates.
 */
static int build_candidates(const char* p, char cand[D_MAX_CANDIDATES][D_PATH_BUF]) {
  int n = 0;
  /* the callers limit the path, so "./", "lib" and the suffix always fit */
  char first[D_PATH_BUF - 16];
  size_t flen;

  if (has_dir_sep(p)) snprintf(first, sizeof(first), "%s", p);
  else snprintf(first, sizeof(first), "./%s", p);
  flen = strlen(first);

  memcpy(cand[n], first, flen + 1);
  n++;

  if (!ends_with(first, D_LIB_SUFFIX)) {
    memcpy(cand[n], first, flen);
    memcpy(cand[n] + flen, D_LIB_SUFFIX, sizeof(D_LIB_SUFFIX));
    n++;

#ifndef _WIN32
    {
      const char* slash = strrchr(first, '/');
      size_t dir = slash == NULL ? 0 : (size_t) (slash + 1 - first);
      if (strncmp(first + dir, "lib", 3) != 0) {
        memcpy(cand[n], first, dir);
        memcpy(cand[n] + dir, "lib", 3);
        memcpy(cand[n] + dir + 3, first + dir, flen - dir);
        memcpy(cand[n] + flen + 3, D_LIB_SUFFIX, sizeof(D_LIB_SUFFIX));
        n++;
      }
    }
#endif
  }

  return n;
}

static int full_path(const char* in, char* out) {
#ifdef _WIN32
  return GetFullPathNameA(in, D_PATH_BUF, out, NULL) != 0;
#else
  char tmp[PATH_MAX];
  if (realpath(in, tmp) == NULL) return 0;
  snprintf(out, D_PATH_BUF, "%s", tmp);
  return 1;
#endif
}

/* the file that 'p' means, absolute. 0 when there is none ('tried' lists what was looked for) */
static int resolve_library(const char* p, char* out, char* tried, size_t tried_size) {
  char cand[D_MAX_CANDIDATES][D_PATH_BUF];
  int n = build_candidates(p, cand);
  int i;

  if (tried != NULL && tried_size > 0) tried[0] = '\0';

  for (i = 0; i < n; i++) {
    if (is_file(cand[i]) && full_path(cand[i], out)) return 1;

    if (tried != NULL && tried_size > 0) {
      size_t used = strlen(tried);
      snprintf(tried + used, tried_size - used, "%s'%s'", i == 0 ? "" : ", ", cand[i]);
    }
  }

  return 0;
}

static int native_allowed(d_vm* vm, const char* resolved) {
  const d_perms* p = &vm->perms;
  int i;

  if (dperm_allows(p, D_PERM_NATIVE)) return 1;

  for (i = 0; i < p->native_count; i++) {
    char other[D_PATH_BUF];
    if (resolve_library(p->native[i], other, NULL, 0) && strcmp(other, resolved) == 0) return 1;
    if (strcmp(p->native[i], resolved) == 0) return 1;
  }

  return 0;
}

/**
 * The system loader
 */

#ifdef _WIN32
static void* lib_open(const char* path, char* err, size_t err_size) {
  HMODULE h = LoadLibraryA(path);
  if (h == NULL) {
    DWORD code = GetLastError();
    char* text = NULL;
    DWORD n = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL, code, 0, (LPSTR) &text, 0, NULL);
    if (n > 0 && text != NULL) {
      while (n > 0 && (text[n - 1] == '\n' || text[n - 1] == '\r' || text[n - 1] == ' ' || text[n - 1] == '.')) text[--n] = '\0';
      snprintf(err, err_size, "%s (error %lu)", text, (unsigned long) code);
      LocalFree(text);
    } else {
      snprintf(err, err_size, "error %lu", (unsigned long) code);
    }
    return NULL;
  }
  return (void*) h;
}

static void* lib_symbol(void* h, const char* name) {
  return (void*) GetProcAddress((HMODULE) h, name);
}

static void lib_close(void* h) {
  FreeLibrary((HMODULE) h);
}
#else
static void* lib_open(const char* path, char* err, size_t err_size) {
  dlerror(); /* clears the last error */
  void* h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (h == NULL) {
    const char* e = dlerror();
    snprintf(err, err_size, "%s", e != NULL ? e : "unknown error");
  }
  return h;
}

static void* lib_symbol(void* h, const char* name) {
  return dlsym(h, name);
}

static void lib_close(void* h) {
  dlclose(h);
}
#endif

/**
 * Native.load
 */

typedef int (*ext_abi_fn)(void);
typedef int (*ext_init_fn)(const drax_api*, drax_registry*);

static const char* value_type_name(drax_value v) {
  return api_type_name((drax_xval) v);
}

/* an expected failure: the function returns an Error (the script can handle it) */
static drax_value load_failed(d_vm* vm, int* stat, const char* fmt, ...) {
  char msg[16384];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  DX_SUCESS_FN(stat);
  return DS_VAL(new_derror(vm, msg));
}

static void free_registry_fns(drax_registry* r) {
  int i;
  for (i = 0; i < r->n; i++) free(r->fns[i].name);
  free(r->fns);
  free(r->module);
}

static d_ext_lib* find_lib_by_path(d_vm* vm, const char* path) {
  d_ext_lib* l;
  if (vm->ext == NULL) return NULL;
  for (l = vm->ext->libs; l != NULL; l = l->next) {
    if (strcmp(l->path, path) == 0) return l;
  }
  return NULL;
}

static d_ext_lib* find_lib_by_module(d_vm* vm, const char* module) {
  d_ext_lib* l;
  if (vm->ext == NULL) return NULL;
  for (l = vm->ext->libs; l != NULL; l = l->next) {
    if (strcmp(l->module, module) == 0) return l;
  }
  return NULL;
}

drax_value d_native_load(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  if (!IS_STRING(a)) {
    DX_ERROR_FN(stat);
    char msg[256];
    snprintf(msg, sizeof(msg),
      "Native.load: expected the path of the library as a string (example: Native.load(\"./greet\")), got %s.",
      value_type_name(a));
    return DS_VAL(new_derror(vm, msg));
  }

  const char* path = CAST_STRING(a)->chars;

  if (CAST_STRING(a)->length == 0) {
    return load_failed(vm, stat, "Native.load: the path of the library is empty (example: Native.load(\"./greet\")).");
  }

  if ((size_t) CAST_STRING(a)->length != strlen(path) || CAST_STRING(a)->length > D_PATH_BUF - 64) {
    return load_failed(vm, stat, "Native.load: the path of the library is not valid (it is too long or has a NUL byte).");
  }

  /* 1. the file */
  char resolved[D_PATH_BUF];
  char tried[D_PATH_BUF * D_MAX_CANDIDATES + 64];
  int found = resolve_library(path, resolved, tried, sizeof(tried));

  /* 2. the permission, also for a file that does not exist: the answer must not tell what is on the disk */
  if (!dperm_allows(&vm->perms, D_PERM_NATIVE) && !(found && native_allowed(vm, resolved))) {
    return dperm_deny(vm, stat, D_PERM_NATIVE, "Native.load", found ? resolved : path);
  }

  if (!found) {
    if (is_dir(path)) {
      return load_failed(vm, stat,
        "Native.load: '%s' is a directory, not a library file. Give the path of the file (example: Native.load(\"%s/greet\")).",
        path, path);
    }

    char cwd[D_PATH_BUF];
    cwd[0] = '\0';
#ifdef _WIN32
    GetCurrentDirectoryA(sizeof(cwd), cwd);
#else
    if (getcwd(cwd, sizeof(cwd)) == NULL) snprintf(cwd, sizeof(cwd), "?");
#endif
    return load_failed(vm, stat,
      "Native.load: cannot find the library '%s'. Looked for: %s. A relative path is resolved from the current "
      "directory (%s). Check the path, or build the extension first.",
      path, tried, cwd);
  }

  /* 3. already loaded: it is the same module */
  d_ext_lib* known = find_lib_by_path(vm, resolved);
  if (known != NULL) {
    DX_SUCESS_FN(stat);
    MSR(vm, known->module);
  }

  /* 4. open it and check that it is a Drax extension */
  char err[1024];
  void* handle = lib_open(resolved, err, sizeof(err));
  if (handle == NULL) {
    return load_failed(vm, stat,
      "Native.load: the system could not load '%s': %s. Check that the file is a shared library built for %s "
      "(same processor and 32/64 bits as drax) and that the libraries it depends on are installed.",
      resolved, err, D_OS_NAME);
  }

  ext_abi_fn abi_fn = NULL;
  ext_init_fn init_fn = NULL;
  {
    void* s1 = lib_symbol(handle, "drax_ext_abi");
    void* s2 = lib_symbol(handle, "drax_ext_init");
    *(void**) (&abi_fn) = s1;
    *(void**) (&init_fn) = s2;
  }

  if (abi_fn == NULL) {
    lib_close(handle);
    return load_failed(vm, stat,
      "Native.load: '%s' is a shared library, but it is not a Drax extension: it does not export 'drax_ext_abi'. "
      "An extension includes drax_ext.h and uses DRAX_EXTENSION(start_function).", resolved);
  }

  int abi = abi_fn();
  if (abi != DRAX_EXT_ABI_VERSION) {
    lib_close(handle);
    return load_failed(vm, stat,
      "Native.load: '%s' was built for version %d of the Drax extension API, but this Drax uses version %d. "
      "Rebuild the extension with the drax_ext.h that comes with this Drax.",
      resolved, abi, DRAX_EXT_ABI_VERSION);
  }

  if (init_fn == NULL) {
    lib_close(handle);
    return load_failed(vm, stat,
      "Native.load: '%s' does not export 'drax_ext_init'. Use DRAX_EXTENSION(start_function) from drax_ext.h.", resolved);
  }

  /* 5. its start function declares the module. From here the library stays loaded. */
  drax_registry reg;
  memset(&reg, 0, sizeof(reg));

  int rc = init_fn(&the_api, &reg);

  if (rc != 0 || reg.error[0] != '\0') {
    char why[640];
    if (reg.error[0] != '\0') snprintf(why, sizeof(why), "%s", reg.error);
    else snprintf(why, sizeof(why), "its start function returned %d without a message", rc);
    free_registry_fns(&reg);
    return load_failed(vm, stat, "Native.load: the extension '%s' failed to start: %s.", resolved, why);
  }

  if (reg.module == NULL) {
    free_registry_fns(&reg);
    return load_failed(vm, stat,
      "Native.load: the extension '%s' did not declare the name of its module. "
      "Call api->module(registry, \"Name\") in its start function.", resolved);
  }

  if (reg.n == 0) {
    char* module = reg.module;
    reg.module = NULL;
    free_registry_fns(&reg);
    drax_value r = load_failed(vm, stat,
      "Native.load: the extension '%s' (module %s) did not register any function. "
      "Add them with api->function(registry, \"name\", arity, code).", resolved, module);
    free(module);
    return r;
  }

  {
    drax_value existing;
    if (get_mod_table(vm->envs->modules, reg.module, &existing)) {
      d_ext_lib* other = find_lib_by_module(vm, reg.module);
      char* module = reg.module;
      reg.module = NULL;
      free_registry_fns(&reg);

      drax_value r;
      if (other != NULL) {
        r = load_failed(vm, stat,
          "Native.load: the extension '%s' declares the module '%s', but the extension '%s' already uses that name. "
          "Rename one of the modules.", resolved, module, other->path);
      } else {
        r = load_failed(vm, stat,
          "Native.load: the extension '%s' declares the module '%s', but Drax already has a module with that name. "
          "Rename the module in the extension.", resolved, module);
      }
      free(module);
      return r;
    }
  }

  /* 6. the module: the registry owns the names and the data of the functions */
  d_ext_lib* lib = (d_ext_lib*) malloc(sizeof(d_ext_lib));
  lib->path = dup_str(resolved);
  lib->module = reg.module;
  lib->handle = handle;
  lib->fns = reg.fns;
  lib->nfns = reg.n;
  lib->next = NULL;

  if (vm->ext == NULL) {
    vm->ext = (struct d_native_registry*) malloc(sizeof(struct d_native_registry));
    vm->ext->libs = NULL;
  }
  lib->next = vm->ext->libs;
  vm->ext->libs = lib;

  drax_native_module* m = new_native_module(vm, lib->module, lib->nfns);
  {
    drax_native_module_helper* helper = (drax_native_module_helper*) malloc(sizeof(drax_native_module_helper) * (size_t) lib->nfns);
    void** data = (void**) malloc(sizeof(void*) * (size_t) lib->nfns);
    int i;

    for (i = 0; i < lib->nfns; i++) {
      helper[i].arity = lib->fns[i].arity;
      helper[i].name = lib->fns[i].name;
      helper[i].fun = ext_trampoline;
      data[i] = &lib->fns[i];
    }

    put_fun_on_module(m, helper, lib->nfns);
    put_data_on_module(m, data, lib->nfns);
    free(helper);
    free(data);
  }

  put_mod_table(vm->envs->modules, DS_VAL(m));

  DX_SUCESS_FN(stat);
  MSR(vm, lib->module);
}

void dnative_free(d_vm* vm) {
  if (vm->ext == NULL) return;

  d_ext_lib* l = vm->ext->libs;
  while (l != NULL) {
    d_ext_lib* next = l->next;
    int i;
    /* the library is not unloaded: its code can still be referenced (handles, threads of the extension) */
    for (i = 0; i < l->nfns; i++) free(l->fns[i].name);
    free(l->fns);
    free(l->module);
    free(l->path);
    free(l);
    l = next;
  }

  free(vm->ext);
  vm->ext = NULL;
}
