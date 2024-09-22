#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include "dscheduler.h"
#include "dvm.h"
#include "dtime.h"
#include "dstructs.h"

static int dread(int fd, void* v, int sv) {
  #ifdef _WIN32
    return _read(fd, v, sv);
  #else
    return read(fd, v, sv);
  #endif
}

static int dpipe(int fd[2]) {
  #ifdef _WIN32
    return _pipe(fd, 256, O_BINARY);
  #else
    return pipe(fd);
  #endif
}

static int dwrite(int fd, void* v, int sv) {
  #ifdef _WIN32
    return _write(fd, v, sv);
  #else
    return write(fd, v, sv);
  #endif
}

static int dcreate_thread(dthread_t* thread_id, start_routine_t __start_routine) {
  #ifdef _WIN32
    DWORD threadID;
    HANDLE thread_handle = CreateThread(
        NULL,                
        0,                   
        __start_routine,      
        NULL,                
        0,                   
        thread_id           
    );

    if (thread_handle == NULL) {
        return 1;
    }
    return 0;
  #else
    return pthread_create(thread_id, NULL, __start_routine, NULL);
  #endif
}

#ifdef _WIN32
  static void dthread_mutex_lock(dthread_mutex_t* lock) {
    WaitForSingleObject( 
      lock,
      INFINITE
    );
  }
#else
  static void dthread_mutex_lock(dthread_mutex_t* lock) {
    pthread_mutex_lock(lock);
  }
#endif

#ifdef _WIN32
  static void dthread_mutex_unlock(dthread_mutex_t* lock) {
    ReleaseMutex(lock);
  }
#else
  static void dthread_mutex_unlock(dthread_mutex_t* lock) {
    pthread_mutex_unlock(lock);
  }
#endif

d_vm **global_vms;

#define INITIAL_SLOTS_VM 2

static void wait(unsigned ms) {
  /*DEBUG(printf("scheduler waiting\n"));*/
  #ifdef _WIN32
    Sleep(ms);
  #else
    struct timeval t;
    t.tv_sec = ms/1000;
    t.tv_usec = (ms % 1000) * 1000;

    select(0, NULL, NULL, NULL, &t);
  #endif
}

/**
 * We must only have one active scheduler
 * 
 * change only vm with status VM_STATUS_FINISHED
 */
static int close_vm_state(d_vm* vm) {
  if (vm->pstatus == VM_STATUS_FINISHED) {
    drax_value v = pop(vm);
    dwrite(vm->pipeID, &v, sizeof(v));
    __reset__(vm); /* maybe dont initialize instructions struct inside reset */
    vm->pstatus = VM_STATUS_STOPED;
    return 1;
  }

  return 0;
}

static no_return_p start_schedule_loop(void* arg) {
  UNUSED(arg);
  int active_routines = 0;
  size_t i;
  while (true) {
    active_routines = 0;
    for (i = 0; i < INITIAL_SLOTS_VM; i++) {
      
      if (close_vm_state(global_vms[i])) {
        active_routines--;
        continue;
      }
      
      if (global_vms[i]->pstatus == VM_STATUS_WORKING) {
        active_routines++;
        __run_per_batch__(global_vms[i]);

        if (close_vm_state(global_vms[i])) {
          active_routines--;
          continue;
        }
      }
    }

    if (active_routines <= 0) {
      wait(1000);
    }
  }
  return 0;
}

/**
 * Create the main scheduler
 * 
 */
int init_scheduler(d_vm * main_vm) {
  global_vms = (d_vm **) malloc(sizeof(d_vm *) * INITIAL_SLOTS_VM);

  size_t i;
  for (i = 0; i < INITIAL_SLOTS_VM; i++) {
    global_vms[i] = ligth_based_createVM(main_vm, i, 0, 0);
  }

  dthread_t thread;
  int err;
  err = dcreate_thread(&thread, start_schedule_loop);
  if (err != 0) {
      fprintf(stderr, "Fail to create scheduler: %s\n", strerror(err));
      return 1;
  }

  return 0;
}

dthread_mutex_t lock;

/**
 * await the first vm available
 * 
 * change only vm with status VM_STATUS_STOPED
 */
static void init_process_on_vm(drax_value val, int fd_result, drax_value value2push) {
  dthread_mutex_lock(&lock);
  d_vm *v = NULL;

  while (v == NULL) {
    size_t i;
    for (i = 0; i < INITIAL_SLOTS_VM; i++) {

      d_vm *el_v = global_vms[i];

      if (el_v->pstatus == VM_STATUS_STOPED) {
        v = el_v;
        el_v->pipeID = fd_result;
        drax_function* fn = CAST_FUNCTION(val);
        el_v->ip = fn->instructions->values;
        el_v->active_instr = el_v->instructions;

        /**
         * initial slots to local range
         */
        el_v->active_instr->local_range = fn->instructions->local_range;

        /**
         * The process need to be initialized by
         * do_call_function_no_validation, because this
         * function incement the local->count.
         */
        zero_new_local_range(el_v, fn->instructions->local_range);
        push(el_v, value2push);

        el_v->pstatus = VM_STATUS_WORKING;
        break;
        /*do_call_function_no_validation(el_v, fn);*/
      }
    }
  }

  dthread_mutex_unlock(&lock);
}

/**
 * queues the routine to be executed and
 * waits for the return.
 * fn           => arg1/drax_function
 * orphan frame => arg2/drax_value
 */
drax_value run_instruction_on_vm_pool(drax_value fn, drax_value v) {
  int fd[2];
  if (dpipe(fd) == -1) {
    return DRAX_NIL_VAL; /* return a error */
  }

  init_process_on_vm(fn, fd[1], v);

  drax_value value;
  dread(fd[0], &value, sizeof(value));
  return value;
}
