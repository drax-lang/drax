#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * is defined on drax.h
 */
int is_teractive_mode = 0;

#include "../../src/dtypes.h"
#include "../../src/dstructs.h"
#include "../../src/dhandler.h"
#include "../../src/dparser.h"
#include "../../src/dvm.h"

/**
 * Tests of the core of the VM: source -> compiler -> interpreter loop, driven
 * from C. The result of a program is read from the global variable 'r', and
 * the state of the VM (stacks, GC) is checked after it ran.
 */

static int total = 0;
static int failed = 0;

#define CHECK(cond, name) \
  do { \
    total++; \
    if (cond) { printf("%-64s -> [PASS]\n", name); } \
    else { printf("%-64s -> [FAIL] (%s:%d)\n", name, __FILE__, __LINE__); failed++; } \
  } while (0)

#define OUT_SIZE 8192

static d_vm* new_vm(void) {
  return createMainVM();
}

static void free_vm(d_vm* vm) {
  __destroy__(vm);
}

/**
 * Builds and runs 'src'. What the program prints (stdout and stderr, where
 * the errors go) is copied to 'out'. Returns the result of the loop (0 = ok)
 * or -1 when the program does not compile.
 */
static int run_src(d_vm* vm, const char* src, char* out, size_t out_size) {
  char tmpl[] = "/tmp/drax_vm_test_XXXXXX";
  int fd = mkstemp(tmpl);
  int saved_out = dup(STDOUT_FILENO);
  int saved_err = dup(STDERR_FILENO);
  int rc;

  fflush(stdout);
  fflush(stderr);
  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);

  if (__build__(vm, (char*) src, NULL)) {
    rc = __run__(vm, 0);
  } else {
    rc = -1;
  }

  fflush(stdout);
  fflush(stderr);
  dup2(saved_out, STDOUT_FILENO);
  dup2(saved_err, STDERR_FILENO);
  close(saved_out);
  close(saved_err);

  if (out != NULL) {
    lseek(fd, 0, SEEK_SET);
    ssize_t n = read(fd, out, out_size - 1);
    out[n > 0 ? n : 0] = '\0';
  }

  close(fd);
  unlink(tmpl);
  return rc;
}

static int global(d_vm* vm, const char* name, drax_value* v) {
  return get_var_table(vm->envs->global, (char*) name, v);
}

/* runs the program and returns r, or the nil value when it fails */
static drax_value eval(d_vm* vm, const char* src) {
  drax_value r = DRAX_NIL_VAL;
  if (run_src(vm, src, NULL, 0) != 0) return DRAX_NIL_VAL;
  if (!global(vm, "r", &r)) return DRAX_NIL_VAL;
  return r;
}

static int num_is(const char* src, double expected) {
  d_vm* vm = new_vm();
  drax_value r = eval(vm, src);
  int ok = IS_NUMBER(r) && CAST_NUMBER(r) == expected;
  if (!ok) printf("    program: %s\n", src);
  free_vm(vm);
  return ok;
}

static int bool_is(const char* src, int expected) {
  d_vm* vm = new_vm();
  drax_value r = eval(vm, src);
  int ok = IS_BOOL(r) && (CAST_BOOL(r) != 0) == expected;
  if (!ok) printf("    program: %s\n", src);
  free_vm(vm);
  return ok;
}

static int str_is(const char* src, const char* expected) {
  d_vm* vm = new_vm();
  drax_value r = eval(vm, src);
  int ok = IS_STRING(r) && strcmp(CAST_STRING(r)->chars, expected) == 0;
  if (!ok) printf("    program: %s\n", src);
  free_vm(vm);
  return ok;
}

/* the program must fail and report 'message' */
static int fails_with(const char* src, const char* message) {
  d_vm* vm = new_vm();
  char out[OUT_SIZE];
  int rc = run_src(vm, src, out, sizeof(out));
  int ok = rc != 0 && strstr(out, message) != NULL;
  if (!ok) printf("    program: %s\n    rc=%d output: %s\n", src, rc, out);
  free_vm(vm);
  return ok;
}

