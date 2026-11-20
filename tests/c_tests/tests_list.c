#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * is defined on drax.h
 */
int is_teractive_mode = 0;

#include "../../src/dtypes.h"
#include "../../src/dstructs.h"
#include "../../src/dvm.h"
#include "../../src/mods/d_mod_list.h"

/**
 * Immutability of lists with the in place append (dlist_concat).
 *
 * A list that was created never changes, whatever is done with the lists
 * that share its buffer (see docs/list-append-optimization.md).
 */

static int total = 0;
static int failed = 0;

#define CHECK(cond, name) \
  do { \
    total++; \
    if (cond) { printf("%-60s -> [PASS]\n", name); } \
    else { printf("%-60s -> [FAIL] (%s:%d)\n", name, __FILE__, __LINE__); failed++; } \
  } while (0)

static d_vm* vm;

static drax_list* mk(const int* v, int n) {
  drax_list* l = new_dlist(vm, n);
  for (int i = 0; i < n; i++) put_value_dlist(l, num_to_draxvalue((double) v[i]));
  return l;
}

static int eq(drax_list* l, const int* v, int n) {
  if (l->length != n) return 0;
  for (int i = 0; i < n; i++) {
    if (l->elems[i] != num_to_draxvalue((double) v[i])) return 0;
  }
  return 1;
}

static drax_list* cat(drax_list* a, drax_list* b) {
  return dlist_concat(vm, a, b);
}

static drax_list* tail(drax_list* l) {
  int stat = 1;
  push(vm, DS_VAL(l));
  return CAST_LIST(__d_list_tail(vm, &stat));
}

static drax_list* root_of(drax_list* l) {
  return l->owner != NULL ? l->owner : l;
}

/* a ++ [x]: the first append reuses the buffer, the second one must copy */
static void test_branching(void) {
  int base[] = {1, 2, 3};
  int x[] = {4}, y[] = {9};
  drax_list* a = cat(mk(base, 3), mk(x, 1));   /* [1,2,3,4], has room */
  drax_list* b = cat(a, mk(x, 1));             /* [1,2,3,4,4] in place */
  drax_list* c = cat(a, mk(y, 1));             /* [1,2,3,4,9] must copy */

  int ea[] = {1, 2, 3, 4}, eb[] = {1, 2, 3, 4, 4}, ec[] = {1, 2, 3, 4, 9};
  CHECK(eq(a, ea, 4), "branching: base is not changed");
  CHECK(eq(b, eb, 5), "branching: first append is right");
  CHECK(eq(c, ec, 5), "branching: second append is right");
  CHECK(b->elems == a->elems && root_of(b) == root_of(a),
        "branching: first append shares the buffer (in place)");
  CHECK(c->elems != a->elems && root_of(c) != root_of(a),
        "branching: second append copies to a new buffer");
}

/* the slot written past the end of a list is never visible through it */
static void test_old_version(void) {
  int s[] = {10, 20, 30};
  int v1[] = {40}, v2[] = {50};
  drax_list* old = cat(mk(s, 2), mk(&s[2], 1));
  drax_list* n1 = cat(old, mk(v1, 1));
  drax_list* n2 = cat(old, mk(v2, 1));
  drax_list* n3 = cat(n1, mk(v2, 1));

  int eo[] = {10, 20, 30}, e1[] = {10, 20, 30, 40}, e2[] = {10, 20, 30, 50}, e3[] = {10, 20, 30, 40, 50};
  CHECK(eq(old, eo, 3), "old version: still [10,20,30]");
  CHECK(eq(n1, e1, 4), "old version: newer list intact");
  CHECK(eq(n2, e2, 4), "old version: append to an old list is right");
  CHECK(eq(n3, e3, 5), "old version: append to the newer list is right");
}

/* List.tail shares the buffer with an offset */
static void test_tail(void) {
  int s[] = {1, 2, 3, 4}, x[] = {9}, y[] = {8};
  drax_list* base = cat(mk(s, 3), mk(&s[3], 1));
  drax_list* t = tail(base);
  drax_list* t1 = cat(t, mk(x, 1));
  drax_list* t2 = cat(t, mk(y, 1));
  drax_list* b2 = cat(base, mk(y, 1));

  int eb[] = {1, 2, 3, 4}, et[] = {2, 3, 4}, e1[] = {2, 3, 4, 9}, e2[] = {2, 3, 4, 8}, eb2[] = {1, 2, 3, 4, 8};
  CHECK(eq(base, eb, 4), "tail: list is not changed by appends to its tail");
  CHECK(eq(t, et, 3), "tail: tail is not changed");
  CHECK(eq(t1, e1, 4), "tail: first append to the tail is right");
  CHECK(eq(t2, e2, 4), "tail: second append to the tail is right");
  CHECK(eq(b2, eb2, 5), "tail: append to the list after tail appends is right");
}

/* the tail of an appended list, then an append to the original */
static void test_tail_of_appended(void) {
  int s[] = {1, 2, 3}, x[] = {4}, y[] = {5};
  drax_list* a = cat(mk(s, 2), mk(&s[2], 1));
  drax_list* ax = cat(a, mk(x, 1));
  drax_list* t = tail(ax);
  drax_list* z = cat(a, mk(y, 1));

  int et[] = {2, 3, 4}, ez[] = {1, 2, 3, 5}, ea[] = {1, 2, 3};
  CHECK(eq(t, et, 3), "tail of appended: [2,3,4]");
  CHECK(eq(z, ez, 4), "tail of appended: append to the original is right");
  CHECK(eq(a, ea, 3), "tail of appended: original is not changed");
}

