/* Extension used by tests/drax/native.dx: it uses every part of drax_ext.h. */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "drax_ext.h"

static int released = 0;

static drax_xval add(const drax_api* d, drax_call* c, const drax_xval* a) {
  double x, y;
  if (!d->to_number(a[0], &x)) return d->errorf(c, "Greet.add: argument 1 must be a number, got %s", d->type_name(a[0]));
  if (!d->to_number(a[1], &y)) return d->errorf(c, "Greet.add: argument 2 must be a number, got %s", d->type_name(a[1]));
  return d->number(c, x + y);
}

static drax_xval shout(const drax_api* d, drax_call* c, const drax_xval* a) {
  const char* s;
  size_t n, i;
  if (!d->to_string(a[0], &s, &n)) return d->errorf(c, "Greet.shout: expected a string, got %s", d->type_name(a[0]));

  char* up = (char*) malloc(n + 1);
  for (i = 0; i < n; i++) up[i] = (char) toupper((unsigned char) s[i]);
  up[n] = '\0';

  drax_xval r = d->string(c, up, n);
  free(up);
  return r;
}

/* the bytes come back as they went in, also a NUL byte and a backslash */
static drax_xval echo(const drax_api* d, drax_call* c, const drax_xval* a) {
  const char* s;
  size_t n;
  if (!d->to_string(a[0], &s, &n)) return d->error(c, "Greet.echo: expected a string");
  return d->string(c, s, n);
}

static drax_xval length(const drax_api* d, drax_call* c, const drax_xval* a) {
  const char* s;
  size_t n;
  if (!d->to_string(a[0], &s, &n)) return d->error(c, "Greet.length: expected a string");
  return d->number(c, (double) n);
}

static drax_xval sum(const drax_api* d, drax_call* c, const drax_xval* a) {
  size_t i, n = d->list_length(a[0]);
  double total = 0;

  if (d->type(a[0]) != DRAX_T_LIST) return d->errorf(c, "Greet.sum: expected a list, got %s", d->type_name(a[0]));

  for (i = 0; i < n; i++) {
    double x;
    if (!d->to_number(d->list_at(a[0], i), &x)) {
      return d->errorf(c, "Greet.sum: item %d of the list is not a number (it is %s)", (int) i, d->type_name(d->list_at(a[0], i)));
    }
    total += x;
  }
  return d->number(c, total);
}

static drax_xval pair(const drax_api* d, drax_call* c, const drax_xval* a) {
  drax_xval l = d->list(c, 2);
  d->list_push(c, l, a[0]);
  d->list_push(c, l, a[1]);
  return l;
}

static drax_xval info(const drax_api* d, drax_call* c, const drax_xval* a) {
  (void) a;
  drax_xval f = d->frame(c);
  d->frame_set(c, f, "name", d->string(c, "greet", 5));
  d->frame_set(c, f, "version", d->number(c, 1));
  d->frame_set(c, f, "ok", d->boolean(c, 1));
  d->frame_set(c, f, "nothing", d->nil(c));
  return f;
}

/* reads a frame: Greet.field({a: 1}, 'a') */
static drax_xval field(const drax_api* d, drax_call* c, const drax_xval* a) {
  const char* key;
  size_t n;
  drax_xval v;
  if (!d->to_string(a[1], &key, &n)) return d->error(c, "Greet.field: the key must be a string");
  if (!d->frame_get(a[0], key, &v)) return d->nil(c);
  return v;
}

static drax_xval keys(const drax_api* d, drax_call* c, const drax_xval* a) {
  size_t i, n = d->frame_length(a[0]);
  drax_xval l = d->list(c, n);
  for (i = 0; i < n; i++) {
    const char* k = d->frame_key(a[0], i);
    d->list_push(c, l, d->string(c, k, strlen(k)));
  }
  return l;
}

static drax_xval type_of(const drax_api* d, drax_call* c, const drax_xval* a) {
  const char* t = d->type_name(a[0]);
  return d->string(c, t, strlen(t));
}

/* handles: a counter that lives in C and is released by the garbage collector */
typedef struct { double value; } counter;

static void counter_release(void* p) {
  free(p);
  released++;
}

static drax_xval counter_new(const drax_api* d, drax_call* c, const drax_xval* a) {
  double start;
  if (!d->to_number(a[0], &start)) return d->error(c, "Greet.counter_new: the start must be a number");
  counter* k = (counter*) malloc(sizeof(counter));
  k->value = start;
  return d->handle(c, k, "greet.counter", counter_release);
}

static drax_xval counter_next(const drax_api* d, drax_call* c, const drax_xval* a) {
  void* p;
  if (!d->get_handle(a[0], "greet.counter", &p)) {
    return d->errorf(c, "Greet.counter_next: expected a counter, got %s", d->type_name(a[0]));
  }
  ((counter*) p)->value += 1;
  return d->number(c, ((counter*) p)->value);
}

