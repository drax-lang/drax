/* drax Lang - 2024 */

/* getaddrinfo, SO_REUSEPORT, signal: the build uses -std=c99 */
#ifndef _WIN32
  #define _POSIX_C_SOURCE 200112L
  #define _DEFAULT_SOURCE
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifndef _WIN32
  #include <signal.h>
#endif

#include "d_mod_http.h"
#include "d_http_parser.h"
#include "../dstring.h"

/* ------------------------------------------------------------------ */
/* handles                                                             */
/* ------------------------------------------------------------------ */

typedef struct dhttp_listener {
  int kind;               /* DHTTP_KIND_LISTENER, must be the first field */
  dsock_t fd;
  int timeout_ms;
  size_t max_header;
  size_t max_body;
} dhttp_listener;

typedef struct dhttp_conn {
  int kind;               /* DHTTP_KIND_CONN, must be the first field */
  dsock_t fd;
  size_t max_header;
  size_t max_body;
  char* buf;              /* bytes read and not consumed yet */
  size_t len;
  size_t cap;
  int broken;             /* the stream is out of sync: no more requests */
  int sent_continue;
} dhttp_conn;

#define READ_CHUNK 16384

/* ------------------------------------------------------------------ */
/* portability: the same code for POSIX and Windows sockets             */
/* ------------------------------------------------------------------ */

#ifdef _WIN32
  #define D_CLOSE(fd) closesocket(fd)
  #define D_LAST_ERROR() WSAGetLastError()
  #define D_IS_EINTR(e) ((e) == WSAEINTR)
  #define D_IS_ABORT(e) ((e) == WSAECONNABORTED || (e) == WSAECONNRESET)
  #define D_SEND_FLAGS 0

  static void d_error_text(int e, char* out, size_t n) { snprintf(out, n, "error %d", e); }

  static void d_init_sockets(void) {
    static int ready = 0;
    WSADATA wsa;
    if (!ready) { WSAStartup(MAKEWORD(2, 2), &wsa); ready = 1; }
  }

  static void d_set_timeouts(dsock_t fd, int ms) {
    DWORD t = (DWORD) ms;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &t, sizeof(t));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*) &t, sizeof(t));
  }
#else
  #define D_CLOSE(fd) close(fd)
  #define D_LAST_ERROR() errno
  #define D_IS_EINTR(e) ((e) == EINTR)
  #define D_IS_ABORT(e) ((e) == ECONNABORTED || (e) == ECONNRESET || (e) == EPROTO)
  #ifdef MSG_NOSIGNAL
    #define D_SEND_FLAGS MSG_NOSIGNAL
  #else
    #define D_SEND_FLAGS 0
  #endif

  static void d_error_text(int e, char* out, size_t n) { snprintf(out, n, "%s", strerror(e)); }

  static void d_init_sockets(void) {
    /* a write on a closed connection must be an error, not a signal that
     * kills the process */
    signal(SIGPIPE, SIG_IGN);
  }

  static void d_set_timeouts(dsock_t fd, int ms) {
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
  }
#endif

static drax_value make_error(d_vm* vm, const char* what, int err) {
  char reason[128];
  char msg[320];
  if (err != 0) {
    d_error_text(err, reason, sizeof(reason));
    snprintf(msg, sizeof(msg), "%s (%s)", what, reason);
  } else {
    snprintf(msg, sizeof(msg), "%s", what);
  }
  return DS_VAL(new_derror(vm, msg));
}

/* ------------------------------------------------------------------ */
/* listener                                                            */
/* ------------------------------------------------------------------ */

static int any_host(const char* h) {
  return h == NULL || h[0] == 0 || strcmp(h, "*") == 0 || strcmp(h, "0.0.0.0") == 0;
}

static int valid_port(const char* p) {
  size_t i, n = p ? strlen(p) : 0;
  long v = 0;
  if (n == 0 || n > 5) return 0;
  for (i = 0; i < n; i++) {
    if (p[i] < '0' || p[i] > '9') return 0;
    v = v * 10 + (p[i] - '0');
  }
  return v <= 65535;
}

