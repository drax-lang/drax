#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../src/mods/d_http_parser.h"

static int failures = 0;

#define HEAD_MAX 16384
#define BODY_MAX 1048576

static void expect(const char* name, const char* req, dhp_result want) {
  dhttp_head h;
  dhp_result got = dhttp_parse_head(req, strlen(req), HEAD_MAX, BODY_MAX, &h);
  printf("%-46s -> ", name);
  if (got != want) {
    printf("[FAIL] expected %d, got %d\n", want, got);
    failures++;
    return;
  }
  printf("[PASS]\n");
}

/* requests with NUL bytes need an explicit length */
static void expect_n(const char* name, const char* req, size_t n, dhp_result want) {
  dhttp_head h;
  dhp_result got = dhttp_parse_head(req, n, HEAD_MAX, BODY_MAX, &h);
  printf("%-46s -> ", name);
  if (got != want) { printf("[FAIL] expected %d, got %d\n", want, got); failures++; return; }
  printf("[PASS]\n");
}

static void check(const char* name, int cond) {
  printf("%-46s -> ", name);
  if (!cond) { printf("[FAIL]\n"); failures++; return; }
  printf("[PASS]\n");
}

int main(void) {
  dhttp_head h;
  const char* r;

  printf("--- http parser: valid requests ---\n");
  r = "GET /a/b?x=1&y=2 HTTP/1.1\r\nHost: x\r\n\r\n";
  expect("simple get", r, DHP_OK);
  dhttp_parse_head(r, strlen(r), HEAD_MAX, BODY_MAX, &h);
  check("method", h.method.n == 3 && memcmp(h.method.p, "GET", 3) == 0);
  check("path without query", h.path.n == 4 && memcmp(h.path.p, "/a/b", 4) == 0);
  check("query", h.query.n == 7 && memcmp(h.query.p, "x=1&y=2", 7) == 0);
  check("head_len", h.head_len == strlen(r));
  check("http/1.1 keeps alive by default", h.keep_alive == 1 && h.minor == 1);

  r = "GET / HTTP/1.0\r\n\r\n";
  expect("http/1.0 without headers", r, DHP_OK);
  dhttp_parse_head(r, strlen(r), HEAD_MAX, BODY_MAX, &h);
  check("http/1.0 closes by default", h.keep_alive == 0 && h.minor == 0);

  r = "GET / HTTP/1.0\r\nConnection: keep-alive\r\n\r\n";
  dhttp_parse_head(r, strlen(r), HEAD_MAX, BODY_MAX, &h);
  check("http/1.0 keep-alive", h.keep_alive == 1);

  r = "GET / HTTP/1.1\r\nConnection: Upgrade, CLOSE\r\n\r\n";
  dhttp_parse_head(r, strlen(r), HEAD_MAX, BODY_MAX, &h);
  check("connection: close (any case, in a list)", h.keep_alive == 0);

  r = "POST /p HTTP/1.1\r\ncontent-LENGTH:   42  \r\nExpect: 100-continue\r\n\r\n";
  expect("body announced", r, DHP_OK);
  dhttp_parse_head(r, strlen(r), HEAD_MAX, BODY_MAX, &h);
  check("content-length", h.content_length == 42);
  check("expect 100-continue", h.expect_continue == 1);
  check("header value is trimmed", h.value[0].n == 2);

  expect("empty lines before the request line", "\r\n\r\nGET / HTTP/1.1\r\n\r\n", DHP_OK);
  expect("equal repeated content-length",
         "POST / HTTP/1.1\r\nContent-Length: 3\r\nContent-Length: 3\r\n\r\n", DHP_OK);
  expect("options *", "OPTIONS * HTTP/1.1\r\n\r\n", DHP_OK);

  printf("--- http parser: incomplete ---\n");
  expect("empty", "", DHP_INCOMPLETE);
  expect("half of the request line", "GET /a HT", DHP_INCOMPLETE);
  expect("no blank line yet", "GET / HTTP/1.1\r\nHost: x\r\n", DHP_INCOMPLETE);
  expect("one CR missing", "GET / HTTP/1.1\r\n\r", DHP_INCOMPLETE);

  printf("--- http parser: malformed ---\n");
  expect("garbage", "GARBAGE\r\n\r\n", DHP_BAD_REQUEST);
  expect("no version", "GET /\r\n\r\n", DHP_BAD_REQUEST);
  expect("http/2", "GET / HTTP/2.0\r\n\r\n", DHP_BAD_REQUEST);
  expect("http/1.2", "GET / HTTP/1.2\r\n\r\n", DHP_BAD_REQUEST);
  expect("lower case version", "GET / http/1.1\r\n\r\n", DHP_BAD_REQUEST);
  expect("two spaces", "GET  / HTTP/1.1\r\n\r\n", DHP_BAD_REQUEST);
  expect("space in the target", "GET /a b HTTP/1.1\r\n\r\n", DHP_BAD_REQUEST);
  expect("control char in the target", "GET /a\x01 HTTP/1.1\r\n\r\n", DHP_BAD_REQUEST);
  expect("bad method char", "G(T / HTTP/1.1\r\n\r\n", DHP_BAD_REQUEST);
  expect("bare LF (no need to wait)", "GET / HTTP/1.1\nHost: x\n\n", DHP_BAD_REQUEST);
  expect("bare LF inside the head", "GET / HTTP/1.1\r\nHost: x\nFoo: y\r\n\r\n", DHP_BAD_REQUEST);
  expect("bare CR inside the head", "GET / HTTP/1.1\r\nHost: x\rFoo: y\r\n\r\n", DHP_BAD_REQUEST);
  {
    static const char nul_done[] = "GET / HTTP/1.1\r\nHost: x\0y\r\n\r\n";
    static const char nul_open[] = "GET / HTTP/1.1\r\nHost: x\0y";
    expect_n("NUL in the head", nul_done, sizeof(nul_done) - 1, DHP_BAD_REQUEST);
    expect_n("NUL before the end (no need to wait)", nul_open, sizeof(nul_open) - 1, DHP_BAD_REQUEST);
  }
  expect("no colon", "GET / HTTP/1.1\r\nHost x\r\n\r\n", DHP_BAD_REQUEST);
  expect("space before the colon", "GET / HTTP/1.1\r\nHost : x\r\n\r\n", DHP_BAD_REQUEST);
  expect("empty header name", "GET / HTTP/1.1\r\n: x\r\n\r\n", DHP_BAD_REQUEST);
  expect("obsolete line folding", "GET / HTTP/1.1\r\nFoo: a\r\n b\r\n\r\n", DHP_BAD_REQUEST);

  printf("--- http parser: request smuggling ---\n");
  expect("content-length with letters", "POST / HTTP/1.1\r\nContent-Length: 1a\r\n\r\n", DHP_BAD_REQUEST);
  expect("content-length negative", "POST / HTTP/1.1\r\nContent-Length: -1\r\n\r\n", DHP_BAD_REQUEST);
  expect("content-length empty", "POST / HTTP/1.1\r\nContent-Length:\r\n\r\n", DHP_BAD_REQUEST);
  expect("content-length overflow", "POST / HTTP/1.1\r\nContent-Length: 99999999999999999999\r\n\r\n", DHP_BAD_REQUEST);
  expect("different content-length values",
         "POST / HTTP/1.1\r\nContent-Length: 3\r\nContent-Length: 4\r\n\r\n", DHP_BAD_REQUEST);
  expect("transfer-encoding", "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n", DHP_UNSUPPORTED);
  expect("transfer-encoding + content-length",
         "POST / HTTP/1.1\r\nContent-Length: 3\r\nTransfer-Encoding: chunked\r\n\r\n", DHP_UNSUPPORTED);

  printf("--- http parser: limits ---\n");
  expect("body too large", "POST / HTTP/1.1\r\nContent-Length: 1048577\r\n\r\n", DHP_BODY_TOO_LARGE);
  expect("body at the limit", "POST / HTTP/1.1\r\nContent-Length: 1048576\r\n\r\n", DHP_OK);

  {
    /* a head bigger than the limit, with and without the blank line */
    size_t n = HEAD_MAX + 100;
    char* big = (char*) malloc(n + 64);
    strcpy(big, "GET / HTTP/1.1\r\nX: ");
    size_t l = strlen(big);
    memset(big + l, 'a', n);
    big[l + n] = '\0';
    expect("head too large, no end yet", big, DHP_HEADER_TOO_LARGE);
    strcat(big, "\r\n\r\n");
    expect("head too large, with the end", big, DHP_HEADER_TOO_LARGE);
    free(big);
  }

  {
    char* many = (char*) malloc(DHP_MAX_HEADERS * 16 + 128);
    strcpy(many, "GET / HTTP/1.1\r\n");
    int i;
    for (i = 0; i < DHP_MAX_HEADERS + 1; i++) {
      char line[32];
      snprintf(line, sizeof(line), "H%d: v\r\n", i);
      strcat(many, line);
    }
    strcat(many, "\r\n");
    expect("too many headers", many, DHP_HEADER_TOO_LARGE);
    free(many);
  }

  printf("--- http parser: helpers ---\n");
  check("status of bad request", dhttp_result_status(DHP_BAD_REQUEST) == 400);
  check("status of header too large", dhttp_result_status(DHP_HEADER_TOO_LARGE) == 431);
  check("status of body too large", dhttp_result_status(DHP_BODY_TOO_LARGE) == 413);
  check("status of unsupported", dhttp_result_status(DHP_UNSUPPORTED) == 501);
  check("reason 200", strcmp(dhttp_reason(200), "OK") == 0);
  check("reason 404", strcmp(dhttp_reason(404), "Not Found") == 0);

  if (failures > 0) {
    printf("\n%d http parser test(s) failed\n", failures);
    return 1;
  }
  printf("\nAll http parser tests passed!\n");
  return 0;
}
