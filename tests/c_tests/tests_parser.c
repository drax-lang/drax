#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * is defined on drax.h
 */
int is_teractive_mode = 0;

#include "../../src/dtypes.h"
#include "../../src/dparser.h"
#include "../../src/dvm.h"
#include "../../src/dlex.h"

d_vm* setup_vm() {
  d_vm* vm = (d_vm*) malloc(sizeof(d_vm));
  vm->active_instr = new_instructions();
  vm->stack_count = 0;
  vm->envs = new_environment(0, 0, 0);
  return vm;
}

void teardown_vm(d_vm* vm) {
  free(vm->envs->global);
  free(vm->envs->local);
  free(vm->envs);
  free(vm);
}

int run_parser_test(const char* input, d_op_code* expected_ops, int op_count) {
  d_vm* vm = setup_vm();
  is_teractive_mode = 0;

  printf("Input: '%-25s' -> ", input);

  if (__build__(vm, (char*) input, NULL) == 0) {
    printf("[FAIL] Compilation failed\n");
    teardown_vm(vm);
    return 0;
  }

  d_instructions* instr = vm->active_instr;
  int op_idx = 0;
  
  for (int i = 0; i < instr->instr_count && op_idx < op_count; i++) {
    drax_value val = instr->values[i];
    
    if (val == (drax_value)expected_ops[op_idx]) {
      op_idx++;
      
      if (val == OP_PUSH || val == OP_CONST || val == OP_SET_G_ID || 
        val == OP_GET_G_ID || val == OP_SET_L_ID || val == OP_GET_L_ID ||
        val == OP_D_CALL || val == OP_FUN) {
        i++; 
      }
    }
  }

  if (op_idx != op_count) {
    printf("[FAIL] Expected %d opcodes, matched %d\n", op_count, op_idx);
    teardown_vm(vm);
    return 0;
  }

  printf("[PASS]\n");
  teardown_vm(vm);
  return 1;
}

int test_arithmetic_bytecode() {
  printf("\n--- parser: arithmetic ---\n");
  
  d_op_code expected[] = {
    OP_PUSH, OP_PUSH, OP_PUSH, 
    OP_MUL, OP_ADD, OP_POP, OP_EXIT
  };

  return run_parser_test("1 + 2 * 3", expected, 7);
}

int test_control_flow_bytecode() {
  printf("\n--- parser: control flow ---\n");

  d_op_code expected_if[] = {
    OP_TRUE, OP_JMF, OP_POP, OP_PUSH, OP_JMP, OP_POP, OP_EXIT
  };

  return run_parser_test("if (true) do 1 end", expected_if, 7);
}

int test_assignment_bytecode() {
  printf("\n--- parser: assignment ---\n");
  
  d_op_code expected[] = {
    OP_PUSH, OP_SET_G_ID, OP_POP, OP_EXIT
  };

  return run_parser_test("x = 10", expected, 4);
}

int test_function_call_bytecode() {
  printf("\n--- parser: function call ---\n");
  
  d_op_code expected[] = {
    OP_GET_G_ID, OP_PUSH, OP_PUSH, OP_D_CALL, OP_POP, OP_EXIT
  };

  return run_parser_test("sum(1, 2)", expected, 6);
}

int test_pipe_operator_bytecode() {
  printf("\n--- parser: pipe operator ---\n");
  
  d_op_code expected[] = {
    OP_PUSH, OP_GET_G_ID, OP_D_CALL_P, OP_POP, OP_EXIT
  };

  return run_parser_test("10 |> func()", expected, 5);
}

int test_list_construction_bytecode() {
  printf("\n--- parser: list construction ---\n");
  
  d_op_code expected[] = {
    OP_PUSH, OP_PUSH, OP_CONST, OP_LIST, OP_POP, OP_EXIT
  };

  return run_parser_test("[1, 2]", expected, 6);
}

int test_tco_bytecode() {
  printf("\n--- parser: tail call optimization ---\n");
  
  d_op_code expected[] = {
    OP_FUN, OP_SET_G_ID, OP_POP, OP_EXIT
  };

  return run_parser_test("f = fun() do f() end", expected, 4);
}

int test_closure_capture_bytecode() {
  printf("\n--- parser: closure capture ---\n");
  
  d_op_code expected[] = {
    OP_PUSH, OP_SET_G_ID, OP_POP, 
    OP_FUN, OP_SET_G_ID, OP_POP, OP_EXIT
  };

  return run_parser_test("x = 10\n f = fun() do x + 1 end", expected, 7);
}

int test_nested_functions_bytecode() {
  printf("\n--- parser: nested functions ---\n");
  
  d_op_code expected[] = {
    OP_FUN, OP_SET_G_ID, OP_POP, OP_EXIT
  };

  return run_parser_test("adder = fun(x) do fun(y) do x + y end end", expected, 4);
}

int test_closure_with_pipe_bytecode() {
  printf("\n--- parser: closure with pipe ---\n");
  
  d_op_code expected[] = {
    OP_PUSH, OP_SET_G_ID, OP_POP,
    OP_FUN, OP_SET_G_ID, OP_POP, OP_EXIT
  };

  return run_parser_test("base = 10\n f = fun(v) do v |> add(base) end", expected, 7);
}

int main() {
  int all_passed = 1;
  printf("--- Starting Drax Parser Tests ---\n");

  all_passed &= test_arithmetic_bytecode();
  all_passed &= test_control_flow_bytecode();
  all_passed &= test_assignment_bytecode();
  all_passed &= test_function_call_bytecode();
  all_passed &= test_pipe_operator_bytecode();
  all_passed &= test_list_construction_bytecode();
  all_passed &= test_tco_bytecode();
  all_passed &= test_closure_capture_bytecode();
  all_passed &= test_nested_functions_bytecode();
  all_passed &= test_closure_with_pipe_bytecode();

  if (!all_passed) {
    fprintf(stderr, "\n[CRITICAL] Some failed tests.\n");
    exit(1);
  }

  printf("\n\033[32mAll parser tests passed!\033[0m\n");
  return 0;
}