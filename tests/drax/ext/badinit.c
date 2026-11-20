/* A start function that fails with a message. */
#include "drax_ext.h"

static int start(const drax_api* d, drax_registry* r) {
  d->module(r, "Broken");
  d->fail(r, "the database driver 'libfoo' is not installed");
  return 1;
}

DRAX_EXTENSION(start)
