# Recursion and tail calls

Drax is a functional language: it has no `while` or `for`. Every loop is a
recursive function, so recursion has to be as cheap and as safe as a loop is
in an imperative language.

The model follows Erlang and its virtual machine (BEAM):

| Kind of call | Example | Memory | Limit |
|---|---|---|---|
| **Tail call** | `loop(n - 1, acc + n)` as the last thing a function does | constant: the frame is reused | none, it can run forever |
| **Body call** | `n + sum(n - 1)` (there is work left after the call) | grows with the depth | the memory of the machine, and `MAX_CALL_DEPTH` |

A recursive function never crashes the interpreter. A deep recursion either
works or stops with a clean error:

```
stack overflow: the maximum call depth was exceeded.
  ./file.dx:5
  ... 9999981 more frame(s)
```

## What is a tail call

A call is a **tail call** when the function has nothing left to do after it,
other than returning its result. This is a property of *where* the call is,
not of the syntax used to write it.

```elixir
fun sum_to(n, acc) do
  if (n == 0) do
    acc
  else
    sum_to(n - 1, acc + n)     # tail call: its result is the result of sum_to
  end
end

fun sum_body(n) do
  if (n == 0) do
    0
  else
    n + sum_body(n - 1)        # not a tail call: the `+` runs after the call
  end
end
```

Both functions work for any `n`. `sum_to` runs in constant memory.
`sum_body` uses memory proportional to `n` (about 70 bytes per level).

### Tail positions

| Code | Is the call a tail call? |
|---|---|
| `f(x)` as the last expression of a function | yes |
| `if (c) do f(x) else g(x) end` as the last expression | yes, both `f` and `g` |
| an `if` nested inside another `if` in tail position | yes |
| `return f(x)`, anywhere in the function | yes |
| `c and f(x)` / `c or f(x)` in tail position | yes, the right side |
| `x \|> f()` in tail position | yes |
| `x \|?> f()` in tail position | yes (see [error handling](./error-handling.md)) |
| `f?(x)` (protected call), anywhere | no, the frame must stay to catch the failure; a protected function can still tail call inside itself |
| `&(n - 1, acc)` (self call operator) in tail position | yes |
| `m.next(x)` (function stored in a frame) in tail position | yes |
| a native function (`Number.to_string(x)`) in tail position | runs as a normal call, a native never grows the Drax stack |
| `n + f(x)`, `f(x) * 2`, `"a" ++ f(x)` | no, the operator runs after the call |
| `-f(x)`, `!f(x)` | no |
| `[f(x)]`, `{ a: f(x) }` | no, the list or frame is built after the call |
| `y = f(x)` | no, the assignment runs after the call |
| `f(x)` followed by more expressions | no |
| `f(g(x))` | `f` can be, `g` never is (it is an argument) |

### Writing loops

To loop in constant memory, keep the state in parameters (accumulators) and
make the recursive call the last thing the function does:

```elixir
# body recursion: simple, memory grows with the list
fun len(l) do
  if (List.is_empty(l)) do 0 else 1 + len(List.tail(l)) end
end

# tail recursion: constant memory, any size
fun count(l, acc) do
  if (List.is_empty(l)) do acc else count(List.tail(l), acc + 1) end
end
```

Like in Erlang, body recursion is not wrong: it is often clearer and, for
lists that fit in memory, fast enough. Use tail recursion for loops that
run for a long time (servers, event loops, counters) or over very large
inputs.

Mutual recursion is also a tail call:

```elixir
fun is_even(n) do
  if (n == 0) do true else is_odd(n - 1) end
end

fun is_odd(n) do
  if (n == 0) do false else is_even(n - 1) end
end

is_even(10000000)   # constant memory
```

`is_even` uses `is_odd` before it is defined. When a function is created,
the global names that already exist are captured, and the names that do not
exist yet are resolved when they are called (late binding, like the calls
between functions of an Erlang module).

Functions are identified by name and arity, like in Erlang, so a public
function can delegate to a tail recursive version with an accumulator:

```elixir
fun total(l) do total(l, 0) end

fun total(l, acc) do
  if (List.is_empty(l)) do acc else total(List.tail(l), acc + List.at(l, 0)) end
end
```

A call `total(l)` looks for `total/1`. A function used as a value (`f = total`)
is the last definition with that name.

## How it works

### 1. The compiler finds the tail calls

Every call emitted inside a function body is recorded (`put_call` in
`src/dparser.c`). When the body is complete, `optimize_tail_calls` checks
each one: if the next instruction after the call, following unconditional
jumps (`OP_JMP`), is `OP_RETURN`, the call is changed to its tail version:

| Normal call | Tail call |
|---|---|
| `OP_D_CALL` | `OP_D_CALL_T` |
| `OP_D_CALL_P` (pipe) | `OP_D_CALL_P_T` |

This is the definition of a tail call applied to the bytecode, so it covers
every syntax at once (`if`/`else`, `return`, `and`/`or`, pipes, nested
blocks) and never marks a call that has work after it.

Bytecode of the two functions above (output of `make opcode`, simplified:
constants and argument counts are shown inline; locals are addressed by
slot number, here `n` is slot 0 and `acc` is slot 1):

```
sum_to                          sum_body
  ...                             ...
  OP_GET_G_ID, "sum_to"           OP_GET_L_ID, 0
  OP_GET_L_ID, 0                  OP_GET_G_ID, "sum_body"
  OP_PUSH, 1                      OP_GET_L_ID, 0
  OP_SUB                          OP_PUSH, 1
  OP_GET_L_ID, 1                  OP_SUB
  OP_GET_L_ID, 0                  OP_D_CALL, 1      <- normal call
  OP_ADD                          OP_ADD            <- work after the call
  OP_D_CALL_T, 2   <- tail call   OP_RETURN
  OP_RETURN
```

