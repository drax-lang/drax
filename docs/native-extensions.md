# Native extensions

An extension is a shared library written in C (or C++) that adds a module to
the program: `.so` on Linux, `.dylib` on macOS, `.dll` on Windows. It is how
Drax talks to a system or a library that has no Drax code (a database client,
a compression library...).

```elixir
Native.load('./greet')      # 'Greet'
Greet.add(1, 2)             # 3
```

How to write one: [writing-extensions.md](writing-extensions.md). How to limit
what a program may load: [safe-mode.md](safe-mode.md).

## `Native.load(path)`

Returns the **name of the module** (a string) or an `Error`.

| Result | When |
|---|---|
| `'Greet'` | loaded, `Greet.*` is now available |
| `'Greet'` | the same file was already loaded (nothing is registered twice) |
| `Error(message)` | an expected failure: the file is missing, it is not an extension, it failed to start... |
| runtime error | `path` is not a string, or `--safe` does not allow it |

Because it returns an `Error`, it works with `Error.is`, `Error.or_else` and
`|?>`:

```elixir
r = Native.load('./db')
if (Error.is(r)) do
    print('no database: ' ++ Error.message(r))
end
```

### Which file

`Native.load('x')` tries, in this order, and uses the first that is a file:

1. `./x`   (a path with no directory is relative to the current directory)
2. `./x.so` (the suffix of the system: `.so`, `.dylib` or `.dll`)
3. `./libx.so` (not on Windows)

The system never searches its own library directories (`LD_LIBRARY_PATH`...):
a bare name is `./name`. The path is resolved to an absolute one (symbolic links
followed) before it is opened; that path is what `--allow-native` is compared
with and what the messages show.

### What happens

1. The file is found, and the permission is checked.
2. The library is opened. It must export `drax_ext_abi` (the version of the API
   it was built for) and `drax_ext_init`.
3. `drax_ext_init` declares the module and its functions. They are not run yet.
4. The module is added to the program. From here `Greet.add(...)` is a native
   function like `List.length(...)`: it works in pipes, with `f?()`, as a value
   (`f = Greet.add`) and in tail calls.

The library stays loaded until the program ends.

### Errors

Each message says what is wrong and what to do.

| Cause | Message |
|---|---|
| the argument is not a string | `Native.load: expected the path of the library as a string (example: Native.load("./greet")), got number.` (runtime error) |
| empty path | `Native.load: the path of the library is empty (example: Native.load("./greet")).` |
| path too long or with a NUL byte | `Native.load: the path of the library is not valid (it is too long or has a NUL byte).` |
| the file does not exist | `Native.load: cannot find the library './x'. Looked for: './x', './x.so', './libx.so'. A relative path is resolved from the current directory (/home/me/app). Check the path, or build the extension first.` |
| it is a directory | `Native.load: './bin' is a directory, not a library file. Give the path of the file (example: Native.load("./bin/greet")).` |
| the system refused it | `Native.load: the system could not load '/abs/x.so': <system text>. Check that the file is a shared library built for this operating system (same processor and 32/64 bits as drax) and that the libraries it depends on are installed.` |
| a library that is not an extension | `Native.load: '/abs/x.so' is a shared library, but it is not a Drax extension: it does not export 'drax_ext_abi'. An extension includes drax_ext.h and uses DRAX_EXTENSION(start_function).` |
| another version of the API | `Native.load: '/abs/x.so' was built for version 2 of the Drax extension API, but this Drax uses version 1. Rebuild the extension with the drax_ext.h that comes with this Drax.` |
| no start function | `Native.load: '/abs/x.so' does not export 'drax_ext_init'. Use DRAX_EXTENSION(start_function) from drax_ext.h.` |
| the start function failed | `Native.load: the extension '/abs/x.so' failed to start: <its message>.` |
| no module name | `Native.load: the extension '/abs/x.so' did not declare the name of its module. Call api->module(registry, "Name") in its start function.` |
| no functions | `Native.load: the extension '/abs/x.so' (module Name) did not register any function. Add them with api->function(registry, "name", arity, code).` |
| the module name is a builtin one | `Native.load: the extension '/abs/x.so' declares the module 'List', but Drax already has a module with that name. Rename the module in the extension.` |
| the module name is used by another extension | `Native.load: the extension '/abs/x.so' declares the module 'Greet', but the extension '/abs/y.so' already uses that name. Rename one of the modules.` |
| the extension is newer than Drax | `... was built with a newer drax_ext.h than this Drax understands (the Drax API table is shorter). Update Drax, or rebuild the extension with the drax_ext.h that comes with this Drax` |

A load that fails leaves nothing behind: the modules that were loaded keep working.

### Calling an extension

An extension function follows the usual rules of Drax:

- A wrong number of arguments is the usual error (`function 'add/1' is not defined`).
  The same name can exist with different numbers of arguments.
- An argument of the wrong type, a database that does not answer, and the like are
  returned as an `Error`, so the program decides what to do:

```elixir
Greet.add(1, 'two')    # Error("Greet.add: argument 2 must be a number, got string")
Greet.add?(1, 2)       # 3
```

### Handles

An extension that keeps native data (a connection, a file) gives the script a
value of its own (`typeof` is `"tid"`). It is released by the garbage collector
when nobody uses it any more, and when the program ends. There is no `close`
that the script must remember to call, unless the extension offers one.

## Limits

- A function has from 0 to 16 arguments.
- An extension cannot call a Drax function (no callbacks yet).
- An extension runs on the thread of the script; a function that blocks, blocks
  the program.
- A native extension runs with all the rights of the process. `--safe` decides
  **which** extensions load, it does not limit what they do.
- The library is never unloaded.

## Implementation

| File | What it is |
|---|---|
| `src/drax_ext.h` | the public header: the API of an extension (self contained) |
| `src/mods/d_mod_native.c` | `Native.load`, the registry of loaded libraries, the API and the function that calls an extension |
| `src/dstructs.h` | `udata` in `drax_native_module` and `drax_os_native`: the data of each function |
| `src/dvm.c` | puts the data of the function in `vm->native_udata` before a native call; frees the registry at the end |

Every function of an extension is the same C function (`ext_trampoline`) with
its own data: it reads the arguments from the stack, calls the code of the
extension with the table of the API and returns the value. The extension never
sees the structures of the VM, only opaque values.

Tests: `tests/drax/native.dx`; the extensions it loads are in `tests/drax/ext/`
(built by `tests/drax/ext/build.sh` into `bin/ext/`).