/* a handle of another kind: counter_next must refuse it */
static void other_release(void* p) { free(p); }

static drax_xval other_new(const drax_api* d, drax_call* c, const drax_xval* a) {
  (void) a;
  return d->handle(c, malloc(8), "greet.other", other_release);
}

static drax_xval released_count(const drax_api* d, drax_call* c, const drax_xval* a) {
  (void) a;
  return d->number(c, released);
}

/* the same name with two arities */
static drax_xval hello0(const drax_api* d, drax_call* c, const drax_xval* a) {
  (void) a;
  return d->string(c, "hello", 5);
}

static drax_xval hello1(const drax_api* d, drax_call* c, const drax_xval* a) {
  const char* s;
  size_t n;
  char buf[256];
  if (!d->to_string(a[0], &s, &n)) return d->error(c, "Greet.hello: the name must be a string");
  if (n > 200) n = 200;
  memcpy(buf, "hello ", 6);
  memcpy(buf + 6, s, n);
  return d->string(c, buf, n + 6);
}

static drax_xval fail(const drax_api* d, drax_call* c, const drax_xval* a) {
  const char* s;
  size_t n;
  if (!d->to_string(a[0], &s, &n)) return d->error(c, "Greet.fail: expected a string");
  return d->errorf(c, "failed: %s", s);
}

/* a list that came from the script must not change: list_push has to refuse it */
static drax_xval mutate_list(const drax_api* d, drax_call* c, const drax_xval* a) {
  return d->number(c, d->list_push(c, a[0], d->number(c, 99)));
}

static drax_xval mutate_frame(const drax_api* d, drax_call* c, const drax_xval* a) {
  return d->number(c, d->frame_set(c, a[0], "injected", d->number(c, 1)));
}

/* a list that this call created can change, and it can be long */
static drax_xval range(const drax_api* d, drax_call* c, const drax_xval* a) {
  double n;
  size_t i;
  if (!d->to_number(a[0], &n) || n < 0) return d->error(c, "Greet.range: expected a number >= 0");

  drax_xval l = d->list(c, 0);
  for (i = 0; i < (size_t) n; i++) {
    if (!d->list_push(c, l, d->number(c, (double) i))) return d->error(c, "Greet.range: list_push failed");
  }
  return l;
}

/* lists inside lists and frames inside frames, all created in this call */
static drax_xval nested(const drax_api* d, drax_call* c, const drax_xval* a) {
  (void) a;
  drax_xval outer = d->list(c, 0);
  drax_xval row = d->list(c, 0);
  drax_xval f = d->frame(c);
  d->list_push(c, row, d->number(c, 1));
  d->list_push(c, row, d->string(c, "x", 1));
  d->frame_set(c, f, "row", row);
  d->list_push(c, row, d->number(c, 2));     /* still the same list: it was created in this call */
  d->list_push(c, outer, f);
  d->list_push(c, outer, row);
  return outer;
}

/* the list that was made in a previous call is not "made in this call" */
static drax_xval kept = 0;

static drax_xval keep(const drax_api* d, drax_call* c, const drax_xval* a) {
  (void) a;
  kept = d->list(c, 0);
  return kept;
}

static drax_xval push_kept(const drax_api* d, drax_call* c, const drax_xval* a) {
  (void) a;
  return d->number(c, kept != 0 ? d->list_push(c, kept, d->number(c, 1)) : -1);
}

static int start(const drax_api* d, drax_registry* r) {
  d->module(r, "Greet");
  d->function(r, "mutate_list", 1, mutate_list);
  d->function(r, "mutate_frame", 1, mutate_frame);
  d->function(r, "range", 1, range);
  d->function(r, "nested", 0, nested);
  d->function(r, "keep", 0, keep);
  d->function(r, "push_kept", 0, push_kept);
  d->function(r, "add", 2, add);
  d->function(r, "shout", 1, shout);
  d->function(r, "echo", 1, echo);
  d->function(r, "length", 1, length);
  d->function(r, "sum", 1, sum);
  d->function(r, "pair", 2, pair);
  d->function(r, "info", 0, info);
  d->function(r, "field", 2, field);
  d->function(r, "keys", 1, keys);
  d->function(r, "type_of", 1, type_of);
  d->function(r, "counter_new", 1, counter_new);
  d->function(r, "counter_next", 1, counter_next);
  d->function(r, "released", 0, released_count);
  d->function(r, "other_new", 0, other_new);
  d->function(r, "hello", 0, hello0);
  d->function(r, "hello", 1, hello1);
  d->function(r, "fail", 1, fail);
  return 0;
}

DRAX_EXTENSION(start)
