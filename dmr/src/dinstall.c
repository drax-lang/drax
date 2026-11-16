/* dmr - drax manager & resolver */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

#include "dinstall.h"
#include "dmanifest.h"
#include "dlock.h"
#include "dfs.h"

#define MARKER ".dmr-rev"

/* ------------------------------------------------------------------ */
/* run state                                                           */
/* ------------------------------------------------------------------ */

typedef struct qitem {
  const dmr_dep* dep;
  char* base;          /* directory of the manifest that declared it */
} qitem;

typedef struct run {
  char* root;
  char* libdir;
  int update;
  int errors;
  lock_file old;
  lock_file now;
  qitem* queue;
  int nq, cq;
  dmr_manifest* manifests;   /* kept alive: the queue points into them */
  int nm;
  char** synced;             /* urls already fetched in this run */
  int nsynced;
} run;

static void error(run* r, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  r->errors++;
}

static void enqueue(run* r, const dmr_manifest* m, const char* base) {
  int i;
  for (i = 0; i < m->ndeps; i++) {
    if (r->nq >= r->cq) {
      r->cq = r->cq == 0 ? 16 : r->cq * 2;
      r->queue = (qitem*) xrealloc(r->queue, sizeof(qitem) * (size_t) r->cq);
    }
    r->queue[r->nq].dep = &m->deps[i];
    r->queue[r->nq].base = xstrdup(base);
    r->nq++;
  }
}

/* ------------------------------------------------------------------ */
/* git (always without a shell)                                        */
/* ------------------------------------------------------------------ */

static char* cache_root(void) {
  const char* c = getenv("DMR_CACHE");
  if (c && *c) return xstrdup(c);
  c = getenv("XDG_CACHE_HOME");
  if (c && *c) return xfmt("%s/dmr", c);
  c = getenv("HOME");
  if (c && *c) return xfmt("%s/.cache/dmr", c);
  return xstrdup("/tmp/dmr-cache");
}

static char* cache_dir_for(const char* url) {
  uint64_t h = 1469598103934665603ULL;
  const char* s;
  for (s = url; *s; s++) { h ^= (unsigned char) *s; h *= 1099511628211ULL; }

  /* readable prefix: the last path component without ".git" */
  const char* last = strrchr(url, '/');
  last = last ? last + 1 : url;
  char base[48];
  size_t n = 0;
  for (; *last && n < sizeof(base) - 1; last++) {
    if (isalnum((unsigned char) *last) || *last == '_' || *last == '-') base[n++] = *last;
    else if (*last == '.') break;
  }
  base[n] = 0;

  char* root = cache_root();
  char* r = xfmt("%s/git/%s-%016llx", root, n ? base : "repo", (unsigned long long) h);
  free(root);
  return r;
}

/* git --git-dir=<cache> args... ; NULL terminated. out may be NULL */
static int git(const char* cache, char** out, ...) {
  char* argv[32];
  int n = 0;
  char* gd = xfmt("--git-dir=%s", cache);
  argv[n++] = (char*) "git";
  argv[n++] = gd;

  va_list ap;
  va_start(ap, out);
  const char* a;
  while (n < 30 && (a = va_arg(ap, const char*)) != NULL) argv[n++] = (char*) a;
  va_end(ap);
  argv[n] = NULL;

  int rc = proc_run(argv, out);
  free(gd);
  return rc;
}

static int sync_repo(run* r, const char* url, const char* cache) {
  int i;
  for (i = 0; i < r->nsynced; i++) {
    if (strcmp(r->synced[i], url) == 0) return 0;
  }

  int rc;
  if (!fs_exists(cache)) {
    char* parent = xstrdup(cache);
    char* slash = strrchr(parent, '/');
    if (slash) *slash = 0;
    fs_mkdir_p(parent);
    free(parent);

    char* argv[] = { (char*) "git", (char*) "clone", (char*) "--bare", (char*) "--quiet",
                     (char*) "--", (char*) url, (char*) cache, NULL };
    rc = proc_run(argv, NULL);
  } else {
    rc = git(cache, NULL, "fetch", "--quiet", "--force", "--tags", "origin",
             "+refs/heads/*:refs/heads/*", NULL);
  }

  if (rc != 0) return 1;

  r->synced = (char**) xrealloc(r->synced, sizeof(char*) * (size_t) (r->nsynced + 1));
  r->synced[r->nsynced++] = xstrdup(url);
  return 0;
}

