/* drax Lang
 *
 * Drax extension API: the header to write a native extension (a shared
 * library: .so on Linux, .dylib on macOS, .dll on Windows) that a script
 * loads with Native.load("path/to/library").
 *
 * This file is self contained: an extension only includes it, it does not
 * link with the interpreter and it never sees the structures of the VM.
 *
 * ---------------------------------------------------------------------------
 * A complete extension
 * ---------------------------------------------------------------------------
 *
 *   #include "drax_ext.h"
 *
 *   static drax_xval add(const drax_api* d, drax_call* c, const drax_xval* a) {
 *     double x, y;
 *     if (!d->to_number(a[0], &x))
 *       return d->errorf(c, "Greet.add: argument 1 must be a number, got %s", d->type_name(a[0]));
 *     if (!d->to_number(a[1], &y))
 *       return d->errorf(c, "Greet.add: argument 2 must be a number, got %s", d->type_name(a[1]));
 *     return d->number(c, x + y);
 *   }
 *
 *   static int start(const drax_api* d, drax_registry* r) {
 *     d->module(r, "Greet");            // the name the script uses: Greet.add(1, 2)
 *     d->function(r, "add", 2, add);    // name, number of arguments, code
 *     return 0;                         // 0 = ok, anything else = failure
 *   }
 *
 *   DRAX_EXTENSION(start)
 *
 * Build:  cc -shared -fPIC -I<drax>/src -o greet.so greet.c      (Linux)
 *         cc -shared -fPIC -I<drax>/src -o greet.dylib greet.c   (macOS)
 *         gcc -shared -I<drax>/src -o greet.dll greet.c          (Windows)
 *
 * Use:    Native.load("./greet")        # the suffix is optional
 *         print(Greet.add(1, 2))        # 3
 *
 * ---------------------------------------------------------------------------
 * Rules
 * ---------------------------------------------------------------------------
 *
 *  - Values are opaque (drax_xval). They are only valid while the function
 *    that received them is running: never keep one in a variable that
 *    outlives the call. To keep native data, create a handle (api->handle).
 *  - A string read with to_string is owned by the interpreter. The pointer
 *    is valid until the function returns, it is always NUL terminated and
 *    it may also contain NUL bytes: use the length.
 *  - Every value that is created (string, list, frame, error...) belongs to
 *    the interpreter, which frees it. A function always returns a value:
 *    api->nil(c) when it has nothing to return.
 *  - An expected failure (bad argument, a database that does not answer) is
 *    returned as an error: return d->error(c, "message"). The script
 *    handles it like any other Error (Error.is, Error.message, |?>).
 *  - A function must not block forever and must not call back into the
 *    interpreter. It runs on the thread of the script.
 *  - Written code runs with all the permissions of the process, there is no
 *    sandbox for native code: see the --safe mode of the interpreter.
 *
 * ---------------------------------------------------------------------------
 * Compatibility
 * ---------------------------------------------------------------------------
 *
 * DRAX_EXT_ABI_VERSION changes only when an existing function of the table
 * changes or is removed. New functions are added at the end of drax_api, so
 * an extension built with an older header keeps working. An extension built
 * for another version is refused with a message that says so.
 */

#ifndef DRAX_EXT_H
#define DRAX_EXT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRAX_EXT_ABI_VERSION 1

#if defined(_WIN32)
  #define DRAX_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
  #define DRAX_EXPORT __attribute__((visibility("default")))
#else
  #define DRAX_EXPORT
#endif

/** the call that is running (needed to create values) */
typedef struct drax_call drax_call;

/** where an extension declares its module and functions (only in the start function) */
typedef struct drax_registry drax_registry;

/** a Drax value, opaque: valid only during the call */
typedef uint64_t drax_xval;

/** what api->type returns */
enum {
  DRAX_T_NIL = 0,
  DRAX_T_BOOL,
  DRAX_T_NUMBER,
  DRAX_T_STRING,
  DRAX_T_LIST,
  DRAX_T_FRAME,
  DRAX_T_ERROR,
  DRAX_T_FUNCTION,
  DRAX_T_HANDLE,  /* a value created by api->handle */
  DRAX_T_OTHER
};

struct drax_api;

/**
 * The code of a function of the module. 'args' has as many values as the
 * arity that the function was registered with.
 */
typedef drax_xval (drax_ext_fn)(const struct drax_api* api, drax_call* call, const drax_xval* args);

