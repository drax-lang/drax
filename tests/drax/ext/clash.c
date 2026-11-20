/* A module with the name of a builtin module. */
#include "drax_ext.h"

static drax_xval f(const drax_api* d, drax_call* c, const drax_xval* a) { (void) a; return d->nil(c); }

static int start(const drax_api* d, drax_registry* r) {
  d->module(r, "List");
  d->function(r, "f", 0, f);
  return 0;
}

DRAX_EXTENSION(start)
