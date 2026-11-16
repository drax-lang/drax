# dmr - drax manager & resolver

The package manager of Drax, and the home of everything that helps LLMs
write Drax code.

```sh
make                 # builds ./bin/dmr
dmr init             # creates a default package.toml in the current directory
dmr add user/repo    # adds a dependency to package.toml (GitHub, git url or path)
dmr install          # downloads the dependencies of package.toml into ./lib, writes package.lock
dmr update           # same, ignoring package.lock
dmr graph [paths]    # writes the code graph for LLMs into ./.drax-cpg
```

| Doc | Content |
|-----|---------|
| [docs/package.md](docs/package.md) | `package.toml`, `package.lock`, how `install` works, security |
| [docs/graph.md](docs/graph.md) | `dmr graph`: the graph for LLMs, its format |

## Layout

| Path | Role |
|------|------|
| `src/main.c` | commands |
| `src/dmanifest.*`, `src/dlock.*`, `src/dinstall.*`, `src/dadd.*` | package manager (manifest, lock, resolver, git, `add`) |
| `src/dfs.*` | file system and process helpers |
| `src/dgraph.*` | **AI part**: scanner of `.dx` and the graph emitter |
| `src/dlang.*`, `src/dlang_data.c` | **AI part**: the language as data |
| `tools/gen_lang.sh` | generates `src/dlang_data.c` from the interpreter |
| `tests/` | `sh tests/run.sh` (offline, uses temporary git repositories) |

## Moving dmr to its own repository

dmr is standalone: it has its own Makefile and does not link the interpreter.
Copy the `dmr/` directory and it builds and tests as it is.

The interpreter and dmr only share two contracts:

1. **Where packages live.** dmr writes to `./lib`, and the interpreter
   searches `lib/` for `import('name')`; a package directory is loaded from
   its `main.dx` (see `docs/imports.md` of the interpreter).
2. **The language table.** `src/dlang_data.c` (keywords, global functions,
   native modules) is generated from the interpreter by `make lang-sync`.
   `make lang-check` fails when it is stale and is skipped when the
   interpreter sources are not next to dmr (`DRAX_SRC=path`).

The AI part (`dgraph.*`, `dlang.*`) uses nothing of the package manager except
`dfs.*`, so it can also be split from the package manager if needed.

## Not in the interpreter

The interpreter has no code for any of this. `drax` only knows the `lib/`
directory; downloading, locking and the graph are dmr's job.