static int is_sha(const char* s) {
  size_t i, n = strlen(s);
  if (n != 40) return 0;
  for (i = 0; i < n; i++) if (!isxdigit((unsigned char) s[i])) return 0;
  return 1;
}

/* returns a malloc'd full commit sha or NULL */
static char* resolve_rev(const char* cache, const dmr_dep* d) {
  char* spec;
  if (d->tag) spec = xfmt("refs/tags/%s^{commit}", d->tag);
  else if (d->branch) spec = xfmt("refs/heads/%s^{commit}", d->branch);
  else if (d->rev) spec = xfmt("%s^{commit}", d->rev);
  else spec = xstrdup("HEAD^{commit}");

  char* out = NULL;
  int rc = git(cache, &out, "rev-parse", "--verify", "--quiet", spec, NULL);
  free(spec);

  if (rc != 0 || out == NULL || !is_sha(out)) { free(out); return NULL; }
  return out;
}

static int has_commit(const char* cache, const char* rev) {
  if (!fs_exists(cache)) return 0;
  char* spec = xfmt("%s^{commit}", rev);
  int rc = git(cache, NULL, "cat-file", "-e", spec, NULL);
  free(spec);
  return rc == 0;
}

/* ------------------------------------------------------------------ */
/* installing into lib/                                                */
/* ------------------------------------------------------------------ */

/* installed by dmr: it left a marker, or it is a link the lock knows about.
 * A directory that only has the same name is the user's. */
static int is_managed(run* r, const char* dest, const char* name) {
  char* marker = fs_join(dest, MARKER);
  int m = fs_exists(marker) || (fs_is_symlink(dest) && lock_find(&r->old, name) != NULL);
  free(marker);
  return m;
}

/* makes room on lib/<name>; refuses to touch what dmr did not install */
static int clear_dest(run* r, const char* dest, const char* name) {
  if (!fs_exists(dest)) return 0;
  if (!is_managed(r, dest, name)) {
    error(r, "'%s/%s' already exists and was not installed by dmr (move it or rename the dependency).",
          DMR_LIBDIR, name);
    return 1;
  }
  if (fs_rm_tree(dest) != 0) {
    error(r, "cannot replace '%s/%s'.", DMR_LIBDIR, name);
    return 1;
  }
  return 0;
}

static int install_git(run* r, const dmr_dep* d, const char* cache, const char* rev) {
  char* dest = fs_join(r->libdir, d->name);
  char* marker = fs_join(dest, MARKER);
  int rc = 1;

  char* have = fs_read(marker, NULL);
  if (have && strcmp(have, rev) == 0) {
    printf("  = %s (up to date)\n", d->name);
    free(have); free(marker); free(dest);
    return 0;
  }
  free(have);

  if (clear_dest(r, dest, d->name)) goto done;
  if (fs_mkdir_p(dest) != 0) { error(r, "cannot create '%s'.", dest); goto done; }

  char* tar = xfmt("%s/dmr-%s.tar", cache, rev);
  char* a1[] = { (char*) "git", NULL, (char*) "archive", (char*) "--format=tar",
                 (char*) "-o", tar, (char*) rev, NULL };
  char* gd = xfmt("--git-dir=%s", cache);
  a1[1] = gd;
  int c1 = proc_run(a1, NULL);
  free(gd);

  int c2 = -1;
  if (c1 == 0) {
    char* a2[] = { (char*) "tar", (char*) "-xf", tar, (char*) "-C", dest, NULL };
    c2 = proc_run(a2, NULL);
  }
  unlink(tar);
  free(tar);

  if (c1 != 0 || c2 != 0) {
    error(r, "failed to extract '%s' (%s).", d->name, rev);
    fs_rm_tree(dest);
    goto done;
  }

  if (fs_write(marker, rev, strlen(rev)) != 0) {
    error(r, "cannot write '%s'.", marker);
    goto done;
  }

  const char* ref = d->tag ? d->tag : d->branch ? d->branch : d->rev ? d->rev : "HEAD";
  printf("  + %s (%s) %.7s\n", d->name, ref, rev);
  rc = 0;

done:
  free(marker);
  free(dest);
  return rc;
}

