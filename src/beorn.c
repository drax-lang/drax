#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <editline/readline.h>
#include "bparser.h"
#include "bvm.h"
#include "bprint.h"
#include "bfunctions.h"
#include "bflags.h"
#include "bio.h"
#include "beorn.h"

int interactive_shell(beorn_env* benv) {
  initial_info();
  
  while (1) {
    char* input = readline("> ");
    add_history(input);
    beorn_state* out = beorn_parser(input);

    if (out->type == BT_ERROR) {
      bprint(out);
      putchar('\n');
    } else {
      for (int i = 0; i < out->length; i++) {
        beorn_state* evaluated = process(benv, out->child[i]);
        bprint(evaluated);
        bbreak_line();
      }
      del_bstate(out);
    }

    free(input);
  }
  return 0;
}

int process_file(beorn_env* benv, char** argv) {
  char * content = 0;
  char * path = argv[1];
  if(get_file_content(path, &content)) {
    bprint(new_error(BFILE_NOT_FOUND, "fail to process '%s' file.", path));
    bbreak_line();
    return 1;
  }

  beorn_state* out = beorn_parser(content);

  if (out->type == BT_ERROR) {
    bprint(out);
    bbreak_line();
  } else {
    for (int i = 0; i < out->length; i++) {
        beorn_state* evaluated = process(benv, out->child[i]);
        if (evaluated->type == BT_ERROR) bprint(evaluated);
    }
    del_bstate(out);
  }

  return 0;
}

int main(int argc, char** argv) {

  bimode bmode = get_bimode(argc, argv);
  beorn_env* benv = new_env();
  
  load_buildtin_functions(&benv);
  switch (bmode)
  {
  case BI_PROCESS_DEFAULT:
    return process_file(benv, argv);

  case BI_INTERACTIVE_DEFAULT:
    interactive_shell(benv);
    break;

  default:
    break;
  }

  return 0;
}