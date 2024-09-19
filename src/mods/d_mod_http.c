#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "d_mod_http.h"
#include "d_mod_os.h"
#include "../dstring.h"

#ifdef _WIN32
  drax_value start_http_server(d_vm* vm, char *options[], int* fail) {
    dhttp_server* configs = (dhttp_server*) malloc(sizeof(dhttp_server));
    configs->server_fd = INVALID_SOCKET;
    configs->new_socket = INVALID_SOCKET;
    configs->address = (struct addrinfo*) malloc(sizeof(struct addrinfo));

    WSADATA wsaData;
    int iResult;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
      perror("WSAStartup failed with error");
      return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    char* port = options[0];
    char* host = options[1];

    iResult = getaddrinfo(host, port, &hints, &result);
    if (iResult != 0) {
      perror("Fail to get address");
      WSACleanup();
      return 1;
    }

    configs->server_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (configs->server_fd == INVALID_SOCKET) {
      perror("Fail to configure");
      WSACleanup();
      return 1;
    }

    iResult = bind(configs->server_fd, result->ai_addr, (int) result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
      closesocket(configs->server_fd);
      WSACleanup();
      *fail = 1;
      return DS_VAL(new_derror(vm, (char *) "server error, fail to bind!"));
    }

    /*freeaddrinfo(result);*/
    iResult = listen(configs->server_fd, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
      closesocket(configs->server_fd);
      WSACleanup();
      *fail = 1;
      return DS_VAL(new_derror(vm, (char *) "server error, fail to listen!"));
    }

    configs->address = result;

    *fail = 0;
    return (drax_value) configs;
  }
#else
  drax_value start_http_server(d_vm* vm, char *options[], int* fail) {
    dhttp_server* configs = (dhttp_server*) malloc(sizeof(dhttp_server));
    configs->address = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));

    configs->new_socket = -1;
    int opt = 1;

    char* port = options[0];
    char* host = options[1];

    char* endptr;
    int iport = strtod(port, &endptr);
    if (endptr == port) iport = 5000;

    if ((configs->server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      perror("Fail to create");
      *fail = 1;
    }

    if (setsockopt(configs->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
      perror("Fail to configure");
      close(configs->server_fd);
      *fail = 1;
    }

    /**
     * Define address and port
     */
    configs->address->sin_family = AF_INET;
    configs->address->sin_addr.s_addr = INADDR_ANY;
    configs->address->sin_port = htons(iport);

    if (inet_pton(AF_INET, host, &configs->address->sin_addr) <= 0) {
      close(configs->server_fd);
      *fail = 1;
      return DS_VAL(new_derror(vm, (char *) "server error, Unsupported host!"));
    }

    if (bind(configs->server_fd, (struct sockaddr *) configs->address, sizeof(*configs->address)) < 0) {
      close(configs->server_fd);
      *fail = 1;
      return DS_VAL(new_derror(vm, (char *) "server error, fail to bind!"));
    }

    if (listen(configs->server_fd, 3) < 0) {
      close(configs->server_fd);
      *fail = 1;
      return DS_VAL(new_derror(vm, (char *) "server error, fail to listen!"));
    }

    *fail = 0;
    return (drax_value) configs;
  }
#endif

#ifdef _WIN32
  int accept_http_server(d_vm* vm, drax_value aconf, drax_value* res) {
    if(!aconf) { return 0; }

    dhttp_server* configs = (dhttp_server*) aconf;
    configs->new_socket = accept(configs->server_fd, NULL, NULL);

    if (configs->new_socket == INVALID_SOCKET) {
        closesocket(configs->new_socket);
        *res = DS_VAL(new_derror(vm, (char *) "server error, fail to accept connection!"));
      return 0;
    }

    return 1;
  }
#else
  int accept_http_server(d_vm* vm, drax_value aconf, drax_value* res) {
    if(!aconf) { return 0; }

    dhttp_server* configs = (dhttp_server*) aconf;
    int addrlen = sizeof(configs->address);

    if ((configs->new_socket = accept(configs->server_fd, (struct sockaddr *) configs->address,
    (socklen_t*) &addrlen)) < 0) {
      close(configs->server_fd);
      *res = DS_VAL(new_derror(vm, (char *) "server error, fail to accept connection!"));
      return 0;
    }

    return 1;
  }
#endif

#ifdef _WIN32
  char* receive_http_server(d_vm* vm, drax_value aconf) {
    UNUSED(vm);
    if(!aconf) { return 0; }
    int bffsz = 16384;
    char* buffer = (char*) malloc(bffsz * sizeof(char));

    dhttp_server* configs = (dhttp_server*) aconf;
    recv(configs->new_socket, buffer, bffsz, 0);
    char* nc = replace_special_char(buffer);
    free(buffer);

    return nc;
  }
#else
  char* receive_http_server(d_vm* vm, drax_value aconf) {
    UNUSED(vm);
    if(!aconf) { return 0; }
    int bffsz = 16384;
    char* buffer = (char*) malloc(bffsz * sizeof(char));

    dhttp_server* configs = (dhttp_server*) aconf;
    read(configs->new_socket, buffer, bffsz);
    char* nc = replace_special_char(buffer);
    free(buffer);

    return nc;
  }
#endif

#ifdef _WIN32
  ssize_t send_http_server(d_vm* vm, drax_value aconf, char* s) {
    UNUSED(vm);
    if(!aconf) { return 0; }
    dhttp_server* configs = (dhttp_server*) aconf;
    char* data = str_format_output(s);
    ssize_t r = send(configs->new_socket, data, strlen(data), 0 );
    return -1 != r ;
  }
#else
  ssize_t send_http_server(d_vm* vm, drax_value aconf, char* s) {
    UNUSED(vm);
    if(!aconf) { return 0; }

    dhttp_server* configs = (dhttp_server*) aconf;
    char* data = str_format_output(s);
    ssize_t r = write(configs->new_socket, data, strlen(data));
    return -1 != r ;
  }
#endif

#ifdef _WIN32
  int disconnect_client_http_server(d_vm* vm, drax_value v) {
    UNUSED(vm);
    if(!v) { return 0; }
    dhttp_server* configs = (dhttp_server*) v;
    closesocket(configs->new_socket);
    return 1;
  }
#else
  int disconnect_client_http_server(d_vm* vm, drax_value v) {
    UNUSED(vm);
    if(!v) { return 0; }
    dhttp_server* configs = (dhttp_server*) v;
    close((int) configs->new_socket);
    return 1;
  }
#endif

#ifdef _WIN32
  int stop_http_server(d_vm* vm, drax_value v) {
    UNUSED(vm);
    if(!v) { return 0; }
    dhttp_server* configs = (dhttp_server*) v;
    closesocket(configs->server_fd);
    WSACleanup();
    return 1;
}
#else
  int stop_http_server(d_vm* vm, drax_value v) {
    UNUSED(vm);
    if(!v) { return 0; }
    dhttp_server* configs = (dhttp_server*) v;
    close((int) configs->server_fd);
    return 1;
  }
#endif