/* ---------------------------------------------------------------- */

static void test_arithmetic(void) {
  CHECK(num_is("r = 2 + 3 * 4", 14), "arithmetic: precedence of * over +");
  CHECK(num_is("r = (2 + 3) * 4", 20), "arithmetic: parentheses");
  CHECK(num_is("r = 10 / 4", 2.5), "arithmetic: division is not integer");
  CHECK(num_is("r = 7 - 10", -3), "arithmetic: negative result");
  CHECK(num_is("r = 1 - 2 - 3", -4), "arithmetic: - is left associative");
  CHECK(num_is("r = 100 / 10 / 5", 2), "arithmetic: / is left associative");
  CHECK(num_is("r = 0.1 + 0.2 - 0.3 + 1", 1 + (0.1 + 0.2 - 0.3)), "arithmetic: floating point");
  CHECK(num_is("r = 0x10 + 0b11", 19), "arithmetic: hex and binary literals");
}

static void test_values_and_comparison(void) {
  CHECK(bool_is("r = 3 < 4", 1), "compare: <");
  CHECK(bool_is("r = 4 < 3", 0), "compare: < false");
  CHECK(bool_is("r = 3 <= 3", 1), "compare: <=");
  CHECK(bool_is("r = 3 >= 4", 0), "compare: >=");
  CHECK(bool_is("r = 3 == 3", 1), "compare: == numbers");
  CHECK(bool_is("r = 3 != 3", 0), "compare: != numbers");
  CHECK(bool_is("r = \"a\" == \"a\"", 1), "compare: == strings by value");
  CHECK(bool_is("r = \"a\" == \"b\"", 0), "compare: == different strings");
  CHECK(bool_is("r = [1, [2]] == [1, [2]]", 1), "compare: == nested lists by value");
  CHECK(bool_is("r = [1, 2] == [1, 3]", 0), "compare: == different lists");
  CHECK(bool_is("r = true and false", 0), "logic: and");
  CHECK(bool_is("r = false or true", 1), "logic: or");

  d_vm* vm = new_vm();
  drax_value r = eval(vm, "r = nil");
  CHECK(IS_NIL(r), "values: nil literal");
  free_vm(vm);
}

static void test_strings_and_lists(void) {
  CHECK(str_is("r = \"ab\" ++ \"cd\"", "abcd"), "strings: ++ concatenates");
  CHECK(str_is("r = \"\" ++ \"x\"", "x"), "strings: ++ with an empty string");
  CHECK(num_is("r = List.length([1, 2, 3] ++ [4])", 4), "lists: ++ concatenates");
  CHECK(num_is("r = List.at([10, 20, 30], 1)", 20), "lists: List.at");
  CHECK(num_is("r = List.at([10, 20, 30], -1)", 30), "lists: List.at with a negative index");

  d_vm* vm = new_vm();
  drax_value r = eval(vm, "r = [1, \"a\", [2, 3]]");
  CHECK(IS_LIST(r) && CAST_LIST(r)->length == 3, "lists: a literal keeps its elements");
  CHECK(IS_LIST(r) && IS_NUMBER(CAST_LIST(r)->elems[0]) && IS_STRING(CAST_LIST(r)->elems[1]) && IS_LIST(CAST_LIST(r)->elems[2]),
        "lists: elements of different types");
  free_vm(vm);
}

static void test_variables(void) {
  CHECK(num_is("a = 1\nb = a + 1\na = 10\nr = a + b", 12), "variables: a later assignment changes the value");
  CHECK(num_is("a = 1\nb = a\na = 2\nr = b", 1), "variables: assignment copies the value");

  d_vm* vm = new_vm();
  char out[OUT_SIZE];
  int rc = run_src(vm, "x = 1", out, sizeof(out));
  drax_value v;
  CHECK(rc == 0 && global(vm, "x", &v) && CAST_NUMBER(v) == 1, "variables: a global is defined");
  CHECK(!global(vm, "never_defined", &v), "variables: an unknown name is not defined");
  free_vm(vm);
}

