#include <string.h>
#include <stdlib.h>

#include "d_mod_http.h"
#include "../dstring.h"
#include "../http/include/civetweb.h"

static int request_handler(struct mg_connection* conn, void* cbdata) {
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");

  drax_value v = (drax_value) cbdata;
  int status_code_int = 200;
  
  if (IS_FUNCTION(v)) {
    drax_value f = run_instruction_on_vm_pool(v);

    if (IS_FRAME(f)) {
      drax_frame* ofr = CAST_FRAME(f);
      
      /**
       * get status as string
       */
      char* status_code = (char*) "200";
      drax_value status;
      if(get_value_dframe(ofr, (char*) "status_code", &status) != -1) {
        if (IS_STRING(status)) {
          status_code = CAST_STRING(status)->chars;
          double dres = atoi(status_code);
          status_code_int = status_code_int == 0 ? 200 : status_code_int;
        }
      }

      /**
       * get document_root as string
       */
      char* data_string = (char*) ".";
      drax_value data;
      if(get_value_dframe(ofr, (char*) "data", &data) != -1) {
        data_string = CAST_STRING(data)->chars;
        mg_printf(conn, "%s", (const char*) data_string);
      }

    }
  }

  return status_code_int;
}

drax_value start_http_server(
  d_vm* vm,
  char *options[],
  void (*callback_caller)(d_vm* vm, drax_value call),
  drax_value call,
  drax_value callback_req_handler
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
    */

    if ((callback_req_handler != 0) && (callback_req_handler != DRAX_NIL_VAL)) {
      mg_set_request_handler(ctx, "/", request_handler, (void*) callback_req_handler);
    }

  callback_caller(vm, call);
  return ctx;
}

void stop_http_server(drax_value v) {
  struct mg_context *ctx = (struct mg_context *) v;
  mg_stop(ctx);
}

