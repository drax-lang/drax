# Writing a native extension

An extension is a shared library that includes `src/drax_ext.h` (one file, no
other header, no link with the interpreter). A script loads it with
`Native.load` (see [native-extensions.md](native-extensions.md)).

## The smallest extension

```c
#include "drax_ext.h"

static drax_xval add(const drax_api* d, drax_call* c, const drax_xval* a) {
  double x, y;
  if (!d->to_number(a[0], &x))
    return d->errorf(c, "Greet.add: argument 1 must be a number, got %s", d->type_name(a[0]));
  if (!d->to_number(a[1], &y))
    return d->errorf(c, "Greet.add: argument 2 must be a number, got %s", d->type_name(a[1]));
  return d->number(c, x + y);
}

static int start(const drax_api* d, drax_registry* r) {
  d->module(r, "Greet");           /* the name in the script: Greet.add(1, 2) */
  d->function(r, "add", 2, add);   /* name, number of arguments, code */
  return 0;                        /* 0 = ok, anything else = failure */
}

DRAX_EXTENSION(start)
```

Build (from the root of the repository, or point `-I` to `src`):

| System | Command |
|---|---|
| Linux | `cc -shared -fPIC -I src -o greet.so greet.c` |
| macOS | `cc -shared -fPIC -I src -o greet.dylib greet.c` |
| Windows (MinGW) | `gcc -shared -I src -o greet.dll greet.c` |
| C++ | the same with `c++`/`g++`; `DRAX_EXTENSION` exports the names without mangling |

Use:

```elixir
Native.load('./greet')
print(Greet.add(1, 2))     # 3
print(Greet.add(1, 'x'))   # Error("Greet.add: argument 2 must be a number, got string")
```

## The parts

- `DRAX_EXTENSION(start)` exports the two functions the interpreter looks for:
  `drax_ext_abi` (the version of the API) and `drax_ext_init` (calls `start`).
  It also refuses to start when the interpreter is older than the header.
- `start(api, registry)` runs once, on load. It only **declares** the module and
  its functions. It returns 0 when everything is fine.
- A function is `drax_xval f(const drax_api* api, drax_call* call, const drax_xval* args)`.
  `args` has as many values as the arity it was registered with, in order.
  It always returns a value (`api->nil(call)` when there is nothing to return).

## Registering (only in `start`)

| Function | What it does |
|---|---|
| `module(r, name)` | the name of the module: starts with an upper case letter, then letters, digits and `_`. It must not be the name of a builtin module or of another extension |
| `function(r, name, arity, fn)` | adds `Module.name`, 0 to 16 arguments. The name starts with a lower case letter or `_`. The same name can be registered with different arities. Returns 0 on failure and the load fails with a message |
| `fail(r, message)` | the text shown to the user when `start` fails (return nonzero after it) |

## Reading values

All of them return 0 (or 0 items) when the value is not of that type, so check
the result and return an `Error` that says what was wrong.

| Function | Returns |
|---|---|
| `type(v)` | `DRAX_T_NIL`, `BOOL`, `NUMBER`, `STRING`, `LIST`, `FRAME`, `ERROR`, `FUNCTION`, `HANDLE`, `OTHER` |
| `type_name(v)` | `"number"`, `"string"`... for messages |
| `to_number(v, &double)` | 1 on success |
| `to_bool(v, &int)` | 1 on success |
| `to_string(v, &chars, &length)` | 1 on success. `chars` is NUL terminated and can also contain NUL bytes: use `length` |
| `list_length(v)`, `list_at(v, i)` | the number of items (0 if not a list); the item (nil out of range) |
| `frame_length(v)`, `frame_key(v, i)`, `frame_value(v, i)` | walk a frame, in the order of its keys |
| `frame_get(v, key, &out)` | 1 when the key exists |
| `get_handle(v, tag, &pointer)` | 1 when `v` is a handle created with that `tag` |

## Creating values

| Function | Creates |
|---|---|
| `nil(c)`, `boolean(c, b)`, `number(c, n)` | the scalars |
| `string(c, chars, length)` | a string: the bytes are copied and kept as they are (no escape sequence is converted) |
| `list(c, capacity)`, `list_push(c, list, item)` | a list. `list_push` returns 0 when it is refused (see below) |
| `frame(c)`, `frame_set(c, frame, key, value)` | a frame (keys are strings) |
| `error(c, message)`, `errorf(c, format, ...)` | an `Error` value |
| `handle(c, pointer, tag, release)` | a value that holds a pointer of the extension |

### Errors

An expected failure is **returned**, not signalled: `return d->error(c, "message")`.
The script handles it like any `Error`. There is nothing to throw; the function
just returns a value. Write messages that say what to do:

```c
return d->errorf(c, "Db.query: expected a string with the SQL, got %s", d->type_name(a[0]));
```

### Lists and frames

`list_push` and `frame_set` only work on a list or a frame that **this call
created** (with `list` or `frame`). A list or a frame that came from the script
is shared with other variables, and `List.tail` makes lists that share its
elements: it is never changed (the function returns 0). Build a new one.

### Handles

```c
typedef struct { FILE* f; } file_t;

static void file_release(void* p) { fclose(((file_t*) p)->f); free(p); }

static drax_xval open_file(const drax_api* d, drax_call* c, const drax_xval* a) {
  ...
  return d->handle(c, f, "myext.file", file_release);
}

static drax_xval read_line(const drax_api* d, drax_call* c, const drax_xval* a) {
  void* p;
  if (!d->get_handle(a[0], "myext.file", &p))
    return d->errorf(c, "File.read_line: expected a file, got %s", d->type_name(a[0]));
  ...
}
```

`release` is called once with the pointer when the garbage collector frees the
value and when the program ends. The `tag` names the kind of pointer: a handle
of another kind (or any other value) is refused by `get_handle`. Use a tag that
includes the name of the extension.

## Rules

- A value (`drax_xval`) is valid **only while the function that received it
  runs**. Never keep one in a variable that lives longer. To keep data, use a
  handle.
- A string from `to_string` belongs to the interpreter: do not free it, do not
  keep it after returning.
- Everything the function creates belongs to the interpreter, which frees it.
- A function must not call back into the interpreter, must not block for ever,
  and runs on the thread of the script.
- A function that returns a value that does not exist (a random number, a
  pointer) is a bug that no check can catch: the value is trusted.
- The extension runs with all the rights of the process. There is no sandbox for
  native code; `--safe` only decides which extensions load
  ([safe-mode.md](safe-mode.md)).

## Compatibility

`DRAX_EXT_ABI_VERSION` (now `1`) changes only when a function of the table
changes or is removed. New functions are added **at the end** of `drax_api`:

- an extension built with an older header keeps working with a newer Drax;
- an extension built with a newer header than the Drax that loads it is refused
  with a message (the table of that Drax is shorter);
- an extension built for another version of the API is refused with a message
  that says both versions.

## Examples

`tests/drax/ext/` has extensions that use every function of the API
(`greet.c`), and the ones that fail on purpose to show the messages
(`plain.c`, `badabi.c`, `badinit.c`, `badreg.c`, `clash.c`, `newapi.c`).
`tests/drax/ext/build.sh` builds them.
