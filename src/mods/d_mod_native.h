/* drax Lang
 *
 * Native extensions: Native.load("path") loads a shared library (.so, .dylib
 * or .dll) written with src/drax_ext.h and adds its module to the program.
 */

#ifndef __D_MOD_NATIVE
#define __D_MOD_NATIVE

#include "../dvm.h"

/* Native.load(path): the name of the module, or an Error (see the messages) */
drax_value d_native_load(d_vm* vm, int* stat);

/* forgets the loaded extensions (the libraries stay loaded until the process ends) */
void dnative_free(d_vm* vm);

#endif
