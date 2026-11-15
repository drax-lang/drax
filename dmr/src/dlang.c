/* dmr - drax manager & resolver */

#include <stdio.h>
#include <string.h>

#include "dlang.h"

/**
 * Precedence of the infix operators, from the LOWEST to the highest.
 * Same order as `priorities` in src/dparser.h of the interpreter
 * (iOR < iAND < iEQUALITY < iDIFF < iTERM < iFACTOR < iCALL).
 * This is the only part of the language that is not generated.
 */
const dlang_op_level DLANG_OPERATORS[] = {
  { "or",       "or" },
  { "and",      "and" },
  { "equality", "!= ==" },
  { "diff",     "< <= > >=" },
  { "term",     "+ ++ - |>" },
  { "factor",   "* /" },
  { "call",     "( ." },
  { "prefix",   "! -" },
  { 0, 0 }
};

int dlang_is_keyword(const char* s) {
  int i;
  for (i = 0; DLANG_KEYWORDS[i]; i++) {
    if (strcmp(DLANG_KEYWORDS[i], s) == 0) return 1;
  }
  return 0;
}

static int has_fn(const char* const* list, const char* name, int arity) {
  char key[256];
  int i;
  snprintf(key, sizeof(key), "%s/%d", name, arity);
  for (i = 0; list[i]; i++) {
    if (strcmp(list[i], key) == 0) return 1;
  }
  return 0;
}

int dlang_has_builtin(const char* name, int arity) {
  return has_fn(DLANG_BUILTINS, name, arity);
}

int dlang_has_module_fn(const char* mod, const char* name, int arity) {
  int i;
  for (i = 0; DLANG_MODULES[i].name; i++) {
    if (strcmp(DLANG_MODULES[i].name, mod) == 0) {
      return has_fn(DLANG_MODULES[i].fns, name, arity);
    }
  }
  return 0;
}
