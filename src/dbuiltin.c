#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "dbuiltin.h"
#include "ddefs.h"
#include "dtypes.h"
#include "dtime.h"
#include "dvm.h"
#include "deval.h"
#include "dgc.h"

#include "mods/d_mod_os.h"

static drax_value __d_assert(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  if (!IS_BOOL(a)) {
    DX_SUCESS_FN(stat);
    return DRAX_TRUE_VAL;
  }

  if (CAST_BOOL(a)) {
     DX_SUCESS_FN(stat);
     return DRAX_TRUE_VAL;
  }
  
  DX_ERROR_FN(stat);

  if (!IS_STRING(b)) {
    return DS_VAL(new_derror(vm, (char*) "assert error"));
  }

  return DS_VAL(new_derror(vm, CAST_STRING(b)->chars));
}

static drax_value __d_sleep(d_vm* vm, int* stat) { 
  drax_value val = pop(vm);
  return_if_is_not_number(val, stat);
  
  double t = CAST_NUMBER(val);
  dx_sleep(t);
  DX_SUCESS_FN(stat);
  return DRAX_NIL_VAL;
}

static drax_value __d_read(d_vm* vm, int* stat) {
  drax_value val = pop(vm);
  return_if_is_not_string(val, stat);

  int buffer_size = 4096;
  char* buff = malloc(sizeof(char) * buffer_size);
  printf("%s", CAST_STRING(val)->chars);

  if (fgets(buff, buffer_size, stdin) == NULL) {
    DX_ERROR_FN(stat);
    return DS_VAL(new_derror(vm, (char *) "Fail to read input"));
  }
  char* r = replace_special_char('\n', 'n', buff);
  free(buff);
  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, r, strlen(r)));
}

static drax_value __d_print(d_vm* vm, int* stat) {
  print_drax(pop(vm), 0);
  dbreak_line();
  DX_SUCESS_FN(stat);
  return DRAX_NIL_VAL;
}

static drax_value __d_typeof(d_vm* vm, int* stat) {
  DX_SUCESS_FN(stat);
  drax_value val = pop(vm);
  if (IS_STRUCT(val)) {
    switch (DRAX_STYPEOF(val)) {
      case DS_NATIVE:
      case DS_FUNCTION: MSR(vm, "function");
      case DS_STRING: MSR(vm, "string");
      case DS_LIST: MSR(vm, "list");
      case DS_FRAME: MSR(vm, "frame");
      case DS_MODULE: MSR(vm, "module");
      default: break;
    }
  }
  
  if (IS_BOOL(val)) { MSR(vm, "boolean"); }
  if (IS_NIL(val)) { MSR(vm, "nil"); }
  if (IS_NUMBER(val)) { MSR(vm, "number"); }

  DX_SUCESS_FN(stat);
  MSR(vm, "none");
}

/* Module OS */

static drax_value __d_get_env(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_string(a, stat);

  char* env = getenv(CAST_STRING(a)->chars);
  if (env == NULL) {
    DX_SUCESS_FN(stat);
    return DRAX_NIL_VAL;
  }

  DX_SUCESS_FN(stat);
  MSR(vm, env);
}

static drax_value __d_cmd(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_string(a, stat);

  char buf[4096];
  double status = (double) d_command(CAST_STRING(a)->chars, buf, sizeof(buf));
  char* r = replace_special_char('\n', 'n', buf);

  if (status != 0) {
    DX_ERROR_FN(stat);
    return DS_VAL(new_derror(vm, (char *) "Fail to execute command"));
  }
  DX_SUCESS_FN(stat);
  return DS_VAL(new_dstring(vm, r, strlen(r)));
}

