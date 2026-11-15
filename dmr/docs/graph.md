# `dmr graph` - the graph for LLMs

A compact, deterministic **code property graph** of Drax code, so an LLM can
understand a project and write Drax code by reading a few hundred tokens
instead of whole files.

```sh
dmr graph                 # the current directory (recursive)
dmr graph src tests       # files and/or directories
```

The graph is written to `.drax-cpg/`:

| File | Content |
|------|---------|
| `lang.dgraph` | the language: keywords, operator precedence, global functions, native modules |
| `cpg.dgraph` | the analyzed program: files, functions, calls, imports, exports |

An LLM reads `lang.dgraph` once and `cpg.dgraph` for the code it has to change
or extend. Nothing is executed. Dot directories (`.git`, `.drax-cpg`) are
skipped; `lib/` (the installed packages) is analyzed, so calls into
dependencies are resolved.

## Where the data comes from

| Data | Source |
|------|--------|
| keywords, global functions, modules | `src/dlang_data.c`, generated from the interpreter by `tools/gen_lang.sh` (`make lang-sync`; `make lang-check` fails when it is stale) |
| operator precedence | `src/dlang.c` (the only part written by hand) |
| functions, calls, imports, exports | dmr's own scanner (`src/dgraph.c`) over the `.dx` files |

dmr does not link the interpreter, so this part can live in another
repository together with `dlang.*` (see the README).

## Deterministic

Files are sorted by path, everything else follows the source order, ids are
sequential, and there are no timestamps or absolute paths (files outside of
the current directory are the only exception). The same sources always
produce the same bytes; `tests/run.sh` checks it against a golden file.

## Format

Line based, the first word is the kind of the line. `F<n>` is a file, `f<n>` a
function. A function is always `name/arity`; anonymous ones are `_/arity`.

### `lang.dgraph`

```
kw and as do ...              keywords
op.<level> + - ...            infix operators, from the LOWEST to the highest
                              precedence (or < and < equality < diff < term
                              < factor < call); op.prefix are the prefix ones
fn print/1 assert/2 ...       global functions
mod List at/2 map/2 ...       module functions: Module.name(args)
                              overloads share the name: copy/2,3
```

### `cpg.dgraph`

```
F1 lib/util.dx                      file (suffix !parse-error when its blocks are not balanced)
f1 F1 twice/1 x 1-3                 function: id, file, name/arity, params ("-" if none), lines
                                    (+ " ^f<n>" at the end: the function it is nested in)
g F1 name 12                        global variable defined at line 12
i F2 F1 1                           file F2 imports F1 (line 1), or a raw "path" if not analyzed
a F2 u F1                           alias: u = import(...) -> F1
e F1 twice name                     keys of the exported frame
c f4 f4 3                           call from f4 to f4 at line 3
c F2 F3.run/1 8                     call from the top level of F2
```

Call targets:

| Target | Meaning |
|--------|---------|
| `f3` | a function defined in the same file |
| `b:print/1` | global builtin |
| `m:List.at/2` | native module function |
| `F3.run/1` | key `run` of the module imported as `F3` (through an alias) |
| `?name/1` | not resolved: a parameter or local (`f(v)`), or an unknown name |

Arity counts the pipe: `x |> f(a)` is a call of `f/2`.

Imports follow the rules of the interpreter: `./` and `../` are relative to
the importing file, absolute paths, or the default `lib/` directory (`.dx` is
optional, a package directory loads its `main.dx`).

## Limits

- The scanner is lexical, not a full parser: it follows `fun`/`if`/`do`/`end`
  blocks, which is enough for valid code. Invalid code can give a wrong graph.
- Only literal imports are followed: `import(path_variable)` is not.
- Calls over a call result (`f()(x)`) are not recorded.
- Export keys are the ones of a literal frame: `export({a: 1})`.
- A plain call is resolved to a function of the same file only.
- POSIX only.
