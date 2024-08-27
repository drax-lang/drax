#include <string.h>
#include <stdlib.h>

#include "d_mod_http.h"
#include "../dstring.h"
#include "../http/include/civetweb.h"


static int request_handler(struct mg_connection* cconn, void* cbdata) {

  /**
   * Get the request info
   */
  const struct mg_request_info *ri = mg_get_request_info(cconn);
  const char *request_method = ri->request_method;
  const char *local_uri = ri->local_uri;

  const char *local_uri_raw = ri->local_uri_raw;
  const char *http_version = ri->http_version;
  const char *query_string = ri->query_string;
  const char *remote_user = ri->remote_user;
  const char* remote_addr = ri->remote_addr;

  char content_length[20];
  snprintf(content_length, sizeof(content_length), "%lld", ri->content_length);

  char remote_port[12];
  snprintf(remote_port, sizeof(remote_port), "%d", ri->remote_port);

  char server_port[12];
  snprintf(server_port, sizeof(server_port), "%d", ri->server_port);

  char is_ssl[12];
  snprintf(is_ssl, sizeof(is_ssl), "%d", ri->is_ssl);

  drax_value v = (drax_value) cbdata;
  int status_code_int = 200;
  
  if (IS_FUNCTION(v)) {

    /**
     * the orphans frame only accept char*
     */
    drax_frame* request_info = new_dframe_orphan(14);
    put_value_dframe(request_info, (char*) "local_uri", (drax_value) local_uri);
    put_value_dframe(request_info, (char*) "request_method", (drax_value) request_method);

    put_value_dframe(request_info, (char*) "local_uri_raw", (drax_value) local_uri_raw);
    put_value_dframe(request_info, (char*) "http_version", (drax_value) http_version);
    put_value_dframe(request_info, (char*) "query_string", (drax_value) query_string);
    put_value_dframe(request_info, (char*) "remote_user", (drax_value) remote_user);
    put_value_dframe(request_info, (char*) "remote_addr", (drax_value) remote_addr);

    put_value_dframe(request_info, (char*) "content_length", (drax_value) content_length);

    put_value_dframe(request_info, (char*) "remote_port", (drax_value) remote_port);
    put_value_dframe(request_info, (char*) "server_port", (drax_value) server_port);
    put_value_dframe(request_info, (char*) "is_ssl", (drax_value) is_ssl);
    
    /**
     * Get the header in request info
     */

    drax_frame* dheaders = new_dframe_orphan(MG_MAX_HEADERS);

    size_t i;
    for (i = 0; i < MG_MAX_HEADERS; i++) {
      if (ri->http_headers[i].name != NULL) {
        const char* val = ri->http_headers[i].value;
        put_value_dframe(dheaders, (char*) ri->http_headers[i].name, (drax_value) val);
      }
    }
    put_value_dframe(request_info, (char*) "headers", DS_VAL(dheaders));

    drax_frame* conn = new_dframe_orphan(4);
    put_value_dframe(conn, (char*) "request_info", DS_VAL(request_info));

    drax_value f = run_instruction_on_vm_pool(v, DS_VAL(conn));

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
          status_code_int = atoi(status_code);
          status_code_int = status_code_int <= 0 ? 200 : status_code_int;
        }
      }

      /**
       * get headers as string
       */
      char* header_string;
      drax_value header;
      if(get_value_dframe(ofr, (char*) "headers", &header) != -1) {
        header_string = str_format_output(CAST_STRING(header)->chars);
        mg_printf(cconn, "%s", (const char*) header_string);
      }

      /**
       * get data as string
       */
      char* data_string = (char*) ".";
      drax_value data;
      if(get_value_dframe(ofr, (char*) "data", &data) != -1) {
        data_string = str_format_output(CAST_STRING(data)->chars);
        mg_printf(cconn, "%s", (const char*) data_string);
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
  return (drax_value) ctx;
}

void stop_http_server(drax_value v) {
  struct mg_context *ctx = (struct mg_context *) v;
  mg_stop(ctx);
}