drax_value dhttp_listen(d_vm* vm, const dhttp_opts* o, int* fail) {
  d_init_sockets();
  *fail = 1;

  if (!valid_port(o->port)) {
    return make_error(vm, "server error, invalid port!", 0);
  }

  struct addrinfo hints, *res = NULL;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int gai = getaddrinfo(any_host(o->host) ? NULL : o->host, o->port, &hints, &res);
  if (gai != 0 || res == NULL) {
    return make_error(vm, "server error, unsupported host!", 0);
  }

  dsock_t fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd == DSOCK_INVALID) {
    int e = D_LAST_ERROR();
    freeaddrinfo(res);
    return make_error(vm, "server error, fail to create the socket!", e);
  }

  int on = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) != 0) {
    int e = D_LAST_ERROR();
    D_CLOSE(fd);
    freeaddrinfo(res);
    return make_error(vm, "server error, fail to configure the socket!", e);
  }

  if (o->reuse_port) {
#if defined(SO_REUSEPORT)
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const char*) &on, sizeof(on)) != 0) {
      int e = D_LAST_ERROR();
      D_CLOSE(fd);
      freeaddrinfo(res);
      return make_error(vm, "server error, fail to enable reuse_port!", e);
    }
#else
    D_CLOSE(fd);
    freeaddrinfo(res);
    return make_error(vm, "server error, reuse_port is not supported on this system!", 0);
#endif
  }

  if (bind(fd, res->ai_addr, (int) res->ai_addrlen) != 0) {
    int e = D_LAST_ERROR();
    D_CLOSE(fd);
    freeaddrinfo(res);
    return make_error(vm, "server error, fail to bind!", e);
  }
  freeaddrinfo(res);

  if (listen(fd, o->backlog > 0 ? o->backlog : SOMAXCONN) != 0) {
    int e = D_LAST_ERROR();
    D_CLOSE(fd);
    return make_error(vm, "server error, fail to listen!", e);
  }

  dhttp_listener* l = (dhttp_listener*) malloc(sizeof(dhttp_listener));
  l->kind = DHTTP_KIND_LISTENER;
  l->fd = fd;
  l->timeout_ms = o->timeout_ms;
  l->max_header = o->max_header;
  l->max_body = o->max_body;

  *fail = 0;
  return (drax_value) l;
}

int dhttp_is_listener(drax_value v) {
  return v != 0 && ((dhttp_listener*) v)->kind == DHTTP_KIND_LISTENER;
}

int dhttp_is_conn(drax_value v) {
  return v != 0 && ((dhttp_conn*) v)->kind == DHTTP_KIND_CONN;
}

int dhttp_stop(drax_value v) {
  if (!dhttp_is_listener(v)) return 0;
  dhttp_listener* l = (dhttp_listener*) v;
  if (l->fd != DSOCK_INVALID) D_CLOSE(l->fd);
  l->fd = DSOCK_INVALID;
  return 1;
}

/* ------------------------------------------------------------------ */
/* connections                                                         */
/* ------------------------------------------------------------------ */

int dhttp_accept(d_vm* vm, drax_value listener, drax_value* out) {
  if (!dhttp_is_listener(listener)) {
    *out = make_error(vm, "server error, invalid server!", 0);
    return 0;
  }

  dhttp_listener* l = (dhttp_listener*) listener;
  if (l->fd == DSOCK_INVALID) {
    *out = make_error(vm, "server error, the server is closed!", 0);
    return 0;
  }

  dsock_t fd;
  for (;;) {
    fd = accept(l->fd, NULL, NULL);
    if (fd != DSOCK_INVALID) break;

    int e = D_LAST_ERROR();
    /* a signal, or a client that gave up before we got it: try the next one */
    if (D_IS_EINTR(e) || D_IS_ABORT(e)) continue;

    /* out of descriptors, closed by another handle, ...: the listener is
     * kept open, the caller decides what to do */
    *out = make_error(vm, "server error, fail to accept connection!", e);
    return 0;
  }

  int on = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*) &on, sizeof(on));
  if (l->timeout_ms > 0) d_set_timeouts(fd, l->timeout_ms);

  dhttp_conn* c = (dhttp_conn*) calloc(1, sizeof(dhttp_conn));
  c->kind = DHTTP_KIND_CONN;
  c->fd = fd;
  c->max_header = l->max_header;
  c->max_body = l->max_body;

  *out = (drax_value) c;
  return 1;
}

/* After answering an invalid request the client may still be sending (a big
 * body that was refused). Closing right away makes the TCP stack reset the
 * connection, and the client can lose the answer. So the sending side is
 * closed first and what still arrives is read and dropped, with limits. */
