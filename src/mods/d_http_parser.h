/* drax Lang - 2026
 *
 * HTTP/1.x request head parser. It does not touch the network and does not
 * know the VM: it reads a byte buffer and reports slices of it, so it can be
 * tested alone (tests/c_tests/tests_http_parser.c).
 *
 * It is strict on purpose (a server that is lenient with malformed input is
 * the base of request smuggling):
 *   - lines end with CRLF (a bare LF or CR is a bad request)
 *   - no NUL byte in the head
 *   - no obsolete line folding
 *   - Content-Length is only digits, repeated values must be equal
 *   - Transfer-Encoding is not supported (answered with 501)
 *   - the head and the body have size limits
 */
#ifndef __D_HTTP_PARSER
#define __D_HTTP_PARSER

#include <stddef.h>

#define DHP_MAX_HEADERS 100
#define DHP_MAX_TARGET  8192
#define DHP_MAX_METHOD  16

typedef struct dhp_slice {
  const char* p;
  size_t n;
} dhp_slice;

typedef enum dhp_result {
  DHP_OK = 0,
  DHP_INCOMPLETE,        /* needs more bytes */
  DHP_BAD_REQUEST,       /* 400 */
  DHP_HEADER_TOO_LARGE,  /* 431 */
  DHP_BODY_TOO_LARGE,    /* 413 */
  DHP_UNSUPPORTED        /* 501 */
} dhp_result;

typedef struct dhttp_head {
  dhp_slice method;
  dhp_slice target;      /* as received */
  dhp_slice path;        /* target without the query */
  dhp_slice query;       /* after '?', empty when there is none */
  int minor;             /* HTTP/1.minor */
  int nheaders;
  dhp_slice name[DHP_MAX_HEADERS];
  dhp_slice value[DHP_MAX_HEADERS];
  size_t head_len;       /* request line + headers + the blank line */
  size_t content_length;
  int keep_alive;
  int expect_continue;
} dhttp_head;

/**
 * Parses the head of a request that starts at buf.
 * max_header: limit for head_len. max_body: limit for content_length.
 * DHP_OK fills h (the slices point into buf).
 */
dhp_result dhttp_parse_head(const char* buf, size_t len, size_t max_header,
                            size_t max_body, dhttp_head* h);

/* status code and short name of an error result */
int dhttp_result_status(dhp_result r);
const char* dhttp_result_name(dhp_result r);

/* reason phrase of a status code ("OK", "Not Found", ...) */
const char* dhttp_reason(int status);

#endif