static void test_control_flow(void) {
  CHECK(num_is("r = if (1 < 2) do 10 else 20 end", 10), "if: the true branch");
  CHECK(num_is("r = if (2 < 1) do 10 else 20 end", 20), "if: the false branch");
  CHECK(num_is("r = if (nil) do 1 else 2 end", 2), "if: nil is false");
  CHECK(num_is("r = if (0) do 1 else 2 end", 1), "if: 0 is true");
  CHECK(num_is("r = if (1 > 2) do 1 else if (2 > 1) do 2 else 3 end end", 2), "if: nested else if");

  /* the right side of and/or is not evaluated when the left decides */
  CHECK(bool_is("fun boom() do undefined_name end\nr = false and boom()", 0), "logic: and does not evaluate the right side");
  CHECK(bool_is("fun boom() do undefined_name end\nr = true or boom()", 1), "logic: or does not evaluate the right side");
}

static void test_functions(void) {
  CHECK(num_is("fun add(a, b) do a + b end\nr = add(2, 3)", 5), "functions: a call");
  CHECK(num_is("fun f(a) do 1 end\nfun f(a, b) do 2 end\nr = f(1) + f(1, 2) * 10", 21), "functions: the arity selects the function");
  CHECK(num_is("fun f(n) do n * 2 end\nr = f(f(f(1)))", 8), "functions: nested calls");
  CHECK(num_is("f = fun(x) do x + 1 end\nr = f(1)", 2), "functions: a lambda");
  CHECK(num_is("fun mk(n) do fun(x) do x + n end end\nadd5 = mk(5)\nr = add5(1)", 6), "functions: a closure captures its argument");
  CHECK(num_is("fun mk(n) do fun(x) do x + n end end\na = mk(1)\nb = mk(100)\nr = a(1) + b(1)", 103), "functions: closures do not share their captured values");
  CHECK(num_is("fun twice(f, x) do f(f(x)) end\nr = twice(fun(n) do n * 3 end, 2)", 18), "functions: a function as an argument");
  CHECK(num_is("fun inc(x) do x + 1 end\nr = 1 |> inc() |> inc()", 3), "functions: the pipe operator");
  CHECK(num_is("fun f(n) do\n  return n * 2\n  99\nend\nr = f(4)", 8), "functions: return ends the function");
  CHECK(num_is("r = List.at(List.map([1, 2, 3], fun(x) do x * 2 end), 2)", 6), "functions: a callback through a native function");
}

static void test_recursion(void) {
  CHECK(num_is("fun fib(n) do if (n < 2) do n else fib(n - 1) + fib(n - 2) end end\nr = fib(15)", 610), "recursion: fib(15)");
  CHECK(num_is("fun s(n) do if (n == 0) do 0 else n + s(n - 1) end end\nr = s(1000)", 500500), "recursion: body recursion of depth 1000");

  /* a tail call reuses the frame: a limit of 100 frames is never reached */
  d_vm* vm = new_vm();
  vm->max_call_depth = 100;
  drax_value r = eval(vm, "fun loop(n, acc) do if (n == 0) do acc else loop(n - 1, acc + n) end end\nr = loop(300000, 0)");
  CHECK(IS_NUMBER(r) && CAST_NUMBER(r) == 45000150000.0, "recursion: a tail call runs 300000 times with a call depth of 100");
  CHECK(vm->call_stack->count == 0, "recursion: the call stack is empty afterwards");
  free_vm(vm);

  /* tail call in the branches of an if, and through and/or */
  vm = new_vm();
  vm->max_call_depth = 100;
  r = eval(vm, "fun even(n) do if (n == 0) do true else odd(n - 1) end end\nfun odd(n) do if (n == 0) do false else even(n - 1) end end\nr = even(100001)");
  CHECK(IS_BOOL(r) && !CAST_BOOL(r), "recursion: mutual tail calls with a call depth of 100");
  free_vm(vm);

  /* a body call is limited by max_call_depth and ends with a clean error */
  vm = new_vm();
  vm->max_call_depth = 1000;
  char out[OUT_SIZE];
  int rc = run_src(vm, "fun s(n) do if (n == 0) do 0 else n + s(n - 1) end end\nr = s(5000)", out, sizeof(out));
  CHECK(rc != 0 && strstr(out, "stack overflow") != NULL, "recursion: a body call over the limit is a stack overflow error");
  free_vm(vm);
}

