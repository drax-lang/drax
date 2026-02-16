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

    if (!all_passed) {
        fprintf(stderr, "\n[CRITICAL] Some failed tests.\n");
        exit(1);
    }

    printf("\n[SUCCESS] All tests passed.\n");
    return 0;
}