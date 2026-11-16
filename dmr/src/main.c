/* dmr - drax manager & resolver */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>

#include "dinstall.h"
#include "dadd.h"
#include "dgraph.h"

#define DMR_VERSION "0.1.0"

static void usage(FILE* o) {
  fprintf(o,
    "dmr " DMR_VERSION " - drax manager & resolver\n"
    "\n"
    "usage: dmr [-C dir] <command> [args]\n"
    "\n"
    "commands:\n"
    "  init             create a default " DMR_MANIFEST " in the current directory\n"
    "  add <spec>       add a dependency to " DMR_MANIFEST " (user/repo[@tag], a git url or a path)\n"
    "  install [spec]   download the dependencies of " DMR_MANIFEST " into ./" DMR_LIBDIR " and write " DMR_LOCKFILE "\n"
    "                   (with a spec: add it first)\n"
    "  update           same, ignoring " DMR_LOCKFILE " (tags and branches are resolved again)\n"
    "  graph [paths]    write the code property graph for LLMs into ./" DMR_GRAPH_DIR "\n"
    "  help             show this help\n"
    "  version          show the version\n"
    "\n"
    "options:\n"
    "  -C dir           run as if dmr was started in dir\n"
    "\n"
    "add / install <spec>:\n"
    "  <spec> is one of\n"
    "    user/repo            GitHub repository (user/repo@v1.0 selects the tag v1.0)\n"
    "    <git url>            https://..., git@host:user/repo.git, file://...\n"
    "    ./dir, ../dir        local path (lib/<name> is a symlink to it)\n"
    "  --tag t              use this tag\n"
    "  --branch b           use this branch\n"
    "  --rev r              use this commit\n"
    "                       (at most one of the three; none: the default branch)\n"
    "  --name n             name under ./" DMR_LIBDIR " (default: the repository or directory name)\n"
    "\n"
    "examples:\n"
    "  dmr init\n"
    "  dmr add drax-lang/griht\n"
    "  dmr add drax-lang/griht@v1.0.0\n"
    "  dmr add https://example.com/x/y.git --branch main\n"
    "  dmr install drax-lang/griht     add and install\n"
    "\n"
    "environment:\n"
    "  DMR_CACHE        git cache directory (default: ~/.cache/dmr)\n");
}

/* init: writes a default package.toml in the current directory. The package
 * name is the name of the directory, restricted to letters, digits, '_' and '-'. */
static int cmd_init(void) {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    fprintf(stderr, "error: cannot get the current directory.\n");
    return 1;
  }

  const char* base = strrchr(cwd, '/');
  base = (base && base[1]) ? base + 1 : "package";

  char name[65];
  size_t n = 0;
  for (; *base && n < sizeof(name) - 1; base++) {
    unsigned char c = (unsigned char) *base;
    name[n++] = (isalnum(c) || c == '_' || c == '-') ? (char) c : '_';
  }
  name[n] = '\0';
  if (n == 0 || name[0] == '-') {
    strcpy(name, "package");
  }

  /* O_EXCL: never overwrite an existing manifest */
  int fd = open(DMR_MANIFEST, O_WRONLY | O_CREAT | O_EXCL, 0644);
  if (fd < 0) {
    if (access(DMR_MANIFEST, F_OK) == 0) {
      fprintf(stderr, "error: " DMR_MANIFEST " already exists.\n");
    } else {
      fprintf(stderr, "error: cannot create " DMR_MANIFEST ".\n");
    }
    return 1;
  }

  FILE* f = fdopen(fd, "w");
  if (f == NULL) { close(fd); return 1; }
  fprintf(f,
    "[package]\n"
    "name = \"%s\"\n"
    "version = \"0.1.0\"\n"
    "\n"
    "[dependencies]\n"
    "# json = { git = \"https://github.com/user/drax-json\", tag = \"v1.0.0\" }\n"
    "# utils = { path = \"../utils\" }\n",
    name);
  if (fclose(f) != 0) {
    fprintf(stderr, "error: cannot write " DMR_MANIFEST ".\n");
    return 1;
  }

  printf("created " DMR_MANIFEST " (%s)\n", name);
  return 0;
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

  if (strcmp(cmd, "init") == 0) return cmd_init();
  if (strcmp(cmd, "add") == 0) return cmd_add(argc - i, argv + i, 0);
  if (strcmp(cmd, "install") == 0) {
    /* install <spec>: add the dependency, then install */
    if (i < argc) return cmd_add(argc - i, argv + i, 1);
    return cmd_install(0);
  }
  if (strcmp(cmd, "update") == 0) {
    if (i < argc) {
      fprintf(stderr, "dmr: 'update' takes no arguments; it resolves what " DMR_MANIFEST " lists again.\n");
      return 1;
    }
    return cmd_install(1);
  }
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
