/* An extension built for another version of the API. */
#include "drax_ext.h"

DRAX_EXPORT int drax_ext_abi(void) { return 999; }
DRAX_EXPORT int drax_ext_init(const drax_api* api, drax_registry* r) { (void) api; (void) r; return 0; }
