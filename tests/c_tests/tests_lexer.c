#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../src/dlex.h"

int run_lexan_test(const char* input, dlex_types expected_type, d_num_type expected_num_base) {
  init_lexan(input);
  d_token t = next_token();

  printf("Input: '%-20s' -> ", input);

  if (t.type != expected_type) {
    printf("[FAIL] Expected type %d, got %d (Msg: %.*s)\n", 
            expected_type, t.type, t.length, t.first);
    return 0;
  }

  if (t.type == DTK_NUMBER && t.num_type != expected_num_base) {
    printf("[FAIL] Expected base %d, got %d\n", expected_num_base, t.num_type);
    return 0;
  }

  printf("[PASS]\n");
  return 1;
}

int main() {
  int all_passed = 1;
  printf("--- startting lexer tests ---\n");
  printf("---         numbers       ---\n");

  all_passed &= run_lexan_test("150", DTK_NUMBER, DNT_DECIMAL);
  all_passed &= run_lexan_test("3.1415", DTK_NUMBER, DNT_DECIMAL);
  all_passed &= run_lexan_test("0xFF", DTK_NUMBER, DNT_HEX);
  all_passed &= run_lexan_test("0b101", DTK_NUMBER, DNT_BIN);
  all_passed &= run_lexan_test("0o77", DTK_NUMBER, DNT_OCT);

  all_passed &= run_lexan_test("0x", DTK_ERROR, 0);
  all_passed &= run_lexan_test("0b102", DTK_ERROR, 0);
  all_passed &= run_lexan_test("0o88", DTK_ERROR, 0);
  all_passed &= run_lexan_test("0x1.A", DTK_ERROR, 0);

  printf("\n---  mult. lines string   ---\n");

  all_passed &= run_lexan_test("\"\"\"hey\"\"\"", DTK_MSTRING, 0);
  all_passed &= run_lexan_test("\"\"\" infinity string", DTK_ERROR, 0);
  all_passed &= run_lexan_test("\"\"\"\"\"\"", DTK_MSTRING, 0);


  printf("\n--- single line strings ---\n");
  all_passed &= run_lexan_test("\"hello world\"", DTK_DSTRING, 0);
  all_passed &= run_lexan_test("\"\"", DTK_DSTRING, 0);
  all_passed &= run_lexan_test("\"escaped \\\" quotes\"", DTK_DSTRING, 0);
  
  all_passed &= run_lexan_test("\"unterminated string", DTK_ERROR, 0);

  printf("\n--- identifiers & keywords ---\n");
  all_passed &= run_lexan_test("my_var", DTK_ID, 0);
  all_passed &= run_lexan_test("_private", DTK_ID, 0);
  all_passed &= run_lexan_test("camelCase123", DTK_ID, 0);
  
  /**
   * Internal words
   */
  all_passed &= run_lexan_test("if", DTK_IF, 0);
  all_passed &= run_lexan_test("as", DTK_AS, 0);
  all_passed &= run_lexan_test("do", DTK_DO, 0);
  all_passed &= run_lexan_test("else", DTK_ELSE, 0);
  all_passed &= run_lexan_test("end", DTK_END, 0);
  all_passed &= run_lexan_test("false", DTK_FALSE, 0);
  all_passed &= run_lexan_test("fun", DTK_FUN, 0);
  all_passed &= run_lexan_test("if", DTK_IF, 0);
  all_passed &= run_lexan_test("nil", DTK_NIL, 0);
  all_passed &= run_lexan_test("or", DTK_OR, 0);
  all_passed &= run_lexan_test("true", DTK_TRUE, 0);
  all_passed &= run_lexan_test("import", DTK_IMPORT, 0);
  all_passed &= run_lexan_test("export", DTK_EXPORT, 0);
  all_passed &= run_lexan_test("return", DTK_RETURN, 0);

  printf("\n--- operators & symbols ---\n");
  all_passed &= run_lexan_test("+", DTK_ADD, 0);
  all_passed &= run_lexan_test("-", DTK_SUB, 0);
  all_passed &= run_lexan_test("*", DTK_MUL, 0);
  all_passed &= run_lexan_test("/", DTK_DIV, 0);
  all_passed &= run_lexan_test("=", DTK_EQ, 0);
  all_passed &= run_lexan_test("!", DTK_BNG, 0);
  all_passed &= run_lexan_test(">", DTK_BG, 0);
  all_passed &= run_lexan_test("<", DTK_LS, 0);
  
  all_passed &= run_lexan_test("(", DTK_PAR_OPEN, 0);
  all_passed &= run_lexan_test("{", DTK_CBR_OPEN, 0);
  all_passed &= run_lexan_test("}", DTK_CBR_CLOSE, 0);
  all_passed &= run_lexan_test("[", DTK_BKT_OPEN, 0);
  all_passed &= run_lexan_test(",", DTK_COMMA, 0);
  all_passed &= run_lexan_test(".", DTK_DOT, 0);

  all_passed &= run_lexan_test("==", DTK_DEQ, 0);
  all_passed &= run_lexan_test("!=", DTK_BNG_EQ, 0);
  all_passed &= run_lexan_test(">=", DTK_BE, 0);
  all_passed &= run_lexan_test("<=", DTK_LE, 0);

  printf("\n--- whitespace & comments ---\n");
  all_passed &= run_lexan_test("   \t \n 42", DTK_NUMBER, DNT_DECIMAL);
  all_passed &= run_lexan_test("\n\n\n  foo", DTK_ID, 0);
  
  all_passed &= run_lexan_test("# comment only", DTK_EOF, 0);
  
  printf("\n--- number edge cases ---\n");
  all_passed &= run_lexan_test("0", DTK_NUMBER, DNT_DECIMAL);
  all_passed &= run_lexan_test("007", DTK_NUMBER, DNT_DECIMAL);
  
  if (!all_passed) {
    fprintf(stderr, "\n[CRITICAL] Some failed tests.\n");
    exit(1);
  }

  printf("\n\033[32mAll lexer tests passed!\033[0m\n");
  return 0;
}