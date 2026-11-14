#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "dparser.h"
#include "dvm.h"
#include "dflags.h"
#include "dio.h"

#ifdef _B_BUILF_FULL
  #ifdef _WIN32
    #include <windows.h>
    #include "dshell.h"
  #else
    #include <editline/readline.h>
  #endif
#else
  #include "dshell.h"
#endif

int is_teractive_mode = 0;

/**
 * The VM is destroyed when the process ends, also when the program
 * calls Os.exit (exit() runs the atexit handlers).
 */
static d_vm* main_vm = NULL;

static void destroy_main_vm(void) {
  d_vm* vm = main_vm;
  main_vm = NULL;
  __destroy__(vm);
}

static int interactive_shell(d_vm* v) {
  is_teractive_mode = 1;
  initial_info();
  
  while (1) {
    #ifdef _B_BUILF_FULL
      char* input = readline("> ");
      if (input == NULL) break;
      add_history(input);
    #else
      char* input = b_read_content();
      if (input == NULL) break;
    #endif
    
    if (__build__(v, input, NULL)) {
      __run__(v, 1);
    }

    __reset__(v);
    free(input);
  }
  return 0;
}

static int process_file(d_vm* v, char** argv) {
  is_teractive_mode = 0;
  int stat = 0;
  char* content = 0;
  char* path = argv[1];

  if(get_file_content(NULL, path, &content)) {
    printf("file '%s' not found.\n", path);
    return 1;
  }

  if (__build__(v, content, path)) {
    stat = __run__(v, 0);
  } else {
    stat = 1;
  }

  __reset__(v);
  free(content);

  return stat;
}

int main(int argc, char** argv) {
  bimode bmode = get_bimode(argc, argv);

  /* create init vm */
  d_vm* gdvm = createMainVM();
  main_vm = gdvm;
  atexit(destroy_main_vm);
  
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
