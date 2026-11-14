# Garbage collector

Drax uses a **mark and sweep** collector (`src/dgc.c`). Every struct the
VM creates (string, list, frame, function, error, native function,
module, server) is on the list `vm->d_ls`. A cycle marks everything that
is reachable from the roots and frees the rest.

## When it runs

| Trigger | Where |
|---|---|
| Automatically, when the memory allocated since the last cycle reaches `next_gc` | at the start of every call (`dgc_maybe_collect`) |
| `Core.gc_swap()` | anywhere |
| At the end of the program and of each imported file | `__run__` / `__irun__` |

After a cycle, `next_gc` is twice the memory still in use, and at least
`GC_INITIAL_THRESHOLD` (8 MB). Programs that allocate a lot collect
often, programs with a large live heap collect rarely, and the cost of
collecting stays proportional to the allocation (amortized).

Every loop in Drax is a call (tail recursion), so a long running loop
always reaches the safe point. A program that creates garbage all the
time runs in constant memory:

| Program | Memory |
|---|---|
| 1,000,000 iterations creating a list, a string, a frame and a closure each | 12 MB |
| 1,000,000 calls to `typeof` | 16 MB |

`tests/drax/gc_memory.dx` runs with `ulimit -v` (512 MB) in the test
suite. Without the automatic collection it needs a few GB and fails.

## Safe points

The collector only runs where every value in use is reachable from the
roots. In the VM that is the start of a call: the function and the
arguments are on the value stack.

Native functions keep values in C variables, which are not roots. A
native function that runs Drax code (`List.map`, `List.filter`,
`List.tap` call a callback) puts the list, the function and the partial
result on the value stack while the callback runs.

`DGC_STRESS` (build flag) collects at every safe point. It is slow, but
it turns a missing root into a use after free that AddressSanitizer
reports.

## Roots

| Root | Notes |
|---|---|
| Value stack | |
| Locals | slots of every active call |
| Globals | current scope and the scope saved in every call frame |
| Natives and modules | |
| Exported value and imported modules | `vm->imported_files` keeps what each module exports |
| Function running in each frame | `d_frame.callee`: a closure called right away is referenced only by the call |
| Code being executed | constants of `vm->active_instr` and of each frame, from the current instruction to the end |

Top level code (the main file and imported modules) only jumps forward,
so a constant before the current instruction is never used again and can
be collected. The code of a function can run again, but the function is
marked by the frame that runs it, which marks all of its instructions.

## Marking

Marking uses an explicit stack (`gc_meta->gray`) instead of recursion: a
list nested inside a list a million times does not overflow the C stack.
A struct already marked is not visited again.

| Struct | Marks |
|---|---|
| List | its elements and, for a list that shares the buffer of another one, the owner |
| Frame | its values |
| Function | the constants of its instructions |

## Ownership

What each struct frees when it is collected:

| Struct | Frees |
|---|---|
| String, error | the characters |
| List | the elements buffer, only if it owns it (`owner == NULL`) |
| Frame | keys, values and its own copy of each key name |
| Function | its instructions (values, lines and external references) |
| Module | the tables of functions |
| Server (`DS_TID`) | `destroy`: closes the sockets still open and frees the configuration |

- **`List.tail` is O(1)**: the tail points into the buffer of the original
  list and keeps it alive through `owner`. The buffer is freed with the
  owner, never by a tail. Lists are immutable, so nothing writes into a
  shared buffer.
- **Frames copy the key names.** A key can come from a string that is
  collected later (`Frame.put(f, "a" ++ "b", 1)`).
- **A closure has its own line table**, aligned with its instructions
  (it has a prologue with the captured values).
- **Names created by the compiler** (variables, keys, `name/arity`, file
  paths) are operands shared by a function and its closures. They belong
  to the string pool of the VM (`dpool_add`) and are freed with the VM.

## Destroying the VM

`__destroy__` frees every struct, the tables, the stacks, the top level
code and the string pool. `drax.c` registers it with `atexit`, so it also
runs when a program ends with `Os.exit(...)`.

Imported modules run with their own code and global scope. Both are freed
at the end of the import: the exported value is kept by
`vm->imported_files`.

## Checking

```
make memcheck
```

Builds with AddressSanitizer and LeakSanitizer and runs the test suite
twice: with the default threshold and with a 4 KB threshold (the
collector runs all the time). Any leak or memory error fails, also in the
scripts that end with an error (`tests/drax/errors.dx` runs them as
subprocesses).

| File | What it checks |
|---|---|
| `tests/drax/gc.dx` | values in use survive a cycle in the middle of the execution (running closure, captured values, list tails, frame keys, `List.map` callbacks, deeply nested lists) and the collector runs automatically |
| `tests/drax/gc_memory.dx` | memory stays bounded while a program creates garbage |
