#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#ifdef __WIN32__
#include <windows.h>
#else
#include <sys/select.h>
#endif

#include "dscheduler.h"
#include "dvm.h"
#include "dtime.h"
#include "dstructs.h"

d_vm **global_vms;

#define INITIAL_SLOTS_VM 2

static void wait(unsigned ms) {
  DEBUG(printf("scheduler waiting\n"));
  #ifdef __WIN32__
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
    write(vm->pipeID, &v, sizeof(v));
    __reset__(vm); /* maybe dont initialize instructions struct inside reset */
    vm->pstatus = VM_STATUS_STOPED;
    return 1;
  }

  return 0;
}

static void* start_schedule_loop(void* arg) {
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
  global_vms = (d_vm **)malloc(sizeof(d_vm *) * INITIAL_SLOTS_VM);

  size_t i;
  for (i = 0; i < INITIAL_SLOTS_VM; i++) {
    global_vms[i] = ligth_based_createVM(main_vm, i, 0);
  }

  pthread_t thread;
  int err;
  err = pthread_create(&thread, NULL, start_schedule_loop, NULL);
  if (err != 0) {
      fprintf(stderr, "Fail to create scheduler: %s\n", strerror(err));
      return 1;
  }

  return 0;
}

static drax_value convert_orphan_frame_to_frame(d_vm *v, drax_value val) {
  if (!val) return DRAX_NIL_VAL;

  drax_frame* f = CAST_FRAME(val);

  drax_frame* nf = new_dframe(v, f->cap);

  int i;
  for (i = 0; i < f->length; i++) {
    if (IS_STRUCT(f->values[i])) {
      drax_frame* sf = (drax_frame*) convert_orphan_frame_to_frame(v, DS_VAL(f->values[i]));
      put_value_dframe(nf, f->literals[i], DS_VAL(sf));
      continue;
    }

    char* raw_str = f->values[i] ? (char*) f->values[i] : (char*) "";
    drax_string* s = new_dstring(v, raw_str, strlen(raw_str));
    put_value_dframe(nf, f->literals[i], DS_VAL(s));
  }
  
  /**
   * we need to clean unuseds elements
   * free(f);
   **/ 
  return DS_VAL(nf);
}

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * await the first vm available
 * 
 * change only vm with status VM_STATUS_STOPED
 */
static void init_process_on_vm(drax_value val, int fd_result, drax_value value2push) {
  pthread_mutex_lock(&lock);
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
        drax_value nv2p = convert_orphan_frame_to_frame(el_v, value2push);
        push(el_v, nv2p);

        el_v->pstatus = VM_STATUS_WORKING;
        break;
        /*do_call_function_no_validation(el_v, fn);*/
      }
    }
  }

  pthread_mutex_unlock(&lock);
}

/**
 * queues the routine to be executed and
 * waits for the return.
 * fn           => arg1/drax_function
 * orphan frame => arg2/drax_value
 */
drax_value run_instruction_on_vm_pool(drax_value fn, drax_value v) {
  int fd[2];
  if (pipe(fd) == -1) {
    return DRAX_NIL_VAL; /* return a error */
  }

  init_process_on_vm(fn, fd[1], v);

  drax_value value;
  read(fd[0], &value, sizeof(value));
  return value;
}