static void conn_linger(dhttp_conn* c) {
  char tmp[4096];
  size_t drained = 0;

#ifdef _WIN32
  shutdown(c->fd, SD_SEND);
#else
  shutdown(c->fd, SHUT_WR);
#endif
  d_set_timeouts(c->fd, 100);

  while (drained < 262144) {
#ifdef _WIN32
    int r = recv(c->fd, tmp, (int) sizeof(tmp), 0);
#else
    ssize_t r = recv(c->fd, tmp, sizeof(tmp), 0);
#endif
    if (r < 0 && D_IS_EINTR(D_LAST_ERROR())) continue;
    if (r <= 0) break;
    drained += (size_t) r;
  }
}

int dhttp_disconnect(drax_value v) {
  if (!dhttp_is_conn(v)) return 0;
  dhttp_conn* c = (dhttp_conn*) v;
  if (c->fd != DSOCK_INVALID && c->broken) conn_linger(c);
  if (c->fd != DSOCK_INVALID) D_CLOSE(c->fd);
  c->fd = DSOCK_INVALID;
  c->len = 0;
  return 1;
}

void dhttp_destroy(drax_value v) {
  if (v == 0) return;

  if (dhttp_is_listener(v)) {
    dhttp_listener* l = (dhttp_listener*) v;
    if (l->fd != DSOCK_INVALID) D_CLOSE(l->fd);
    free(l);
    return;
  }

  if (dhttp_is_conn(v)) {
    dhttp_conn* c = (dhttp_conn*) v;
    if (c->fd != DSOCK_INVALID) D_CLOSE(c->fd);
    free(c->buf);
    free(c);
  }
}

/* one read, retrying on signals. <= 0: closed, timeout or error */
static long conn_read(dhttp_conn* c, char* dst, size_t n) {
  if (c->fd == DSOCK_INVALID) return -1;
  for (;;) {
#ifdef _WIN32
    int r = recv(c->fd, dst, (int) n, 0);
#else
    ssize_t r = recv(c->fd, dst, n, 0);
#endif
    if (r < 0 && D_IS_EINTR(D_LAST_ERROR())) continue;
    return (long) r;
  }
}

static int conn_reserve(dhttp_conn* c, size_t want) {
  if (want <= c->cap) return 1;
  size_t cap = c->cap ? c->cap : 4096;
  while (cap < want) cap *= 2;
  char* nb = (char*) realloc(c->buf, cap);
  if (nb == NULL) return 0;
  c->buf = nb;
  c->cap = cap;
  return 1;
}

/* the string takes the ownership of a malloc'd copy of the bytes */
static drax_value make_str(d_vm* vm, const char* p, size_t n) {
  char* s = (char*) malloc(n + 1);
  if (n > 0) memcpy(s, p, n);
  s[n] = '\0';
  return DS_VAL(new_dstring(vm, s, (int) n));
}

drax_value dhttp_recv(d_vm* vm, drax_value v) {
  if (!dhttp_is_conn(v)) return DRAX_NIL_VAL;
  dhttp_conn* c = (dhttp_conn*) v;

  /* bytes that a request read already got from the socket come first */
  if (c->len > 0) {
    drax_value s = make_str(vm, c->buf, c->len);
    c->len = 0;
    return s;
  }

  char tmp[READ_CHUNK];
  long n = conn_read(c, tmp, sizeof(tmp));
  if (n <= 0) return DRAX_NIL_VAL;
  return make_str(vm, tmp, (size_t) n);
}

int dhttp_send(drax_value v, const char* data, size_t len) {
  if (!dhttp_is_conn(v)) return 0;
  dhttp_conn* c = (dhttp_conn*) v;
  if (c->fd == DSOCK_INVALID) return 0;

  size_t sent = 0;
  while (sent < len) {
#ifdef _WIN32
    int r = send(c->fd, data + sent, (int) (len - sent), D_SEND_FLAGS);
#else
    ssize_t r = send(c->fd, data + sent, len - sent, D_SEND_FLAGS);
#endif
    if (r < 0 && D_IS_EINTR(D_LAST_ERROR())) continue;
    if (r <= 0) return 0;
    sent += (size_t) r;
  }
  return 1;
}

/* ------------------------------------------------------------------ */
/* requests                                                            */
/* ------------------------------------------------------------------ */

