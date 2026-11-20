/* A function with an invalid name. */
#include "drax_ext.h"

static drax_xval f(const drax_api* d, drax_call* c, const drax_xval* a) { (void) a; return d->nil(c); }

static int start(const drax_api* d, drax_registry* r) {
  d->module(r, "Badreg");
  d->function(r, "Bad-Name", 0, f);
  return 0;
}

DRAX_EXTENSION(start)
