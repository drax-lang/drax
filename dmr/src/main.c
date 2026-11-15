/* dmr - drax manager & resolver */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dinstall.h"
#include "dgraph.h"

#define DMR_VERSION "0.1.0"

static void usage(FILE* o) {
  fprintf(o,
    "dmr " DMR_VERSION " - drax manager & resolver\n"
    "\n"
    "usage: dmr [-C dir] <command> [args]\n"
    "\n"
    "commands:\n"
    "  install          download the dependencies of " DMR_MANIFEST " into ./" DMR_LIBDIR " and write " DMR_LOCKFILE "\n"
    "  update           same, ignoring " DMR_LOCKFILE " (tags and branches are resolved again)\n"
    "  graph [paths]    write the code property graph for LLMs into ./" DMR_GRAPH_DIR "\n"
    "  help             show this help\n"
    "  version          show the version\n"
    "\n"
    "options:\n"
    "  -C dir           run as if dmr was started in dir\n"
    "\n"
    "environment:\n"
    "  DMR_CACHE        git cache directory (default: ~/.cache/dmr)\n");
}

int main(int argc, char** argv) {
  int i = 1;

  if (i + 1 < argc && strcmp(argv[i], "-C") == 0) {
    if (chdir(argv[i + 1]) != 0) {
      fprintf(stderr, "dmr: cannot enter '%s'.\n", argv[i + 1]);
      return 1;
    }
    i += 2;
  }

  if (i >= argc) { usage(stderr); return 1; }
  const char* cmd = argv[i++];

  if (strcmp(cmd, "install") == 0) return cmd_install(0);
  if (strcmp(cmd, "update") == 0) return cmd_install(1);
  if (strcmp(cmd, "graph") == 0) return cmd_graph(argc - i, argv + i);
  if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0) {
    printf("dmr " DMR_VERSION "\n");
    return 0;
  }
  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
    usage(stdout);
    return 0;
  }

  fprintf(stderr, "dmr: unknown command '%s'.\n\n", cmd);
  usage(stderr);
  return 1;
}