static void test_vm_state(void) {
  /* after a program that finishes, nothing is left on the stacks */
  const char* programs[] = {
    "r = 1 + 2",
    "fun f(a, b) do a + b end\nr = f(1, 2)",
    "fun fib(n) do if (n < 2) do n else fib(n - 1) + fib(n - 2) end end\nr = fib(12)",
    "fun loop(n) do if (n == 0) do 0 else loop(n - 1) end end\nr = loop(1000)",
    "r = List.map([1, 2, 3], fun(x) do x + 1 end)",
    "r = [1, 2, 3] ++ [4]\nq = { a: 1, b: 2 }\nz = q.a + q.b",
    "fun mk(n) do fun(x) do x + n end end\nr = mk(1)(2)",
  };
  int ok_stack = 1, ok_calls = 1;
  for (size_t i = 0; i < sizeof(programs) / sizeof(programs[0]); i++) {
    d_vm* vm = new_vm();
    if (run_src(vm, programs[i], NULL, 0) != 0) { printf("    did not run: %s\n", programs[i]); ok_stack = 0; }
    if (vm->stack_count != 0) { printf("    stack_count=%d after: %s\n", vm->stack_count, programs[i]); ok_stack = 0; }
    if (vm->call_stack->count != 0) { printf("    call stack=%d after: %s\n", vm->call_stack->count, programs[i]); ok_calls = 0; }
    free_vm(vm);
  }
  CHECK(ok_stack, "state: the value stack is empty after a program");
  CHECK(ok_calls, "state: the call stack is empty after a program");
}

static void test_errors(void) {
  CHECK(fails_with("r = undefined_name", "'undefined_name' is not defined"), "errors: an undefined variable");
  CHECK(fails_with("fun f(a) do a end\nr = f(1, 2)", "function 'f/2' is not defined"), "errors: a call with the wrong arity");
  CHECK(fails_with("r = 1 + \"a\"", "Bad argument in arithmetic expression"), "errors: arithmetic with a string");
  CHECK(fails_with("r = \"a\" < \"b\"", "Bad argument in arithmetic expression"), "errors: comparing strings");
  CHECK(fails_with("x = 5\nr = x()", "function is not valid"), "errors: calling a number");
  CHECK(fails_with("r = 1 ++ 2", "Concat with unspected type"), "errors: concatenating numbers");
  CHECK(fails_with("a = { x: 1 }\na.x = 2", "invalid assigment"), "errors: assigning into a frame");
  CHECK(fails_with("assert(false, \"boom\")", "boom"), "errors: assert reports its message");
  CHECK(fails_with("r = (", "Expect expression"), "errors: a syntax error is reported");

  /* a program that does not compile is not run */
  d_vm* vm = new_vm();
  char out[OUT_SIZE];
  int rc = run_src(vm, "r = 1\nr = (", out, sizeof(out));
  drax_value v;
  CHECK(rc == -1, "errors: a syntax error stops before running");
  CHECK(!global(vm, "r", &v), "errors: nothing of a program with a syntax error ran");
  free_vm(vm);

  /* the error line is reported */
  vm = new_vm();
  run_src(vm, "a = 1\nb = 2\nr = undefined_name", out, sizeof(out));
  CHECK(strstr(out, "line: 3") != NULL, "errors: the line of the failure is reported");
  free_vm(vm);
}

