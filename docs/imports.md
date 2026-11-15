# Imports

`import(path)` always takes a string and returns the value the module passed to `export`.

| Form | Resolved against |
|------|------------------|
| `import('./a.dx')`, `import('../a.dx')` | the directory of the importing file |
| `import('/abs/a.dx')` | absolute path |
| `import('name')`, `import('dir/name')` | the default `lib/` directory |

- The `.dx` extension is optional.
- `lib/` is searched next to the entry script first, then in the current directory.
- Each module runs once; imports are cached by canonical path (cycles are safe).
- A failed import is not cached.