/* empty sides return the other list, nothing is copied or changed */
static void test_empty(void) {
  int s[] = {1, 2};
  drax_list* empty = new_dlist(vm, 0);
  drax_list* a = mk(s, 2);

  CHECK(cat(a, empty) == a, "empty: l ++ [] is l");
  CHECK(cat(empty, a) == a, "empty: [] ++ l is l");
  CHECK(cat(empty, empty)->length == 0, "empty: [] ++ [] is empty");
  CHECK(eq(a, s, 2), "empty: list is not changed");
}

/* a right side that does not fit in the free room copies, the left is intact */
static void test_right_side_bigger_than_room(void) {
  int s[] = {1, 2}, r[20];
  for (int i = 0; i < 20; i++) r[i] = 100 + i;
  drax_list* a = cat(mk(s, 1), mk(&s[1], 1));
  drax_list* big = cat(a, mk(r, 20));

  int e[22] = {1, 2};
  for (int i = 0; i < 20; i++) e[2 + i] = r[i];
  CHECK(eq(big, e, 22), "big right side: result is right");
  CHECK(eq(a, s, 2), "big right side: left list is not changed");
}

/* the result must never share storage that is writable by a later append */
static void test_result_is_independent(void) {
  int s[] = {1, 2, 3, 4, 5, 6, 7, 8};
  drax_list* a = mk(s, 8);
  drax_list* acc = new_dlist(vm, 0);
  drax_list* snaps[64];
  int lens[64];
  int count = 0;

  for (int i = 0; i < 40; i++) {
    int one[] = {i};
    acc = cat(acc, mk(one, 1));
    if (count < 64) { snaps[count] = acc; lens[count] = acc->length; count++; }
  }

  /* every snapshot still has its length and its values 0..len-1 */
  int ok = 1;
  for (int k = 0; k < count; k++) {
    if (snaps[k]->length != lens[k]) ok = 0;
    for (int i = 0; i < snaps[k]->length; i++) {
      if (snaps[k]->elems[i] != num_to_draxvalue((double) i)) ok = 0;
    }
  }
  CHECK(ok, "growing loop: every intermediate list keeps its values");
  CHECK(eq(a, s, 8), "growing loop: unrelated list is not changed");
}

/* deterministic xorshift, the same sequence on every run */
static unsigned int rng_state = 2463534242u;
static unsigned int rnd(void) {
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  return rng_state;
}

#define MAX_LISTS 400
#define MAX_LEN 600

/**
 * Model check: random ++ and tail over lists that already exist, against a
 * plain array that is the expected content of each list. After every step ALL
 * the lists created so far are compared with their expected content.
 */
static void test_random_model(void) {
  static drax_list* lists[MAX_LISTS];
  static int model[MAX_LISTS][MAX_LEN];
  static int mlen[MAX_LISTS];
  int n = 0;
  int ok = 1;
  int next_value = 1;

  lists[0] = new_dlist(vm, 0);
  mlen[0] = 0;
  n = 1;

  for (int step = 0; step < 3000 && n < MAX_LISTS && ok; step++) {
    int op = (int) (rnd() % 10);
    int pick = (op < 6 && n > 1) ? (int) (n - 1 - (int) (rnd() % (unsigned) (n < 4 ? n : 4)))
                                 : (int) (rnd() % (unsigned) n);   /* mostly the newest lists */

    if (op < 8) {
      int rl = (int) (rnd() % 4) + (op == 7 ? 0 : 1);   /* 0..3 or 1..4 elements */
      int r[4];
      for (int i = 0; i < rl; i++) r[i] = next_value++;
      if (mlen[pick] + rl > MAX_LEN) continue;

      drax_list* res = cat(lists[pick], mk(r, rl));
      lists[n] = res;
      memcpy(model[n], model[pick], sizeof(int) * (size_t) mlen[pick]);
      memcpy(model[n] + mlen[pick], r, sizeof(int) * (size_t) rl);
      mlen[n] = mlen[pick] + rl;
      n++;
    } else if (lists[pick]->length > 0) {
      lists[n] = tail(lists[pick]);
      memcpy(model[n], model[pick] + 1, sizeof(int) * (size_t) (mlen[pick] - 1));
      mlen[n] = mlen[pick] - 1;
      n++;
    }

    for (int k = 0; k < n; k++) {
      if (!eq(lists[k], model[k], mlen[k])) {
        printf("  list %d differs from the model after step %d\n", k, step);
        ok = 0;
        break;
      }
    }
  }

  CHECK(ok, "random model: every list keeps its expected content");
  CHECK(n > 100, "random model: ran a meaningful number of steps");
}

int main(void) {
  vm = createMainVM();

  printf("\n=== list immutability ===\n");
  test_branching();
  test_old_version();
  test_tail();
  test_tail_of_appended();
  test_empty();
  test_right_side_bigger_than_room();
  test_result_is_independent();
  test_random_model();

  __destroy__(vm);

  if (failed > 0) {
    printf("\n%d of %d list tests failed!\n", failed, total);
    return 1;
  }

  printf("\nAll list tests passed! (%d checks)\n", total);
  return 0;
}
