# Error handling

Drax has no `try`/`catch`. A failure is a **value** (`Error`) and you choose
where to handle it:

| | |
|---|---|
| `Error(payload)` | creates an error, the payload is any value |
| `Error.is(x)` `Error.payload(x)` `Error.message(x)` `Error.or_else(x, default)` | work with an error |
| `f?(args)` | protected call: a failure inside the call becomes an `Error` |
| `a \|?> f()` | pipe that stops at the first `Error` |

A success is the plain value, there is no wrapper to remove.

## Two kinds of failure

| Kind | Example | Default |
|---|---|---|
| expected failure | invalid input, a missing file | the function **returns** `Error(...)` |
| runtime error | `1 + "a"`, `List.at(5, 0)`, calling `nil` | ends the program (message, trace, exit status 1) |

Nothing changes for a program that does not use the features below. A runtime
error that nobody protects ends the program exactly as before.

## Returning an error

```elixir
fun read_age(s) do
    if (String.contains(s, 'x')) do
        return Error('not a number: ' ++ s)
    end
    String.to_number(s)
end
```

`Error(...)` is only a value. Returning it never stops anything by itself.
The payload can be anything, so the caller can tell the errors apart:

```elixir
return Error({code: 404, msg: 'not found'})
```

```elixir
e = fetch(7)
if (Error.is(e)) do
    print(Error.payload(e).code)   # 404
    print(Error.message(e))        # not found
end
```

`Error.message` is the payload when it is a string, a number, `nil` or
`true`/`false`; the `msg` field when the payload is a frame with a string
`msg`; `<list>` or `<frame>` otherwise.

`Error.payload` and `Error.message` return `nil` for a value that is not an
error. `Error.or_else(x, default)` is `default` when `x` is an error and `x`
otherwise (`nil` is a value, not an error).

Two errors are equal (`==`) when their payloads are equal.

## Protected call: `f?(...)`

```elixir
fun ratio(a, b) do a / b end
fun boom(a) do a + 'x' end

boom(1)           # ends the program: Bad argument in arithmetic expression.
boom?(1)          # Error("Bad argument in arithmetic expression.")
List.at?(5, 0)    # Error("Expected list as argument")
ratio?(10, 2)     # 5, the value as it is
```

`f?(...)` catches every runtime error of the call, at any depth (the functions
called by `f`, and the callbacks of `List.map`, `List.filter`, `List.tap`). The
error becomes an `Error` whose payload is the message. An error that `f`
returns by itself passes through unchanged, so both kinds are handled the same
way after the call.

What is protected is the execution of the call. These still end the program,
because they happen before the call starts:

* the name of the function is not defined: `undefined_fn?(1)`;
* an error while evaluating the arguments: `f?(1 + 'a')`.

A failure caught by `f?()` drops everything the call had on the stack and the
program goes on after the call. A failure that is not protected is still
reported, even after other failures were caught.

`f?()` is never a tail call (the frame has to stay to catch the failure). The
protected function itself can use tail calls as usual: a loop of 100000 tail
calls inside `f?()` runs in constant memory.

## Safe pipe: `|?>`

```elixir
fun process(s) do
    s
    |?> read_age()
    |?> check_range()
    |?> double()
end

process('30')     # 60
process('3x')     # Error("not a number: 3x"), check_range and double did not run
```

When the value on the left is an `Error`, the call on the right is **skipped**
(its arguments are not evaluated either) and the `Error` is the result. For
any other value `|?>` is the same as `|>`, so it is safe to use in a whole
chain even if only one step can fail.

`|?>` only looks at the value that arrives. To also catch a runtime error of a
step, protect that step: `x |?> f?()`.

`|>` does not look at the value: the function receives the `Error`. This is
how an error is handled in the middle of a chain:

```elixir
s |?> read_age() |?> check_range() |> Error.or_else(0)
```

`x |?> f()` in tail position is a tail call.

## Using an error as a value by mistake

An `Error` that reaches an operator that cannot use it ends the program with
**its own message**, not with a generic one:

```elixir
x = Error('boom')
x + 1        # boom
'a' ++ x     # boom
-x           # boom
x < 3        # boom
x(1)         # boom
```

Storing, returning, comparing (`==`), passing to a function and putting in a
list or frame are allowed, so an error can travel until someone handles it.
An `Error` is not `false` or `nil`: `if (x)` takes the branch for an error, test
it with `Error.is`.

## Performance

* The happy path has no extra work: arithmetic and calls are the same
  instructions as before. The checks for an `Error` are in the paths that
  were already failing.
* A call returns with one extra test of a frame flag.
* `f?()` costs one saved state per call; a failure is recovered by restoring
  that state (no `longjmp`, no unwinding of the C stack).
* `|?>` costs one instruction that tests the value.

## Notes

* `?` is not allowed in names: it is the mark of a protected call. `fun is_even?(n)`
  is a syntax error ("'?' is not allowed in names") and a call `is_even?(n)` is a
  protected call of `is_even`. Rename functions that used `?` in the name.
* `Error` is both a module (`Error.is`) and a function (`Error(x)`): a call with
  the name of a module is the native function with that name when there is one.
