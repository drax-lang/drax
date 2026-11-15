/* dmr - drax manager & resolver
 *
 * package.toml: the manifest of a package. A small TOML subset:
 *
 *   [package]                  name, version
 *   [dependencies]             name = "1.2.0"
 *                              name = { git = "url", tag|branch|rev = "x" }
 *                              name = { registry = "url", version = "x" }
 *                              name = { path = "../dir" }
 *
 * Comments (#) and blank lines are accepted. Inline tables are one line.
 */
#ifndef __DMR_MANIFEST
#define __DMR_MANIFEST

typedef enum dep_kind {
  DEP_REGISTRY_DEFAULT,   /* name = "1.2.0": the official registry (does not exist yet) */
  DEP_REGISTRY_CUSTOM,    /* { registry = "url", version = "x" } */
  DEP_GIT,
  DEP_PATH
} dep_kind;

typedef struct dmr_dep {
  char* name;
  dep_kind kind;
  char* version;
  char* registry;
  char* git;
  char* tag;
  char* branch;
  char* rev;
  char* path;
  int line;
} dmr_dep;

typedef struct dmr_manifest {
  char* name;
  char* version;
  dmr_dep* deps;
  int ndeps;
} dmr_manifest;

/* 0 ok; 1 error (message on err). A missing file is an error too. */
int manifest_load(const char* path, dmr_manifest* m, char* err, int errsz);
void manifest_free(dmr_manifest* m);

/* names become directory names: [A-Za-z0-9_-], at most 64, no leading '-' */
int valid_name(const char* s);
/* values handed to git: not empty, no leading '-', no control chars */
int valid_ref(const char* s);

#endif
