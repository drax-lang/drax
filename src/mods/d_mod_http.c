#include "d_mod_http.h"
#include "../dstring.h"

#include "../http/include/civetweb.h"
#include <string.h>

static int request_handler(struct mg_connection *conn, void *cbdata) {
    const char *response = "Hello, World!";
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
    mg_printf(conn, "%s", response);
    return 200;
}

drax_value start_http_server(
  d_vm* vm,
  char *options[],
  void (*callback_caller)(d_vm* vm, drax_value call),
  drax_value call
) {
    struct mg_callbacks callbacks;
    struct mg_context *ctx;

    /*
     * Initialize callbacks
     */
    memset(&callbacks, 0, sizeof(callbacks));

    /*
     * Start the server
     */
    ctx = mg_start(&callbacks, NULL, (const char**) options);

    /*
     * Add a URI handler
    mg_set_request_handler(ctx, "/", request_handler, NULL);
     */

    callback_caller(vm, call);
    return ctx;
}

void stop_http_server(drax_value v) {
    struct mg_context *ctx = (struct mg_context *) v;
    mg_stop(ctx);
}

