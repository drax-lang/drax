/* drax Lang - 2024
 * Jean Carlos (jeantux)
 */

 
#ifndef __D_MOD_HTTP
#define __D_MOD_HTTP

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#include "../dscheduler.h"
#include "../dvm.h"

typedef struct dhttp_server {
  int server_fd;
  int new_socket;
  struct sockaddr_in* address;
} dhttp_server;

drax_value start_http_server(d_vm* vm, char *options[], int* fail);

int stop_http_server(d_vm* vm, drax_value v);

int accept_http_server(d_vm* vm, drax_value aconf, drax_value* res);

char* receive_http_server(d_vm* vm, drax_value aconf);

ssize_t send_http_server(d_vm* vm, drax_value aconf, char* s);

int disconnect_client_http_server(d_vm* vm, drax_value aconf);

#endif