/* drax Lang - 2024
 * Jean Carlos (jeantux)
 */

#ifndef __DSCHEDULER
#define __DSCHEDULER

#include "dvm.h"
#include "dtypes.h"

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #include <process.h>

    typedef DWORD dthread_t;
    typedef HANDLE dthread_mutex_t;
    typedef DWORD no_return_p;
    typedef LPTHREAD_START_ROUTINE start_routine_t;
#else
    #include <sys/select.h>
    #include <pthread.h>

    typedef pthread_t dthread_t;
    typedef pthread_mutex_t dthread_mutex_t;
    typedef void* no_return_p;
    typedef void *(*start_routine_t) (void *);
#endif

int init_scheduler(d_vm * main_vm);

drax_value run_instruction_on_vm_pool(drax_value fn, drax_value v);

#endif
