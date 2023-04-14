#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "dparser.h"
#include "dvm.h"
#include "dflags.h"
#include "dio.h"

#ifdef _B_BUILF_FULL
  #include <editline/readline.h>
#else
  #include "dshell.h"
#endif

static int interactive_shell(d_vm* v) {
  initial_info();
  
  while (1) {
    #ifdef _B_BUILF_FULL
      char* input = readline("> ");
      add_history(input);
    #else
      char* input = b_read_content();
    #endif
    
    __build__(v, input);
    __run__(v, 1);

    free(input);
  }
  return 0;
}

static int process_file(d_vm* v, char** argv) {
  char * content = 0;
  char * path = argv[1];
  if(get_file_content(path, &content)) {
    // bprint(new_error(BFILE_NOT_FOUND, "fail to process '%s' file.", path));
    // bbreak_line();
    return 1;
  }

  __build__(v, content);
  __run__(v, 0);

  return 0;
}

int main(int argc, char** argv) {

  bimode bmode = get_bimode(argc, argv);
  // drax_env* benv = new_env(); // remove this env

  // create init vm
  d_vm* gdvm = createVM();
  
  // load_builtin_functions(&benv);
  switch (bmode) {
    case BI_PROCESS_DEFAULT:
      return process_file(gdvm, argv);

    case BI_INTERACTIVE_DEFAULT:
      interactive_shell(gdvm);
      break;

    default:
      break;
  }

  return 0;
}