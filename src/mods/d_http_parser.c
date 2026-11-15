/* drax Lang - 2026 */

#include <string.h>

#include "d_http_parser.h"

/* RFC 9110 token characters */
static int is_tchar(unsigned char c) {
  if (c >= '0' && c <= '9') return 1;
  if (c >= 'a' && c <= 'z') return 1;
  if (c >= 'A' && c <= 'Z') return 1;
  return c != 0 && strchr("!#$%&'*+-.^_`|~", c) != NULL;
}

static unsigned char lower(unsigned char c) {
  return (c >= 'A' && c <= 'Z') ? (unsigned char) (c + 32) : c;
}

static int ieq(const char* s, size_t n, const char* lit) {
  size_t i, ln = strlen(lit);
  if (n != ln) return 0;
  for (i = 0; i < n; i++) if (lower((unsigned char) s[i]) != (unsigned char) lit[i]) return 0;
  return 1;
}

/* the token list of a header value (comma separated) has "tok" */
static int has_token(const char* v, size_t n, const char* tok) {
  size_t i = 0, tl = strlen(tok);
  while (i < n) {
    while (i < n && (v[i] == ' ' || v[i] == '\t' || v[i] == ',')) i++;
    size_t s = i;
    while (i < n && v[i] != ',' && v[i] != ' ' && v[i] != '\t') i++;
    if (i - s == tl && ieq(v + s, tl, tok)) return 1;
  }
  return 0;
}

/* position of the "\r\n\r\n" that ends the head, or (size_t) -1 */
static size_t find_end(const char* b, size_t n) {
  size_t i;
  if (n < 4) return (size_t) -1;
  for (i = 0; i + 3 < n; i++) {
    if (b[i] == '\r' && b[i + 1] == '\n' && b[i + 2] == '\r' && b[i + 3] == '\n') return i;
  }
  return (size_t) -1;
}

