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
  printf(" usage: drax [options] [script.dx]\n\
\n\
  -h, --help             show this help\n\
  -v, --version          show version\n\
\n\
 Safe mode: by default a program may do everything. With --safe it can do\n\
 only what the --allow-* options grant:\n\
\n");
  printf("%s", dperm_options_help());
  printf("\n example: drax --safe --allow-run --allow-native=./greet.so script.dx\n");
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

bimode get_bimode(int argc, char** argv, d_perms* perms, int* script) {
  *script = 0;

  if (argc <= 1) return BI_INTERACTIVE_DEFAULT;

  int i;
  for (i = 1; i < argc; i++) {
    if (non_flag(argv[i])) {
      *script = i;

      /* an option after the script would be ignored, and --safe ignored is a program that is not safe */
      int j;
      for (j = i + 1; j < argc; j++) {
        d_perms scratch;
        char ignored[64];
        dperm_init(&scratch);
        int r = dperm_parse_option(&scratch, argv[j], ignored, sizeof(ignored));
        dperm_free(&scratch);

        if (r != 0) {
          fprintf(stderr, "drax: the option '%s' is after the script and would be ignored. "
                          "Options go before the script (example: drax --safe %s)\n", argv[j], argv[i]);
          return BI_ERROR;
        }
      }

      return BI_PROCESS_DEFAULT;
    }

    if (argcmp('h', "--help", argv[i]) == 0) { help_app(); return BI_NONE; }
    if (argcmp('v', "--version", argv[i]) == 0) { version_app(); return BI_NONE; }

    char err[512];
    int r = dperm_parse_option(perms, argv[i], err, sizeof(err));

    if (r == 1) continue;

    if (r == -1) {
      fprintf(stderr, "%s\n", err);
    } else {
      fprintf(stderr, "drax: unknown option '%s'. Run 'drax --help' to see the options.\n", argv[i]);
    }
    return BI_ERROR;
  }

  /* only options (drax --safe): the interactive shell */
  return BI_INTERACTIVE_DEFAULT;
}
