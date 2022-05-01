#include <stdio.h>
#include <stdlib.h>
#include "bparser.h"
#include "bvm.h"
#include "bprint.h"
#include "flags.c"
#include <editline/readline.h>

void interactive_shell() {
  beorn_env* benv = malloc(sizeof(beorn_state));
  initial_info();
  
  while (1) {
    char* input = readline("> ");
    add_history(input);
    beorn_state* out = beorn_parser(input);

    for (size_t i = 0; i < out->length; i++) {
      beorn_state* evaluated = process(benv, out->child[i]);
      bprint(evaluated);
      putchar('\n');
    }

    free(out);
    free(input);
  }
}

int main(int argc, char** argv) {

  if (argc == 1) {
    interactive_shell();
  }

  return 0;
}