/* drax Lang - 2024
 * Jean Carlos (jeantux)
 */

#ifndef __D_MOD_HTTP
#define __D_MOD_HTTP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
  #include <windows.h>
  #undef _WIN32_WINNT
  #define _WIN32_WINNT 0x0501
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET dsock_t;
  #define DSOCK_INVALID INVALID_SOCKET
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/time.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  typedef int dsock_t;
  #define DSOCK_INVALID (-1)
#endif

#include "../dvm.h"
#include "../dstructs.h"

/**
 * Two kinds of native handles live inside a tid:
 *
 *   listener  TCPServer.new(...)       accepts connections
 *   conn      TCPServer.accept(srv)    one client, with its own socket
 *
 * Each connection owns its socket and its read buffer, so many connections
 * can be open at the same time. The functions of the language are blocking
 * and sequential: accept, read, answer, repeat.
 */
#define DHTTP_KIND_LISTENER 1
#define DHTTP_KIND_CONN     2

#define DHTTP_DEFAULT_TIMEOUT_MS 10000
#define DHTTP_DEFAULT_MAX_HEADER 16384            /* 16 KB */
#define DHTTP_DEFAULT_MAX_BODY   (1024 * 1024)    /* 1 MB */

typedef struct dhttp_opts {
  const char* host;       /* NULL, "", "*" or "0.0.0.0": every interface */
  const char* port;       /* "0".."65535" */
  int backlog;            /* <= 0: SOMAXCONN */
  int reuse_port;         /* SO_REUSEPORT: several processes on the same port */
  int timeout_ms;         /* read/write timeout of each connection, 0: none */
  size_t max_header;
  size_t max_body;
} dhttp_opts;

/* starts a listener. fail = 1: the result is an error value (a drax_error). */
drax_value dhttp_listen(d_vm* vm, const dhttp_opts* o, int* fail);

/* closes the listening socket (the handle is released by the GC) */
int dhttp_stop(drax_value listener);

/* waits for a client. 1: *out is the new connection (native pointer),
 * 0: *out is an error value. The listener stays open on errors. */
int dhttp_accept(d_vm* vm, drax_value listener, drax_value* out);

/* closes the connection (the handle is released by the GC) */
int dhttp_disconnect(drax_value conn);

/* bytes that are available (a single read), a string, or nil on end/timeout */
drax_value dhttp_recv(d_vm* vm, drax_value conn);

/* writes everything. 1: ok, 0: the client is gone or too slow */
int dhttp_send(drax_value conn, const char* data, size_t len);

/**
 * Reads one complete request (head and body) and returns a frame:
 *   method, path, query, version, headers (lower case names), body, keep_alive
 * An invalid request returns {error: "...", status: 400|413|431|501}: answer
 * it and close. nil: the client closed the connection or timed out.
 */
drax_value dhttp_read_request(d_vm* vm, drax_value conn);

/* builds "HTTP/1.1 <status> ...\r\n<headers>Content-Length\r\n\r\n<body>".
 * fail = 1: the result is an error value. */
drax_value dhttp_response(d_vm* vm, drax_value status, drax_value headers,
                          drax_value body, int* fail);

/* is the tid value a listener / a connection of this module */
int dhttp_is_listener(drax_value tid_value);
int dhttp_is_conn(drax_value tid_value);

/* releases a handle (listener or connection): used as tid->destroy */
void dhttp_destroy(drax_value v);

#endif