typedef struct drax_api {
  /* the version of the interpreter (DRAX_EXT_ABI_VERSION) and sizeof of this table */
  uint32_t abi_version;
  uint32_t size;

  /* -- start function: declare the module -------------------------------- */

  /** name of the module in the script: starts with an upper case letter, then letters, digits and '_' */
  int (*module)(drax_registry* r, const char* name);

  /** adds Module.name(arity arguments), 0..16 arguments. Returns 0 on failure (and the load fails with a message) */
  int (*function)(drax_registry* r, const char* name, int arity, drax_ext_fn* fn);

  /** the text shown to the user when the start function fails (return nonzero after it) */
  void (*fail)(drax_registry* r, const char* message);

  /* -- read values (return 0 when the value is not of that type) ---------- */

  int (*type)(drax_xval v);
  const char* (*type_name)(drax_xval v);                     /* "number", "string", "list"... (for messages) */
  int (*to_number)(drax_xval v, double* out);
  int (*to_bool)(drax_xval v, int* out);
  int (*to_string)(drax_xval v, const char** chars, size_t* length);

  size_t (*list_length)(drax_xval v);                        /* 0 for anything that is not a list */
  drax_xval (*list_at)(drax_xval v, size_t index);           /* nil when out of range */

  size_t (*frame_length)(drax_xval v);                       /* number of keys */
  const char* (*frame_key)(drax_xval v, size_t index);       /* NULL when out of range */
  drax_xval (*frame_value)(drax_xval v, size_t index);       /* nil when out of range */
  int (*frame_get)(drax_xval v, const char* key, drax_xval* out);

  /** the pointer of a handle created with the same 'tag' (0 for any other value) */
  int (*get_handle)(drax_xval v, const char* tag, void** out);

  /* -- create values ------------------------------------------------------ */

  drax_xval (*nil)(drax_call* c);
  drax_xval (*boolean)(drax_call* c, int b);
  drax_xval (*number)(drax_call* c, double n);
  drax_xval (*string)(drax_call* c, const char* chars, size_t length);   /* copies the bytes */
  drax_xval (*list)(drax_call* c, size_t capacity);
  int (*list_push)(drax_call* c, drax_xval list, drax_xval item);        /* 0 when 'list' is not a list */
  drax_xval (*frame)(drax_call* c);
  int (*frame_set)(drax_call* c, drax_xval frame, const char* key, drax_xval value);

  /** an Error value: what a function returns for an expected failure */
  drax_xval (*error)(drax_call* c, const char* message);
  drax_xval (*errorf)(drax_call* c, const char* format, ...);

  /**
   * A value that holds a pointer of the extension (a connection, a file...).
   * 'release' is called once with the pointer when the interpreter collects
   * the value, or when the program ends. 'tag' names the kind of pointer, so
   * a function can refuse a handle of another kind (get_handle).
   */
  drax_xval (*handle)(drax_call* c, void* pointer, const char* tag, void (*release)(void* pointer));
} drax_api;

/**
 * The extension was built with this header. The interpreter that loads it
 * may be older and have a shorter table: the functions at the end of the
 * table would be read beyond it. 'expected' is sizeof(drax_api) of the header
 * of the extension. Returns 0 (and sets the message of the load) when the
 * table of the interpreter is shorter.
 */
static inline int drax_ext_api_ok(const drax_api* api, size_t expected, drax_registry* registry) {
  if ((size_t) api->size >= expected) return 1;
  api->fail(registry, "this extension was built with a newer drax_ext.h than this Drax understands "
                      "(the Drax API table is shorter). Update Drax, or rebuild the extension with the "
                      "drax_ext.h that comes with this Drax");
  return 0;
}

#ifdef __cplusplus
  #define DRAX_EXT_C extern "C"
#else
  #define DRAX_EXT_C
#endif

/**
 * Put it once in the file of the extension. 'start' is
 *   int start(const drax_api* api, drax_registry* registry)
 * and returns 0 when everything is fine.
 */
#define DRAX_EXTENSION(start)                                                          \
  DRAX_EXT_C DRAX_EXPORT int drax_ext_abi(void) { return DRAX_EXT_ABI_VERSION; }       \
  DRAX_EXT_C DRAX_EXPORT int drax_ext_init(const drax_api* api, drax_registry* registry) { \
    if (!drax_ext_api_ok(api, sizeof(drax_api), registry)) return 1;                   \
    return start(api, registry);                                                       \
  }

#ifdef __cplusplus
}
#endif

#endif