The `else` branch is the last branch of the `if`, so the call is followed
directly by `OP_RETURN`. The `then` branch ends with an `OP_JMP` to the end
of the `if`, the compiler follows it and finds the same `OP_RETURN`.

### 2. Call frames

Each call to a Drax function pushes a frame (`d_frame` in `src/dvm.h`):

```c
typedef struct d_frame {
  d_instructions* instr;          /* caller code     */
  drax_value* ip;                 /* caller position */
  d_generic_var_table* env_global;
  int stack_base;                 /* where the callee data starts on the value stack */
  int local_base;                 /* where the callee locals start on the local stack */
} d_frame;
```

The VM has three stacks:

```
value stack   [ ...caller values... | callee values ............ ]
                                    ^ stack_base
local stack   [ ...caller locals... | callee params and locals . ]
                                    ^ local_base
call stack    [ frame | frame | ... | callee frame ]
```

- **`OP_D_CALL`** pushes a frame and saves both bases.
- **`OP_RETURN`** takes the top of the value stack as the result (`nil` if the
  function produced nothing), moves both stacks back to the bases, pops
  the frame and pushes the result. Nothing the function left behind leaks.
- **`OP_D_CALL_T`** reuses the current frame. It moves the arguments down to
  `stack_base`, moves the local stack back to `local_base`, reserves the
  locals of the new function and jumps to its code. The call stack does not
  grow, and neither do the other two. This is what Erlang calls *last call
  optimization* (`call_last` in BEAM).

If the value in a tail call is not a Drax function (a native function, or
an invalid value), it falls back to a normal call. The next instruction is
`OP_RETURN`, so the result is the same.

### 3. Stacks that grow

Like the stack of an Erlang process, the three stacks start small and
double when they are full (`grow_stack`, `ensure_local_table_capacity` and
`callstack_push` in `src/dvm.c` and `src/dhandler.c`). Local variables are
stored inline in the local stack (`d_local_var_node`), there is no
allocation per variable.

### 4. Limits

| Limit | Default | Change it with |
|---|---|---|
| Body recursion depth (`MAX_CALL_DEPTH`) | 10,000,000 calls | `DRAX_MAX_CALL_DEPTH=<n> drax file.dx`, or at build time with `make DWN_CCFLAGS=-DMAX_CALL_DEPTH=<n>` |
| Recursion through a native function (`MAX_NATIVE_NESTING`) | 5,000 levels | `make DWN_CCFLAGS=-DMAX_NATIVE_NESTING=<n>` |

`MAX_CALL_DEPTH` exists only to give a clean error before the machine runs
out of memory, like `max_heap_size` in Erlang. Tail calls never count
towards it. At the default limit, body recursion uses about 700 MB.

`MAX_NATIVE_NESTING` protects the C stack. `List.map`, `List.filter` and
`List.tap` call Drax functions by running a nested VM loop in C. A
recursion that goes *through* them (a function that calls `List.map` with
itself as the callback) nests one C loop per level, so it has a much
smaller limit. Recursion between Drax functions never uses the C stack.

When a limit is reached, the error shows the top of the stack trace (at
most 20 lines) and how many frames were left out.

## Numbers

Measured on Linux x86_64, default build (`make all`):

| Program | Before | Now |
|---|---|---|
| tail recursive loop, 3,000,000 iterations | 2.63 s, 345 MB | 1.70 s, 11.5 MB |
| tail recursive loop, 10,000,000 iterations | memory corruption | 7.5 s, 11.5 MB |
| body recursion, 1,000,000 levels | `vm_error: function is not valid!` | 0.44 s, 75 MB |
| `fib(25)` | 0.18 s, 32 MB | 0.17 s, 11.5 MB |
| unbounded body recursion | segmentation fault | clean error, exit 1 |

Before this change, tail calls did not free the locals of the frame they
replaced, so long loops wrote past the end of the local stack. Many calls
were also marked as tail calls by looking only at the next token, which
broke expressions like `n + f(n - 1)` inside an `if`.

## Related behavior

Two changes make every expression produce exactly one value, which the
frame model depends on:

- An `if` without `else` (or with an empty branch) evaluates to `nil`.
- An assignment is an expression and evaluates to the assigned value
  (`x = 42` as the last line of a function returns `42`), like a match in
  Erlang.

## Tests

| File | What it checks |
|---|---|
| `tests/drax/tail_calls.dx` | every tail position in the table above, 100,000 iterations each. It runs with `DRAX_MAX_CALL_DEPTH=1000`, so any call that is not optimized fails with a stack overflow. |
| `tests/drax/recursion.dx` | body recursion (1,000,000 levels), calls inside operators, lists, frames and assignments, recursion through `List.map`. |
| `tests/drax/TCO.dx` | the original 1,000,000 iteration loops. |
| `tests/drax/errors.dx` | unbounded recursion and recursion through a native function end with a clean error. |

To check a change to the compiler or to the VM, also run the suite with
AddressSanitizer:

```
make all LIGHT=1 DWN_CCFLAGS="-g -fsanitize=address -fsanitize-recover=address"
mkdir -p /tmp/asan
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0:log_path=/tmp/asan/r sh tests/drax/run-test.sh
grep SUMMARY /tmp/asan/* | sort | uniq -c
```

`log_path` keeps the reports out of the output of the tests that run
`./bin/drax` as a subprocess (`errors.dx`). Rebuild with `make all`
afterwards.