static int install_path(run* r, const dmr_dep* d, const char* target) {
  char* dest = fs_join(r->libdir, d->name);
  int rc = 1;

  if (fs_is_symlink(dest)) {
    char buf[PATH_MAX];
    ssize_t n = readlink(dest, buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = 0;
      if (strcmp(buf, target) == 0) {
        printf("  = %s (path, up to date)\n", d->name);
        free(dest);
        return 0;
      }
    }
    unlink(dest);
  } else if (clear_dest(r, dest, d->name)) {
    free(dest);
    return 1;
  }

  if (symlink(target, dest) != 0) {
    error(r, "cannot link '%s' -> '%s'.", dest, target);
  } else {
    printf("  + %s (path) %s\n", d->name, target);
    rc = 0;
  }
  free(dest);
  return rc;
}

/* ------------------------------------------------------------------ */
/* resolving                                                           */
/* ------------------------------------------------------------------ */

static char* requested_of(const dmr_dep* d) {
  if (d->tag) return xfmt("tag:%s", d->tag);
  if (d->branch) return xfmt("branch:%s", d->branch);
  if (d->rev) return xfmt("rev:%s", d->rev);
  return xstrdup("head");
}

static void load_children(run* r, const char* pkgdir, lock_pkg* node) {
  char* mf = fs_join(pkgdir, DMR_MANIFEST);
  if (!fs_exists(mf)) { free(mf); return; }

  dmr_manifest m;
  char err[256];
  if (manifest_load(mf, &m, err, sizeof(err))) {
    error(r, "%s: %s", mf, err);
    free(mf);
    return;
  }
  free(mf);

  r->manifests = (dmr_manifest*) xrealloc(r->manifests, sizeof(dmr_manifest) * (size_t) (r->nm + 1));
  r->manifests[r->nm] = m;
  dmr_manifest* kept = &r->manifests[r->nm++];

  /* the queue points into kept->deps (a separate heap block): it stays
   * valid when the manifests array grows. */
  int i;
  for (i = 0; i < kept->ndeps; i++) lock_pkg_dep(node, kept->deps[i].name);
  enqueue(r, kept, pkgdir);
}

static void process(run* r, const qitem* it) {
  const dmr_dep* d = it->dep;

  if (d->kind == DEP_REGISTRY_DEFAULT || d->kind == DEP_REGISTRY_CUSTOM) {
    error(r, "dependency '%s': registry dependencies are not supported yet (use 'git' or 'path').", d->name);
    return;
  }

  char* source;
  char* requested = NULL;
  char* abs_path = NULL;

  if (d->kind == DEP_GIT) {
    source = xfmt("git+%s", d->git);
    requested = requested_of(d);
  } else {
    char* joined = fs_join(it->base, d->path);
    abs_path = fs_realpath(joined);
    free(joined);
    if (abs_path == NULL || !fs_is_dir(abs_path)) {
      error(r, "dependency '%s': path '%s' is not a directory.", d->name, d->path);
      free(abs_path);
      return;
    }
    char* rel = fs_relpath(r->root, abs_path);
    source = xfmt("path+%s", rel);
    free(rel);
  }

  /* same name twice: the same source is fine (diamond), a different one is a conflict */
  lock_pkg* seen = lock_find(&r->now, d->name);
  if (seen) {
    int same = strcmp(seen->source, source) == 0 &&
               ((seen->requested == NULL && requested == NULL) ||
                (seen->requested && requested && strcmp(seen->requested, requested) == 0));
    if (!same) {
      error(r, "dependency '%s' is required with different sources:\n         %s %s\n         %s %s",
            d->name, seen->source, seen->requested ? seen->requested : "", source, requested ? requested : "");
    }
    free(source); free(requested); free(abs_path);
    return;
  }

  char* pkgdir = NULL;
  char* rev = NULL;

  if (d->kind == DEP_GIT) {
    char* cache = cache_dir_for(d->git);
    lock_pkg* locked = r->update ? NULL : lock_find(&r->old, d->name);
    int use_lock = locked && locked->rev && locked->requested &&
                   strcmp(locked->source, source) == 0 &&
                   strcmp(locked->requested, requested) == 0 &&
                   is_sha(locked->rev);

    if (use_lock) {
      rev = xstrdup(locked->rev);
      if (!has_commit(cache, rev)) {
        if (sync_repo(r, d->git, cache) != 0 || !has_commit(cache, rev)) {
          error(r, "dependency '%s': cannot fetch the locked revision %.7s from '%s'.", d->name, rev, d->git);
          free(rev); free(cache); free(source); free(requested);
          return;
        }
      }
    } else {
      if (sync_repo(r, d->git, cache) != 0) {
        error(r, "dependency '%s': cannot download '%s'.", d->name, d->git);
        free(cache); free(source); free(requested);
        return;
      }
      rev = resolve_rev(cache, d);
      if (rev == NULL) {
        error(r, "dependency '%s': '%s' not found in '%s'.", d->name,
              d->tag ? d->tag : d->branch ? d->branch : d->rev ? d->rev : "HEAD", d->git);
        free(cache); free(source); free(requested);
        return;
      }
    }

    if (install_git(r, d, cache, rev) != 0) {
      free(rev); free(cache); free(source); free(requested);
      return;
    }
    pkgdir = fs_join(r->libdir, d->name);
    free(cache);
  } else {
    if (install_path(r, d, abs_path) != 0) {
      free(source); free(abs_path);
      return;
    }
    pkgdir = xstrdup(abs_path);
  }

  lock_pkg* node = lock_add(&r->now, d->name);
  node->source = source;
  node->requested = requested;
  node->rev = rev;

  load_children(r, pkgdir, node);
  free(pkgdir);
  free(abs_path);
}

