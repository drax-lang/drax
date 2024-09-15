  /* drax Lang - 2024
 * Jean Carlos (jeantux)
 */

 
#ifndef __D_MOD_HTTP
#define __D_MOD_HTTP

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
    #include <windows.h>
    #define snprintf _snprintf

    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <arpa/inet.h>
#endif

#include "../dscheduler.h"
#include "../dvm.h"

typedef struct dhttp_server {
  int new_socket;
  int server_fd;
  #ifdef _WIN32
    void* address;
  #else
    struct sockaddr_in* address;
  #endif
} dhttp_server;

drax_value start_http_server(d_vm* vm, char *options[], int* fail);

int stop_http_server(d_vm* vm, drax_value v);

int accept_http_server(d_vm* vm, drax_value aconf, drax_value* res);

char* receive_http_server(d_vm* vm, drax_value aconf);

ssize_t send_http_server(d_vm* vm, drax_value aconf, char* s);

int disconnect_client_http_server(d_vm* vm, drax_value aconf);

#endif