# Safe mode (`--safe`)

By default a Drax program can do everything, like any script. With `--safe` it
starts with **no permissions**: only what the `--allow-*` options grant works.
A function that needs another permission fails with a message that says which
option to use.

```
drax script.dx                                  # everything is allowed (default)
drax --safe script.dx                           # nothing that needs a permission works
drax --safe --allow-run script.dx               # only running commands
drax --safe --allow-native=./greet.so script.dx # only that extension
```

Options go **before** the script. `drax script.dx --safe` is refused (status 2),
because an ignored `--safe` would be a program that is not safe.

## The permissions

| Option | Allows | Functions |
|---|---|---|
| `--allow-run` | running system commands | `Os.cmd`, `Os.cmd_with_status`, `Os.system` |
| `--allow-net` | using the network | `TCPServer.new` |
| `--allow-env` | reading environment variables | `Os.get_env` |
| `--allow-fs` | changing the file system | `Os.mkdir`, `Os.pmkdir` |
| `--allow-native` | loading any native extension | `Native.load` |
| `--allow-native=a.so,b.so` | loading only those extensions | `Native.load` |
| `--allow-all` | everything | |

- Each option allows only its own permission.
- The order of the options does not matter (`--allow-run --safe` is the same).
- Without `--safe` the `--allow-*` options change nothing: everything is allowed.
- `--allow-native` can be repeated: `--allow-native=a.so --allow-native=b.so`.
- Paths in `--allow-native=` are compared after they are resolved (symbolic
  links followed), so `./bin/greet`, `bin/greet.so` and the absolute path are
  the same extension.
- The network permission is checked when a server is created. The functions
  that work on a connection (`TCPServer.accept`, `Http.*`) can only receive a
  value that `TCPServer.new` created.

What needs no permission: the language, `List`, `String`, `Number`, `Math`,
`Frame`, `Error`, `print`, `inspect`, `Core`, and `import`.

`import` is not a permission: the imported code runs with the **same**
permissions as the code that imported it, so a package cannot do more than the
program that uses it.

## The messages

A denied call is a runtime error. It ends the program, unless it was made with
`f?()` (then it is an `Error`, see [error-handling.md](error-handling.md)).

```
Os.cmd: permission denied. This program runs in safe mode and is not allowed to run system commands. To allow it, start drax with --allow-run (example: drax --safe --allow-run script.dx).
```

For an extension the message names the file (absolute path) and the exact option:

```
Native.load: permission denied. This program runs in safe mode and is not allowed to load the native extension '/home/me/app/bin/greet.so'. To allow it, start drax with --allow-native=/home/me/app/bin/greet.so (or --allow-native to allow every extension).
```

The denial does not depend on the file existing: `Native.load` of a missing file
is also "permission denied", so the answer does not tell what is on the disk.

Wrong options end with status 2:

| Command | Message |
|---|---|
| `drax --frobnicate` | `drax: unknown option '--frobnicate'. Run 'drax --help' to see the options.` |
| `drax --allow-ftp` | `drax: unknown option '--allow-ftp'. The permissions are: --allow-run, --allow-net, --allow-env, --allow-fs, --allow-native, --allow-all (see drax --help)` |
| `drax --allow-run=1` | `drax: option '--allow-run' does not take a value (use it as it is: it allows everything of that kind)` |
| `drax --allow-native=` | `drax: option '--allow-native=' needs at least one path (example: --allow-native=./greet.so,./other.so)` |

## What it does not do

Safe mode limits **Drax code** (a script or a package) that does what it should
not. It is not an isolation:

- A native extension that was allowed runs with all the rights of the process.
  Allow only extensions you trust.
- The list of allowed extensions is by path, not by content. Someone who can
  write in the directory can replace the file.
- `import` runs any valid `.dx` file it is pointed at (with the same permissions).
- It does not limit memory, time or the size of what a program reads.

For code that you do not trust at all, use the isolation of the operating system
(containers, seccomp, Landlock) around the whole process.

## Embedding the VM

The table lives in the VM (`vm->perms`, `src/dperm.h`), so a C program that
runs Drax uses the same table the command line fills:

```c
d_vm* vm = createMainVM();
dperm_set_safe(&vm->perms, 1);              // the same as --safe
dperm_allow(&vm->perms, D_PERM_RUN);        // --allow-run
dperm_allow_native_path(&vm->perms, "./greet.so");  // --allow-native=./greet.so
```

## Implementation

| File | What it is |
|---|---|
| `src/dperm.h`, `src/dperm.c` | the table (`d_perms`), the permissions (`d_perm`), the parser of the options and the text of the denials |
| `src/dflags.c` | reads the command line; sends the permission options to `dperm_parse_option` |
| `src/dvm.h` | `d_vm.perms` and `dperm_deny` (the error that a denied function returns) |
| `src/dbuiltin.c` | the check at the start of each function that needs a permission |
| `src/mods/d_mod_native.c` | the check of `Native.load` (it also looks at the list of paths) |

A function checks its permission with:

```c
if (!dperm_allows(&vm->perms, D_PERM_RUN)) return dperm_deny(vm, stat, D_PERM_RUN, "Os.cmd", NULL);
```

The check is inside the function, so an alias (`f = Os.cmd`), a pipe or a
callback (`List.map(xs, Os.cmd)`) cannot skip it. To protect a new function,
add a permission to `d_perm` and to `perm_options` in `dperm.c`, then check it
as above.

Tests: `tests/drax/safe/run-safe-tests.sh` (run by `tests/drax/run-test.sh`).
