/* A second extension (to test the list of allowed ones). */
#include "drax_ext.h"

static drax_xval one(const drax_api* d, drax_call* c, const drax_xval* a) { (void) a; return d->number(c, 1); }

static int start(const drax_api* d, drax_registry* r) {
  d->module(r, "Other");
  d->function(r, "one", 0, one);
  return 0;
}

DRAX_EXTENSION(start)