static drax_value error_frame(d_vm* vm, dhp_result r) {
  drax_frame* f = new_dframe(vm, 2);
  put_value_dframe(f, (char*) "error", make_str(vm, dhttp_result_name(r), strlen(dhttp_result_name(r))));
  put_value_dframe(f, (char*) "status", NUMBER_VAL((double) dhttp_result_status(r)));
  return DS_VAL(f);
}

static drax_value build_request(d_vm* vm, const dhttp_head* h, const char* body) {
  int i;
  drax_frame* f = new_dframe(vm, 8);
  drax_frame* hs = new_dframe(vm, h->nheaders > 0 ? h->nheaders : 1);

  for (i = 0; i < h->nheaders; i++) {
    size_t k, nn = h->name[i].n;
    char* name = (char*) malloc(nn + 1);
    for (k = 0; k < nn; k++) name[k] = (char) tolower((unsigned char) h->name[i].p[k]);
    name[nn] = '\0';

    drax_value prev;
    if (get_value_dframe(hs, name, &prev) != -1 && IS_STRING(prev)) {
      /* the same header again: values are joined with a comma (RFC 9110) */
      drax_string* ps = CAST_STRING(prev);
      size_t total = (size_t) ps->length + 2 + h->value[i].n;
      char* j = (char*) malloc(total + 1);
      memcpy(j, ps->chars, (size_t) ps->length);
      memcpy(j + ps->length, ", ", 2);
      memcpy(j + ps->length + 2, h->value[i].p, h->value[i].n);
      j[total] = '\0';
      put_value_dframe(hs, name, DS_VAL(new_dstring(vm, j, (int) total)));
    } else {
      put_value_dframe(hs, name, make_str(vm, h->value[i].p, h->value[i].n));
    }
    free(name);
  }

  put_value_dframe(f, (char*) "method", make_str(vm, h->method.p, h->method.n));
  put_value_dframe(f, (char*) "path", make_str(vm, h->path.p, h->path.n));
  put_value_dframe(f, (char*) "query", make_str(vm, h->query.p, h->query.n));
  put_value_dframe(f, (char*) "version", make_str(vm, h->minor ? "1.1" : "1.0", 3));
  put_value_dframe(f, (char*) "headers", DS_VAL(hs));
  put_value_dframe(f, (char*) "body", make_str(vm, body, h->content_length));
  put_value_dframe(f, (char*) "keep_alive", h->keep_alive ? DRAX_TRUE_VAL : DRAX_FALSE_VAL);
  return DS_VAL(f);
}

drax_value dhttp_read_request(d_vm* vm, drax_value v) {
  if (!dhttp_is_conn(v)) return DRAX_NIL_VAL;
  dhttp_conn* c = (dhttp_conn*) v;
  if (c->broken || c->fd == DSOCK_INVALID) return DRAX_NIL_VAL;

  for (;;) {
    size_t need = 0; /* bytes the request needs in total, once the head is known */

    if (c->len > 0) {
      dhttp_head h;
      dhp_result r = dhttp_parse_head(c->buf, c->len, c->max_header, c->max_body, &h);

      if (r == DHP_OK) {
        need = h.head_len + h.content_length;

        if (c->len >= need) {
          drax_value req = build_request(vm, &h, c->buf + h.head_len);
          memmove(c->buf, c->buf + need, c->len - need);
          c->len -= need;
          c->sent_continue = 0;
          /* a big request must not keep a big buffer for the whole connection */
          if (c->len == 0 && c->cap > 65536) {
            free(c->buf);
            c->buf = NULL;
            c->cap = 0;
          }
          return req;
        }

        /* "Expect: 100-continue": the client waits before sending the body */
        if (h.expect_continue && !c->sent_continue) {
          c->sent_continue = 1;
          dhttp_send(v, "HTTP/1.1 100 Continue\r\n\r\n", 25);
        }
      } else if (r != DHP_INCOMPLETE) {
        /* the stream can not be trusted after this: answer and close */
        c->broken = 1;
        c->len = 0;
        return error_frame(vm, r);
      }
    }

    size_t want = c->len + READ_CHUNK;
    if (need > want) want = need;
    if (!conn_reserve(c, want)) return DRAX_NIL_VAL;

    long n = conn_read(c, c->buf + c->len, c->cap - c->len);
    if (n <= 0) return DRAX_NIL_VAL; /* closed, timeout or error */
    c->len += (size_t) n;
  }
}

