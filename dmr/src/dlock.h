/* dmr - drax manager & resolver
 *
 * package.lock: what was resolved, so the next install is reproducible.
 * It is generated (sorted by name, no timestamps): do not edit it.
 *
 *   version = 1
 *
 *   [[package]]
 *   name = "http"
 *   source = "git+https://github.com/user/drax-http"
 *   requested = "tag:v0.4.1"        (tag:x | branch:x | rev:x | head)
 *   rev = "<full commit sha>"
 *   dependencies = ["json"]
 */
#ifndef __DMR_LOCK
#define __DMR_LOCK

typedef struct lock_pkg {
  char* name;
  char* source;      /* "git+<url>" or "path+<relative path>" */
  char* requested;   /* NULL for path packages */
  char* rev;         /* NULL for path packages */
  char** deps;
  int ndeps;
} lock_pkg;

typedef struct lock_file {
  lock_pkg* pkgs;
  int n;
  int cap;
} lock_file;

/* a missing file is an empty lock (returns 0). 1 on a malformed file. */
int lock_load(const char* path, lock_file* l, char* err, int errsz);
int lock_save(const char* path, lock_file* l);
lock_pkg* lock_add(lock_file* l, const char* name);
lock_pkg* lock_find(lock_file* l, const char* name);
void lock_pkg_dep(lock_pkg* p, const char* dep);
void lock_free(lock_file* l);

#endif
