// An extension written in C++: DRAX_EXTENSION must export the names without mangling.
#include "drax_ext.h"

static drax_xval twice(const drax_api* d, drax_call* c, const drax_xval* a) {
  double x;
  if (!d->to_number(a[0], &x)) return d->error(c, "Cpp.twice: expected a number");
  return d->number(c, x * 2);
}

static int start(const drax_api* d, drax_registry* r) {
  d->module(r, "Cpp");
  d->function(r, "twice", 1, twice);
  return 0;
}

DRAX_EXTENSION(start)