/* ------------------------------------------------------------------ */
/* responses                                                           */
/* ------------------------------------------------------------------ */

typedef struct strbuf { char* p; size_t n, cap; } strbuf;

static void sb_add(strbuf* b, const char* s, size_t n) {
  if (b->n + n + 1 > b->cap) {
    size_t cap = b->cap ? b->cap : 256;
    while (cap < b->n + n + 1) cap *= 2;
    b->p = (char*) realloc(b->p, cap);
    b->cap = cap;
  }
  memcpy(b->p + b->n, s, n);
  b->n += n;
  b->p[b->n] = '\0';
}

static int header_name_ok(const char* s) {
  size_t i, n = strlen(s);
  if (n == 0) return 0;
  for (i = 0; i < n; i++) {
    unsigned char c = (unsigned char) s[i];
    if (!(isalnum(c) || strchr("!#$%&'*+-.^_`|~", c))) return 0;
  }
  return 1;
}

static int ieq_name(const char* a, const char* b) {
  for (; *a && *b; a++, b++) {
    if (tolower((unsigned char) *a) != tolower((unsigned char) *b)) return 0;
  }
  return *a == *b;
}

drax_value dhttp_response(d_vm* vm, drax_value status, drax_value headers,
                          drax_value body, int* fail) {
  *fail = 1;

  if (!IS_NUMBER(status)) return make_error(vm, "Http.response: the status must be a number", 0);
  double dst = CAST_NUMBER(status);
  int code = (int) dst;
  if ((double) code != dst || code < 100 || code > 599) {
    return make_error(vm, "Http.response: invalid status code", 0);
  }

  if (!IS_FRAME(headers)) return make_error(vm, "Http.response: the headers must be a frame", 0);
  if (!(IS_STRING(body) || IS_NIL(body))) {
    return make_error(vm, "Http.response: the body must be a string or nil", 0);
  }

  const char* bytes = "";
  size_t blen = 0;
  if (IS_STRING(body)) {
    bytes = CAST_STRING(body)->chars;
    blen = (size_t) CAST_STRING(body)->length;
  }

  /* these statuses have no body, and no Content-Length either */
  int no_body = (code >= 100 && code < 200) || code == 204 || code == 304;
  if (no_body && blen > 0) {
    return make_error(vm, "Http.response: this status can not have a body", 0);
  }

  strbuf b = { NULL, 0, 0 };
  char line[96];
  int n = snprintf(line, sizeof(line), "HTTP/1.1 %d %s\r\n", code, dhttp_reason(code));
  sb_add(&b, line, (size_t) n);

  drax_frame* hf = CAST_FRAME(headers);
  int i;
  for (i = 0; i < hf->length; i++) {
    const char* name = hf->literals[i];
    drax_value val = hf->values[i];
    char num[64];
    const char* vp;
    size_t vn;

    if (!header_name_ok(name)) {
      free(b.p);
      return make_error(vm, "Http.response: invalid header name", 0);
    }
    /* the length is always computed from the body */
    if (ieq_name(name, "content-length")) continue;

    if (IS_STRING(val)) {
      vp = CAST_STRING(val)->chars;
      vn = (size_t) CAST_STRING(val)->length;
    } else if (IS_NUMBER(val)) {
      snprintf(num, sizeof(num), "%.15g", CAST_NUMBER(val));
      vp = num;
      vn = strlen(num);
    } else {
      free(b.p);
      return make_error(vm, "Http.response: a header value must be a string or a number", 0);
    }

    /* CR, LF and NUL would split the response (header injection) */
    if (memchr(vp, '\r', vn) || memchr(vp, '\n', vn) || memchr(vp, '\0', vn)) {
      free(b.p);
      return make_error(vm, "Http.response: invalid header value", 0);
    }

    sb_add(&b, name, strlen(name));
    sb_add(&b, ": ", 2);
    sb_add(&b, vp, vn);
    sb_add(&b, "\r\n", 2);
  }

  if (!no_body) {
    n = snprintf(line, sizeof(line), "Content-Length: %lu\r\n", (unsigned long) blen);
    sb_add(&b, line, (size_t) n);
  }
  sb_add(&b, "\r\n", 2);
  sb_add(&b, bytes, blen);

  /* the string takes the ownership of the buffer */
  *fail = 0;
  return DS_VAL(new_dstring(vm, b.p, (int) b.n));
}
