#include <pthread.h>
#include "dscheduler.h"
#include "dvm.h"

d_vm **global_vms;

#define INITIAL_SLOTS_VM 2

/**
 * We must only have one active scheduler
 * 
 * change only vm with status VM_STATUS_FINISHED
 */
static void start_schedule_loop(void* arg) {
  while (true) {
    size_t i;
    for (i = 0; i < INITIAL_SLOTS_VM; i++) {
      
      if (global_vms[i]->pstatus == VM_STATUS_FINISHED) {
        drax_value v = pop(global_vms[i]);
        write(global_vms[i]->pipeID, &v, sizeof(v));
        __reset__(global_vms[i]); /* maybe dont initialize instructions struct inside reset */
        global_vms[i]->pstatus = VM_STATUS_STOPED;
        continue;
      } 
      
      if (global_vms[i]->pstatus == VM_STATUS_WORKING) {
        __run_per_batch__(global_vms[i]);

        if (global_vms[i]->pstatus == VM_STATUS_FINISHED) {
          drax_value v = pop(global_vms[i]);
          write(global_vms[i]->pipeID, &v, sizeof(v));
          __reset__(global_vms[i]); /* maybe dont initialize instructions struct inside reset */
          global_vms[i]->pstatus = VM_STATUS_STOPED;
        }
      }
    }
  }
}

/**
 * Create the main scheduler
 * 
 */
int init_scheduler() {
  global_vms = (d_vm **)malloc(sizeof(d_vm *) * INITIAL_SLOTS_VM);

  size_t i;
  for (i = 0; i < INITIAL_SLOTS_VM; i++) {
    global_vms[i] = createVM();
  }

  pthread_t thread;
  if (pthread_create(&thread, NULL, start_schedule_loop, NULL) != 0) {
      perror("Fail to create scheduler");
      return 1;
  }

  return 0;
}

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * await the first vm available
 * 
 * change only vm with status VM_STATUS_STOPED
 */
static void init_process_on_vm(drax_value val, int fd_result) {
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
 */
drax_value run_instruction_on_vm_pool(drax_value fn) {
  int fd[2];
  if (pipe(fd) == -1) {
    return DRAX_NIL_VAL; /* return a error */
  }

  init_process_on_vm(fn, fd[1]);

  drax_value value;
  read(fd[0], &value, sizeof(value));
  return value;
}
