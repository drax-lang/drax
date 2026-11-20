/* An extension built with a header that has more functions than the interpreter's table. */
#include "drax_ext.h"

DRAX_EXPORT int drax_ext_abi(void) { return DRAX_EXT_ABI_VERSION; }

DRAX_EXPORT int drax_ext_init(const drax_api* api, drax_registry* r) {
  if (!drax_ext_api_ok(api, sizeof(drax_api) + 64, r)) return 1;
  return 0;
}
