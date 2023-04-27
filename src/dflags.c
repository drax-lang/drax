/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#include "dflags.h"

void initial_info() {
  printf("Drax (%s) (%s)\n", drax_VM_VERSION, drax_LIB_VERSION);
}

static void version_app() {
  printf("Drax Language (%s) (%s)\n", drax_VM_VERSION, drax_LIB_VERSION);
}

static void help_app() {
  printf(" usage drax [option] [args] \n\
  -h, --help     show this help\n\
  -v, --version  show version\
  \n");
}

static int non_flag(char * name) {
  if (name[0] != '-') return 1;

  return 0;
}

static int argcmp(char sname, const char * name, char * arg) {
  if (arg[0] == '-') {
    if (arg[1] == '-') {
      return strcmp(name, arg);
    }

    if (arg[1] == sname) return 0;
  }

  return 1;
}

bimode get_bimode(int argc, char** argv) {
  #define _call_opt(s, o, v, fn) if (argcmp(s, o, v) == 0) fn();

  if (argc <= 1) return BI_INTERACTIVE_DEFAULT;

  int i;
  for (i = 1; i < argc; i++) {
    if (non_flag(argv[i])) return BI_PROCESS_DEFAULT;

    _call_opt('h', "--help", argv[i], help_app);
    _call_opt('v', "--version", argv[i], version_app);

  }

  return BI_NONE;
}