static drax_value __d_cmd_with_status(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_string(a, stat);

  char buf[4096];
  double status = (double) d_command(CAST_STRING(a)->chars, buf, sizeof(buf));
  char* r = replace_special_char('\n', 'n', buf);

  drax_list* l = new_dlist(vm, 2);
  put_value_dlist(l, AS_VALUE(status));
  put_value_dlist(l, DS_VAL(new_dstring(vm, r, strlen(r))));
  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

static drax_value __d_mkdir(d_vm* vm, int* stat, int permission) {
  drax_value b = permission ? pop(vm) : DRAX_NIL_VAL;
  drax_value a = pop(vm);

  if (permission) { return_if_is_not_number(b, stat); }

  return_if_is_not_string(a, stat);
  
  mode_t mode = 0;
  if (permission) {
    double d = CAST_NUMBER(b);
    mode = (d >= 0 && d <= MODE_T_MAX) ? (mode_t) d : 0;
    
    if (mode == 0 && d != 0) {
      DX_ERROR_FN(stat);
      return DS_VAL(new_derror(vm, (char *) "Invalid mode"));
    }
  } else {
    mode = umask(0);
    /**
     * sets the permissions for creating the directory, ignoring the umask
     * mode = S_IRWXU | S_IRWXG | S_IRWXO;
     */
  }

  int r = mkdir(CAST_STRING(a)->chars, mode);

  if (r == -1) {
    DX_SUCESS_FN(stat);
    return DRAX_FALSE_VAL;
  }

  DX_SUCESS_FN(stat);
  return DRAX_TRUE_VAL;
}

static drax_value __d_mkdir1(d_vm* v, int* s) { return __d_mkdir(v, s, 0); }
static drax_value __d_mkdir2(d_vm* v, int* s) { return __d_mkdir(v, s, 1); }

static drax_value __d_system(d_vm* vm, int* stat) {
  drax_value a = pop(vm);  
  return_if_is_not_string(a, stat);
  int r = system(CAST_STRING(a)->chars);

  DX_SUCESS_FN(stat);
  return AS_VALUE((double) r);
}

void load_callback_fn(d_vm* vm, vm_builtin_setter* reg) {
  reg(vm, "assert", 2, __d_assert);
  reg(vm, "typeof", 1, __d_typeof);
  reg(vm, "sleep", 1, __d_sleep);
  reg(vm, "read", 1, __d_read);
  reg(vm, "print", 1, __d_print);
}

/**
 * Core module
 */

static drax_value __d_exit(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_number(a, stat);
  int ext_stat = (int) CAST_NUMBER(a);
  exit(ext_stat);
}

static drax_value __d_gc_swap(d_vm* vm, int* stat) {
  dgc_swap(vm);
  DX_SUCESS_FN(stat);
  return DRAX_NIL_VAL;
}

/**
 * Frame module
 */

static drax_value __d_frame_put(d_vm* vm, int* stat) {
  drax_value c = pop(vm);
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_frame(a, stat);
  return_if_is_not_string(b, stat);
  
  drax_frame* o = CAST_FRAME(a);
  drax_frame* n = new_dframe(vm, o->length + 1);
  n->length = o->length;

  memcpy(n->keys, o->keys, o->length * sizeof(int));
  memcpy(n->literals, o->literals, o->length * sizeof(char*));
  memcpy(n->values, o->values, o->length * sizeof(drax_value));

  put_value_dframe(n, CAST_STRING(b)->chars, c);

  DX_SUCESS_FN(stat);
  return DS_VAL(n);
}

static drax_value __d_list_concat(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  return_if_is_not_list(a, stat);
  return_if_is_not_list(b, stat);

  drax_list* l1 = CAST_LIST(a);
  drax_list* l2 = CAST_LIST(b);

  drax_list* l = new_dlist(vm, l1->length + l2->length);
  l->length = l1->length + l2->length;

  memcpy(l->elems, l1->elems, l1->length * sizeof(drax_value));
  memcpy(l->elems + l1->length, l2->elems, l2->length * sizeof(drax_value));

  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

static drax_value __d_list_head(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_list(a, stat);
  drax_list* l1 = CAST_LIST(a);

  DX_SUCESS_FN(stat);
  return l1->length > 0 ? l1->elems[0] : DRAX_NIL_VAL;
}

static drax_value __d_list_tail(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  return_if_is_not_list(a, stat);
  drax_list* l1 = CAST_LIST(a);

  drax_list* l = new_dlist(vm, l1->length -1);
  l->length = l1->length - 1;
  l->elems = l1->elems + 1;
  DX_SUCESS_FN(stat);
  return DS_VAL(l);
}

static drax_value __d_list_length(d_vm* vm, int* stat) {
  drax_value a = pop(vm);
  return_if_is_not_list(a, stat);
  drax_list* l = CAST_LIST(a);

  DX_SUCESS_FN(stat);
  return AS_VALUE(l->length);
}

/*
  TCP module
*/

static drax_value __d_tcp_connect(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  int server = socket(AF_INET, SOCK_STREAM, 0);
  char* addr = CAST_STRING(b)->chars;
  int port = (int) CAST_NUMBER(a);

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(addr);
  address.sin_port = htons(port);
  
  socklen_t addressLength = sizeof(address);

  connect(server,(struct sockaddr*)&address,addressLength);

  drax_frame* frame = new_dframe(vm, 4);
  put_value_dframe(frame, (char*) "port", AS_VALUE(address.sin_port));
  put_value_dframe(frame, (char*) "host", AS_VALUE(address.sin_addr.s_addr));
  put_value_dframe(frame, (char*) "sin_family", AS_VALUE(address.sin_family));
  put_value_dframe(frame, (char*) "socket", AS_VALUE(server));

  DX_SUCESS_FN(stat);
  return DS_VAL(frame);
}

static drax_value __d_tcp_send(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  drax_frame* f = CAST_FRAME(a);

  drax_value port;
  drax_value host;
  drax_value sin_family;
  drax_value socket;

  get_value_dframe(f, (char*) "port", &port);
  get_value_dframe(f, (char*) "host", &host);
  get_value_dframe(f, (char*) "sin_family", &sin_family);
  get_value_dframe(f, (char*) "socket", &socket);

  int server = (int) CAST_NUMBER(socket);

  struct sockaddr_in serverAddress;
  serverAddress.sin_family = (sa_family_t) (int) CAST_NUMBER(sin_family);
  serverAddress.sin_addr.s_addr = (in_addr_t) (int) CAST_NUMBER(host);
  serverAddress.sin_port = (in_port_t) (int) CAST_NUMBER(port);
  
  socklen_t addressLength = sizeof(serverAddress);

  connect(server, (struct sockaddr*)&serverAddress, addressLength);

  drax_string* s1 = CAST_STRING(b);
  char buffer[1024];
  strcpy(buffer, s1->chars);
  buffer[s1->length + 1] = '\0';

  sendto(server, buffer, s1->length + 1, 0, (struct sockaddr*)&serverAddress, addressLength);

  DX_SUCESS_FN(stat);

  return DRAX_NIL_VAL;
}

static drax_value __d_tcp_recive(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  drax_frame* f = CAST_FRAME(a);

  drax_value port;
  drax_value host;
  drax_value sin_family;
  drax_value socket;

  get_value_dframe(f, (char*) "port", &port);
  get_value_dframe(f, (char*) "host", &host);
  get_value_dframe(f, (char*) "sin_family", &sin_family);
  get_value_dframe(f, (char*) "socket", &socket);

  int server = (int) CAST_NUMBER(socket);

  struct sockaddr_in serverAddress;
  serverAddress.sin_family = (sa_family_t) (int) CAST_NUMBER(sin_family);
  serverAddress.sin_addr.s_addr = (in_addr_t) (int) CAST_NUMBER(host);
  serverAddress.sin_port = (in_port_t) (int) CAST_NUMBER(port);
  
  socklen_t addressLength = sizeof(serverAddress);

  connect(server,(struct sockaddr*)&serverAddress,addressLength);

  char buffer[1024];

  recvfrom(server, buffer, 1024, 0, NULL, NULL);

  DX_SUCESS_FN(stat);

  return DS_VAL(new_dstring(vm, buffer, strlen(buffer)));
}

static drax_value __d_tcp_listen(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  char* addr = CAST_STRING(b)->chars;
  int port = (int) CAST_NUMBER(a);

  int socketDescriptor = socket(AF_INET,SOCK_STREAM,0);
  
  struct sockaddr_in socketAddress;
  socketAddress.sin_family = AF_INET;
  socketAddress.sin_port = htons(port);
  socketAddress.sin_addr.s_addr = inet_addr(addr);

  bind(socketDescriptor, (struct sockaddr*) &socketAddress, sizeof(socketAddress));

  listen(socketDescriptor, 5);

  DX_SUCESS_FN(stat);

  return AS_VALUE(socketDescriptor);
}

static drax_value __d_tcp_accept(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  int socket = (int) CAST_NUMBER(a);

  int client = accept(socket, NULL, NULL);

  DX_SUCESS_FN(stat);

  return AS_VALUE(client);
}

static drax_value __d_tcp_read(d_vm* vm, int* stat) {
  drax_value a = pop(vm);

  int client = (int) CAST_NUMBER(a);
  char buffer[1024];

  recv(client, buffer, 1024, 0);

  DX_SUCESS_FN(stat);

  return DS_VAL(new_dstring(vm, buffer, strlen(buffer)));
}

static drax_value __d_tcp_write(d_vm* vm, int* stat) {
  drax_value b = pop(vm);
  drax_value a = pop(vm);

  int client = (int) CAST_NUMBER(a);

  drax_string* s1 = CAST_STRING(b);
  char buffer[1024];
  strcpy(buffer, s1->chars);
  buffer[s1->length + 1] = '\0';

  send(client, buffer, s1->length + 1, 0);

  DX_SUCESS_FN(stat);

  return DRAX_NIL_VAL;
}

/**
 * Entry point for native modules
 */

void create_native_modules(d_vm* vm) {
  /**
   * OS module
  */
  drax_native_module* mos = new_native_module(vm, "os", 6);
  const drax_native_module_helper os_helper[] = {
    {1, "cmd", __d_cmd },
    {1, "cmd_with_status", __d_cmd_with_status },
    {1, "system", __d_system},
    {1, "get_env", __d_get_env },
    {1, "mkdir", __d_mkdir1 },
    {2, "mkdir", __d_mkdir2 },
  };

  put_fun_on_module(mos, os_helper, sizeof(os_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(mos));

  /*
   * TCP module
  */

  drax_native_module* mtcp = new_native_module(vm, "tcp", 7);
  const drax_native_module_helper tcp_helper[] = {
    {2, "connect", __d_tcp_connect },
    {1, "recive",  __d_tcp_recive },
    {2, "listen",  __d_tcp_listen },
    {1, "accept",  __d_tcp_accept },
    {1, "read",    __d_tcp_read },
    {2, "send",    __d_tcp_send },
    {2, "write",    __d_tcp_write },
  };

  put_fun_on_module(mtcp, tcp_helper, sizeof(tcp_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(mtcp));

  /**
   * Core module
  */
  drax_native_module* mcore = new_native_module(vm, "core", 2);
  const drax_native_module_helper core_helper[] = {
    {0, "gc_swap", __d_gc_swap },
    {1, "exit", __d_exit },
  };

  put_fun_on_module(mcore, core_helper, sizeof(core_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(mcore));

  /**
   * Frame Module
   */ 
  drax_native_module* frame = new_native_module(vm, "frame", 1);
  const drax_native_module_helper frame_helper[] = {
    {3, "put", __d_frame_put },
  };

  put_fun_on_module(frame, frame_helper, sizeof(frame_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(frame));

  /**
   * List Module
   */ 
  drax_native_module* list = new_native_module(vm, "list", 4);
  const drax_native_module_helper list_helper[] = {
    {2, "concat", __d_list_concat },
    {1, "head", __d_list_head},
    {1, "tail", __d_list_tail},
    {1, "length", __d_list_length},
  };

  put_fun_on_module(list, list_helper, sizeof(list_helper) / sizeof(drax_native_module_helper)); 
  put_mod_table(vm->envs->modules, DS_VAL(list));
}
