/* drax Lang - 2024
 * Jean Carlos (jeantux)
 */

#ifndef __DSCHEDULER
#define __DSCHEDULER

#include "dvm.h"
#include "dtypes.h"

int init_scheduler(d_vm * main_vm);

drax_value run_instruction_on_vm_pool(drax_value fn);

#endif
