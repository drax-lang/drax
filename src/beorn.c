#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "bparser.h"
#include "bvm.h"
#include "bprint.h"
#include "bfunctions.h"
#include "bflags.h"
#include "bio.h"
#include "beorn.h"

#ifdef _B_BUILF_FULL
  #include <editline/readline.h>
#else
  #include "bshell.h"
#endif

int interactive_shell(beorn_env* benv) {
  initial_info();
  
  while (1) {
    #ifdef _B_BUILF_FULL
      char* input = readline("> ");
      add_history(input);
    #else
      char* input = b_read_content();
    #endif

    beorn_state* out = beorn_parser(input);

    __run__(benv, out, 1);
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
  __run__(benv, out, 0);

  return 0;
}

int main(int argc, char** argv) {

  bimode bmode = get_bimode(argc, argv);
  beorn_env* benv = new_env();
  
  load_builtin_functions(&benv);
  switch (bmode) {
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