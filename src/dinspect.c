#ifdef _AST_INSPECT

#include "dinspect.h"
#include "dtypes.h"
#include <stdio.h>

static void process_ast_inspect(d_vm* vm, int ident) {

}

void __ast_inspect__(d_vm* vm) {
  process_ast_inspect(vm, 0);
}
#endif