dhp_result dhttp_parse_head(const char* buf, size_t len, size_t max_header,
                            size_t max_body, dhttp_head* h) {
  size_t i, start = 0;

  memset(h, 0, sizeof(*h));

  /* empty lines before the request line are ignored (RFC 9112, 2.2) */
  while (start + 1 < len && buf[start] == '\r' && buf[start + 1] == '\n') start += 2;

  size_t end = find_end(buf + start, len - start);
  if (end == (size_t) -1) {
    /* no blank line yet: too big, or just not complete */
    if (len >= max_header) return DHP_HEADER_TOO_LARGE;
    /* a NUL, a bare LF or a bare CR already make it bad: no need to wait
     * for a blank line that may never come (a client that only sends LF) */
    for (i = start; i < len; i++) {
      if (buf[i] == '\0') return DHP_BAD_REQUEST;
      if (buf[i] == '\n' && (i == 0 || buf[i - 1] != '\r')) return DHP_BAD_REQUEST;
      if (buf[i] == '\r' && i + 1 < len && buf[i + 1] != '\n') return DHP_BAD_REQUEST;
    }
    return DHP_INCOMPLETE;
  }

  h->head_len = start + end + 4;
  if (h->head_len > max_header) return DHP_HEADER_TOO_LARGE;

  const char* b = buf + start;
  size_t blen = end + 2; /* up to and including the last header's CRLF */

  /* only CRLF pairs, no NUL */
  for (i = 0; i < blen; i++) {
    if (b[i] == '\0') return DHP_BAD_REQUEST;
    if (b[i] == '\r' && (i + 1 >= blen || b[i + 1] != '\n')) return DHP_BAD_REQUEST;
    if (b[i] == '\n' && (i == 0 || b[i - 1] != '\r')) return DHP_BAD_REQUEST;
  }

  /* request line: METHOD SP target SP HTTP/1.x */
  size_t le = 0;
  while (b[le] != '\r') le++;

  size_t sp1 = 0;
  while (sp1 < le && b[sp1] != ' ') sp1++;
  if (sp1 == 0 || sp1 >= le || sp1 > DHP_MAX_METHOD) return DHP_BAD_REQUEST;
  for (i = 0; i < sp1; i++) if (!is_tchar((unsigned char) b[i])) return DHP_BAD_REQUEST;

  size_t sp2 = sp1 + 1;
  while (sp2 < le && b[sp2] != ' ') sp2++;
  if (sp2 == sp1 + 1 || sp2 >= le) return DHP_BAD_REQUEST;
  if (sp2 - sp1 - 1 > DHP_MAX_TARGET) return DHP_BAD_REQUEST;
  for (i = sp1 + 1; i < sp2; i++) {
    unsigned char c = (unsigned char) b[i];
    if (c < 0x21 || c == 0x7f) return DHP_BAD_REQUEST;
  }

  size_t vn = le - sp2 - 1;
  if (vn != 8 || memcmp(b + sp2 + 1, "HTTP/1.", 7) != 0) return DHP_BAD_REQUEST;
  char minor = b[sp2 + 8];
  if (minor != '0' && minor != '1') return DHP_BAD_REQUEST;
  h->minor = minor - '0';

  h->method.p = b;
  h->method.n = sp1;
  h->target.p = b + sp1 + 1;
  h->target.n = sp2 - sp1 - 1;

  h->path = h->target;
  h->query.p = h->target.p + h->target.n;
  h->query.n = 0;
  for (i = 0; i < h->target.n; i++) {
    if (h->target.p[i] == '?') {
      h->path.n = i;
      h->query.p = h->target.p + i + 1;
      h->query.n = h->target.n - i - 1;
      break;
    }
  }

  h->keep_alive = h->minor == 1;
  int have_len = 0;

  /* headers */
  size_t pos = le + 2;
  while (pos < blen) {
    size_t e = pos;
    while (b[e] != '\r') e++;

    /* obsolete line folding is rejected */
    if (b[pos] == ' ' || b[pos] == '\t') return DHP_BAD_REQUEST;

    size_t colon = pos;
    while (colon < e && b[colon] != ':') colon++;
    if (colon == pos || colon >= e) return DHP_BAD_REQUEST;
    for (i = pos; i < colon; i++) if (!is_tchar((unsigned char) b[i])) return DHP_BAD_REQUEST;

    size_t vs = colon + 1, ve = e;
    while (vs < ve && (b[vs] == ' ' || b[vs] == '\t')) vs++;
    while (ve > vs && (b[ve - 1] == ' ' || b[ve - 1] == '\t')) ve--;

    if (h->nheaders >= DHP_MAX_HEADERS) return DHP_HEADER_TOO_LARGE;
    h->name[h->nheaders].p = b + pos;
    h->name[h->nheaders].n = colon - pos;
    h->value[h->nheaders].p = b + vs;
    h->value[h->nheaders].n = ve - vs;
    h->nheaders++;

    const char* nm = b + pos;
    size_t nn = colon - pos;
    const char* vl = b + vs;
    size_t vn2 = ve - vs;

    if (ieq(nm, nn, "content-length")) {
      size_t v = 0, d;
      if (vn2 == 0 || vn2 > 18) return DHP_BAD_REQUEST;
      for (d = 0; d < vn2; d++) {
        if (vl[d] < '0' || vl[d] > '9') return DHP_BAD_REQUEST;
        v = v * 10 + (size_t) (vl[d] - '0');
      }
      if (have_len && v != h->content_length) return DHP_BAD_REQUEST;
      have_len = 1;
      h->content_length = v;
    } else if (ieq(nm, nn, "transfer-encoding")) {
      return DHP_UNSUPPORTED;
    } else if (ieq(nm, nn, "connection")) {
      if (has_token(vl, vn2, "close")) h->keep_alive = 0;
      else if (has_token(vl, vn2, "keep-alive")) h->keep_alive = 1;
    } else if (ieq(nm, nn, "expect")) {
      if (ieq(vl, vn2, "100-continue")) h->expect_continue = 1;
    }

    pos = e + 2;
  }

  if (h->content_length > max_body) return DHP_BODY_TOO_LARGE;
  return DHP_OK;
}

int dhttp_result_status(dhp_result r) {
  switch (r) {
    case DHP_BAD_REQUEST:      return 400;
    case DHP_HEADER_TOO_LARGE: return 431;
    case DHP_BODY_TOO_LARGE:   return 413;
    case DHP_UNSUPPORTED:      return 501;
    default:                   return 200;
  }
}

const char* dhttp_result_name(dhp_result r) {
  switch (r) {
    case DHP_BAD_REQUEST:      return "bad_request";
    case DHP_HEADER_TOO_LARGE: return "header_too_large";
    case DHP_BODY_TOO_LARGE:   return "body_too_large";
    case DHP_UNSUPPORTED:      return "unsupported_transfer_encoding";
    default:                   return "";
  }
}

const char* dhttp_reason(int status) {
  switch (status) {
    case 100: return "Continue";
    case 101: return "Switching Protocols";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 204: return "No Content";
    case 206: return "Partial Content";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 307: return "Temporary Redirect";
    case 308: return "Permanent Redirect";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 413: return "Content Too Large";
    case 414: return "URI Too Long";
    case 415: return "Unsupported Media Type";
    case 422: return "Unprocessable Content";
    case 429: return "Too Many Requests";
    case 431: return "Request Header Fields Too Large";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    default:  return "Unknown";
  }
}