/* ------------------------------------------------------------------ */
/* entry                                                               */
/* ------------------------------------------------------------------ */

static void remove_stale(run* r) {
  int i;
  for (i = 0; i < r->old.n; i++) {
    const char* name = r->old.pkgs[i].name;
    if (lock_find(&r->now, name) != NULL) continue;
    if (!valid_name(name)) continue; /* a tampered lock never reaches the file system */

    char* dest = fs_join(r->libdir, name);
    if (fs_exists(dest) && is_managed(r, dest, name)) {
      if (fs_rm_tree(dest) == 0) printf("  - %s (not used anymore)\n", name);
    }
    free(dest);
  }
}

int cmd_install(int update) {
  run r;
  memset(&r, 0, sizeof(r));
  r.update = update;

  /* never wait for a password on a terminal: a private repository fails */
  setenv("GIT_TERMINAL_PROMPT", "0", 1);

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) { fprintf(stderr, "error: cannot get the current directory.\n"); return 1; }
  char* real_cwd = fs_realpath(cwd);
  r.root = real_cwd ? real_cwd : xstrdup(cwd);
  r.libdir = fs_join(r.root, DMR_LIBDIR);

  char err[256];
  dmr_manifest root;
  if (manifest_load(DMR_MANIFEST, &root, err, sizeof(err))) {
    fprintf(stderr, "error: %s: %s\n", DMR_MANIFEST, err);
    if (access(DMR_MANIFEST, F_OK) != 0) {
      fprintf(stderr, "run 'dmr init' to create it.\n");
    }
    return 1;
  }

  if (lock_load(DMR_LOCKFILE, &r.old, err, sizeof(err))) {
    fprintf(stderr, "error: %s: %s\n", DMR_LOCKFILE, err);
    manifest_free(&root);
    return 1;
  }

  if (root.ndeps > 0 && fs_mkdir_p(r.libdir) != 0) {
    fprintf(stderr, "error: cannot create '%s'.\n", DMR_LIBDIR);
    return 1;
  }

  printf("%s %s\n", update ? "updating" : "installing", root.name ? root.name : "package");

  /* breadth first; the queue grows with the dependencies of dependencies */
  enqueue(&r, &root, r.root);
  int i;
  for (i = 0; i < r.nq; i++) process(&r, &r.queue[i]);

  int rc = 0;
  if (r.errors > 0) {
    fprintf(stderr, "%d error(s): %s was not changed.\n", r.errors, DMR_LOCKFILE);
    rc = 1;
  } else {
    remove_stale(&r);
    if (lock_save(DMR_LOCKFILE, &r.now) != 0) {
      fprintf(stderr, "error: cannot write %s.\n", DMR_LOCKFILE);
      rc = 1;
    } else {
      printf("done: %d package(s) in %s/, %s written.\n", r.now.n, DMR_LIBDIR, DMR_LOCKFILE);
    }
  }

  /* the dev tool exits right after: only what is cheap to release */
  manifest_free(&root);
  lock_free(&r.old);
  lock_free(&r.now);
  return rc;
}