static void test_vm_reuse(void) {
  /* after a failure and a reset the same VM runs another program */
  d_vm* vm = new_vm();
  char out[OUT_SIZE];
  int rc1 = run_src(vm, "fun f(n) do n + undefined_name end\nr = f(1)", out, sizeof(out));
  __reset__(vm);
  drax_value r = eval(vm, "r = 40 + 2");
  CHECK(rc1 != 0, "reuse: the first program failed");
  CHECK(IS_NUMBER(r) && CAST_NUMBER(r) == 42, "reuse: the VM runs another program after a failure");
  CHECK(vm->stack_count == 0 && vm->call_stack->count == 0, "reuse: the stacks are clean after the second program");
  free_vm(vm);

  /* the globals of the first program stay for the next ones */
  vm = new_vm();
  run_src(vm, "base = 10", NULL, 0);
  __reset__(vm);
  r = eval(vm, "r = base + 5");
  CHECK(IS_NUMBER(r) && CAST_NUMBER(r) == 15, "reuse: a global of a previous program is visible");
  free_vm(vm);

  /* two VMs do not share anything */
  d_vm* a = new_vm();
  d_vm* b = new_vm();
  drax_value ra = eval(a, "r = 1");
  drax_value rb = eval(b, "r = 2");
  drax_value ra2;
  CHECK(IS_NUMBER(ra) && IS_NUMBER(rb) && CAST_NUMBER(ra) == 1 && CAST_NUMBER(rb) == 2, "reuse: two VMs have their own globals");
  CHECK(global(a, "r", &ra2) && CAST_NUMBER(ra2) == 1, "reuse: the other VM did not change the first one");
  free_vm(a);
  free_vm(b);
}

static void test_protected_calls(void) {
  d_vm* vm = new_vm();
  drax_value r = eval(vm, "fun f() do undefined_name end\nr = f?()");
  CHECK(IS_ERROR(r), "protected: a failing call evaluates to an Error value");
  free_vm(vm);

  vm = new_vm();
  r = eval(vm, "fun f() do undefined_name end\nx = f?()\nr = 7");
  CHECK(IS_NUMBER(r) && CAST_NUMBER(r) == 7, "protected: the program goes on after the handled error");
  CHECK(vm->stack_count == 0 && vm->call_stack->count == 0, "protected: the stacks are clean after a handled error");
  free_vm(vm);

  vm = new_vm();
  r = eval(vm, "fun f(n) do n * 2 end\nr = f?(4)");
  CHECK(IS_NUMBER(r) && CAST_NUMBER(r) == 8, "protected: a call that works returns its value");
  free_vm(vm);

  /* an error that nobody handles still ends the program */
  CHECK(fails_with("fun f() do undefined_name end\nr = f()", "'undefined_name' is not defined"), "protected: without ? the failure is not handled");
}

static void test_gc(void) {
  d_vm* vm = new_vm();
  vm->gc_meta->next_gc = 4096;

  drax_value r = eval(vm,
    "keep = [1, 2, 3]\n"
    "name = \"a\" ++ \"b\"\n"
    "fun churn(n, acc) do\n"
    "  if (n == 0) do acc else churn(n - 1, [n, n + 1, \"s\" ++ \"t\", [n]]) end\n"
    "end\n"
    "r = churn(20000, 0)");

  drax_value keep, name;
  CHECK(vm->gc_meta->n_cycles > 0, "gc: the collector ran while the program ran");
  CHECK(IS_LIST(r) && CAST_LIST(r)->length == 4 && CAST_NUMBER(CAST_LIST(r)->elems[0]) == 1, "gc: the result survives");
  CHECK(global(vm, "keep", &keep) && IS_LIST(keep) && CAST_LIST(keep)->length == 3 &&
        CAST_NUMBER(CAST_LIST(keep)->elems[2]) == 3, "gc: a global list survives");
  CHECK(global(vm, "name", &name) && IS_STRING(name) && strcmp(CAST_STRING(name)->chars, "ab") == 0, "gc: a global string survives");
  free_vm(vm);

  /* values only reachable from the stack while a native function runs */
  vm = new_vm();
  vm->gc_meta->next_gc = 4096;
  r = eval(vm, "r = List.length(List.map([1, 2, 3, 4, 5, 6, 7, 8], fun(x) do [x, x, [x], \"v\" ++ \"w\"] end))");
  CHECK(IS_NUMBER(r) && CAST_NUMBER(r) == 8, "gc: a callback that allocates inside List.map");
  free_vm(vm);

  /* a closure keeps what it captured alive */
  vm = new_vm();
  vm->gc_meta->next_gc = 4096;
  r = eval(vm,
    "fun mk(l) do fun() do List.length(l) end end\n"
    "f = mk([1, 2, 3, 4])\n"
    "fun churn(n) do if (n == 0) do 0 else churn(n - 1 + 0 * List.length([n, n, n])) end end\n"
    "churn(20000)\n"
    "r = f()");
  CHECK(IS_NUMBER(r) && CAST_NUMBER(r) == 4, "gc: a closure keeps its captured list alive");
  free_vm(vm);
  /* the payload of an Error is only reachable through the Error */
  vm = new_vm();
  vm->gc_meta->next_gc = 4096;
  r = eval(vm,
    "e = Error([1, 2, 3, 4])\n"
    "fun churn(n) do if (n == 0) do 0 else churn(n - 1 + 0 * List.length([n, n, n, n])) end end\n"
    "churn(20000)\n"
    "r = e");
  CHECK(IS_ERROR(r) && IS_LIST(CAST_ERROR(r)->payload) && CAST_LIST(CAST_ERROR(r)->payload)->length == 4 &&
        CAST_NUMBER(CAST_LIST(CAST_ERROR(r)->payload)->elems[3]) == 4, "gc: the payload of an Error survives");
  free_vm(vm);
}

static void test_list_values(void) {
  /* the in place append never changes a list that already exists */
  CHECK(num_is("a = [1, 2] ++ [3]\nb = a ++ [4]\nc = a ++ [5]\nr = List.length(a) * 100 + List.at(b, 3) * 10 + List.at(c, 3)", 345),
        "lists: appends from the same list are independent");
  CHECK(num_is("fun build(n, acc) do if (n == 0) do acc else build(n - 1, acc ++ [n]) end end\nr = List.length(build(5000, []))", 5000),
        "lists: a list built by appending in a loop");
}

static void test_frames(void) {
  CHECK(num_is("f = { a: 1, b: 2 }\nr = f.a + f.b", 3), "frames: access to the keys");
  CHECK(num_is("f = { a: { b: { c: 42 } } }\nr = f.a.b.c", 42), "frames: nested access");
  CHECK(num_is("f = Frame.put({ a: 1 }, \"b\", 2)\nr = f.a + f.b", 3), "frames: Frame.put returns a new frame");
  CHECK(num_is("f = { a: 1 }\ng = Frame.put(f, \"a\", 9)\nr = f.a * 10 + g.a", 19), "frames: Frame.put does not change the original");
  d_vm* vm = new_vm();
  drax_value r = eval(vm, "f = { a: 1 }\nr = f.b");
  CHECK(IS_NIL(r), "frames: a missing key is nil");
  free_vm(vm);
  CHECK(fails_with("f = { a: 1 }\nr = f.b.c", "key 'c' is not defined"), "frames: a key of nil is an error");
}

int main(void) {
  printf("\n=== core of the VM ===\n");
  test_arithmetic();
  test_values_and_comparison();
  test_strings_and_lists();
  test_variables();
  test_control_flow();
  test_functions();
  test_recursion();
  test_vm_state();
  test_errors();
  test_vm_reuse();
  test_protected_calls();
  test_gc();
  test_list_values();
  test_frames();

  if (failed > 0) {
    printf("\n%d of %d VM tests failed!\n", failed, total);
    return 1;
  }

  printf("\nAll VM tests passed! (%d checks)\n", total);
  return 0;
}
