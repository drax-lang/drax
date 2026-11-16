/* dmr - drax manager & resolver: `dmr add` */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dadd.h"
#include "dinstall.h"
#include "dmanifest.h"
#include "dfs.h"

/* a value written between double quotes in package.toml */
static int toml_safe(const char* s) {
  if (!valid_ref(s)) return 0;
  for (; *s; s++) if (*s == '"' || *s == '\\') return 0;
  return 1;
}

static int is_shorthand(const char* s) {
  const char* slash = strchr(s, '/');
  if (!slash || slash == s || !slash[1] || strchr(slash + 1, '/')) return 0;
  for (; *s; s++) {
    if (*s == '/') continue;
    if (!(isalnum((unsigned char) *s) || *s == '_' || *s == '-' || *s == '.')) return 0;
  }
  return 1;
}

/* name of the dependency: last path segment of the spec, without ".git" */
static char* default_name(const char* spec) {
  size_t n = strlen(spec);
  while (n > 0 && spec[n - 1] == '/') n--;
  size_t start = n;
  while (start > 0 && spec[start - 1] != '/' && spec[start - 1] != ':') start--;
  char* name = xstrdup(spec + start);
  name[n - start] = '\0';
  size_t l = strlen(name);
  if (l > 4 && strcmp(name + l - 4, ".git") == 0) name[l - 4] = '\0';
  return name;
}

static void usage_add(void) {
  fprintf(stderr,
    "usage: dmr add <spec> [--name n] [--tag t | --branch b | --rev r]\n"
    "\n"
    "  user/repo          GitHub repository (user/repo@v1.0 selects the tag v1.0)\n"
    "  <git url>          https://..., git@host:user/repo.git, file://...\n"
    "  ./dir, ../dir      local path (lib/<name> is a symlink to it)\n"
    "\n"
    "options:\n"
    "  --tag t | --branch b | --rev r   version to use (at most one; default: the default branch)\n"
    "  --name n                         name under ./" DMR_LIBDIR " (default: the repository or directory name)\n"
    "\n"
    "The line is written in [dependencies] of " DMR_MANIFEST "; run 'dmr install' to download it\n"
    "(or use 'dmr install <spec>' to do both).\n");
}

/* "[dependencies]", optionally followed by blanks and a comment */
static int is_deps_header(const char* a, const char* b) {
  if ((size_t) (b - a) < 14 || strncmp(a, "[dependencies]", 14) != 0) return 0;
  for (a += 14; a < b; a++) {
    if (*a == '#') return 1;
    if (*a != ' ' && *a != '\t' && *a != '\r') return 0;
  }
  return 1;
}

/* inserts `line` at the end of the [dependencies] section (created when it
 * does not exist) and writes the file */
static int insert_dependency(const char* text, const char* line) {
  size_t n = strlen(text);
  char* out = (char*) malloc(n + strlen(line) + 64);
  if (!out) { fprintf(stderr, "error: out of memory.\n"); return 1; }

  /* find the section */
  const char* p = text;
  const char* sec_end = NULL;    /* where the section ends (start of next [x] or EOF) */
  int found = 0;
  const char* last_content = NULL; /* end of the last non-blank line of the section */

  while (*p) {
    const char* eol = strchr(p, '\n');
    const char* next = eol ? eol + 1 : p + strlen(p);
    const char* a = p;
    while (a < next && (*a == ' ' || *a == '\t')) a++;
    const char* b = eol ? eol : next;
    while (b > a && (b[-1] == ' ' || b[-1] == '\t' || b[-1] == '\r')) b--;

    if (found && !sec_end) {
      if (b > a && *a == '[') sec_end = p;
      else if (b > a) last_content = next;
    } else if (!found && is_deps_header(a, b)) {
      found = 1;
      last_content = next;
    }
    p = next;
  }

  if (found) {
    const char* at = last_content ? last_content : text + n;
    if (sec_end && at > sec_end) at = sec_end;
    size_t head = (size_t) (at - text);
    memcpy(out, text, head);
    size_t o = head;
    if (o > 0 && out[o - 1] != '\n') out[o++] = '\n';
    o += (size_t) sprintf(out + o, "%s\n", line);
    memcpy(out + o, text + head, n - head);
    o += n - head;
    out[o] = '\0';
  } else {
    size_t o = n;
    memcpy(out, text, n);
    if (o > 0 && out[o - 1] != '\n') out[o++] = '\n';
    o += (size_t) sprintf(out + o, "\n[dependencies]\n%s\n", line);
    out[o] = '\0';
  }

  int rc = fs_write(DMR_MANIFEST, out, strlen(out));
  free(out);
  if (rc != 0) { fprintf(stderr, "error: cannot write %s.\n", DMR_MANIFEST); return 1; }
  return 0;
}

int cmd_add(int argc, char** argv, int then_install) {
  const char* spec = NULL;
  const char* name_opt = NULL;
  const char* tag = NULL;
  const char* branch = NULL;
  const char* rev = NULL;

  int i;
  for (i = 0; i < argc; i++) {
    const char** target = NULL;
    if (strcmp(argv[i], "--name") == 0) target = &name_opt;
    else if (strcmp(argv[i], "--tag") == 0) target = &tag;
    else if (strcmp(argv[i], "--branch") == 0) target = &branch;
    else if (strcmp(argv[i], "--rev") == 0) target = &rev;

    if (target) {
      if (i + 1 >= argc) { fprintf(stderr, "dmr: %s needs a value.\n", argv[i]); return 1; }
      *target = argv[++i];
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "dmr: unknown option '%s'.\n\n", argv[i]);
      usage_add();
      return 1;
    } else if (!spec) {
      spec = argv[i];
    } else {
      fprintf(stderr, "dmr: unexpected argument '%s'.\n\n", argv[i]);
      usage_add();
      return 1;
    }
  }

  if (!spec) { usage_add(); return 1; }
  if (!toml_safe(spec)) { fprintf(stderr, "error: invalid spec '%s'.\n", spec); return 1; }

  int rc = 1;
  char* url = NULL;            /* git url */
  char* shorthand_tag = NULL;
  char* base = NULL;           /* what the name comes from */
  char* name = NULL;
  char* line = NULL;
  char* text = NULL;
  int has_manifest_copy = 0;

  int is_path = (spec[0] == '/' || strncmp(spec, "./", 2) == 0 || strncmp(spec, "../", 3) == 0);
  int is_url = (strstr(spec, "://") != NULL || strncmp(spec, "git@", 4) == 0);

  if (is_path) {
    base = xstrdup(spec);
  } else if (is_url) {
    url = xstrdup(spec);
    base = xstrdup(spec);
  } else {
    char* s = xstrdup(spec);
    char* at = strrchr(s, '@');
    if (at) { *at = '\0'; shorthand_tag = xstrdup(at + 1); }
    if (!is_shorthand(s)) {
      fprintf(stderr, "error: cannot understand '%s'.\n\n", spec);
      usage_add();
      free(s);
      goto done;
    }
    url = xfmt("https://github.com/%s", s);
    base = s;
  }

  if (shorthand_tag) {
    if (tag || branch || rev) {
      fprintf(stderr, "error: use either @ref or --tag/--branch/--rev, not both.\n");
      goto done;
    }
    tag = shorthand_tag;
  }

  if ((tag != NULL) + (branch != NULL) + (rev != NULL) > 1) {
    fprintf(stderr, "error: use at most one of --tag, --branch and --rev.\n");
    goto done;
  }
  if (is_path && (tag || branch || rev)) {
    fprintf(stderr, "error: --tag, --branch and --rev do not apply to a path.\n");
    goto done;
  }
  const char* refs[3] = { tag, branch, rev };
  for (i = 0; i < 3; i++) {
    if (refs[i] && !toml_safe(refs[i])) { fprintf(stderr, "error: invalid ref '%s'.\n", refs[i]); goto done; }
  }

  name = name_opt ? xstrdup(name_opt) : default_name(base);
  if (!valid_name(name)) {
    fprintf(stderr, "error: '%s' is not a valid package name (letters, digits, '_' and '-'); use --name.\n", name);
    goto done;
  }

  char err[256];
  dmr_manifest m;
  if (manifest_load(DMR_MANIFEST, &m, err, sizeof(err))) {
    fprintf(stderr, "error: %s: %s\n", DMR_MANIFEST, err);
    if (fs_exists(DMR_MANIFEST) == 0) fprintf(stderr, "run 'dmr init' to create it.\n");
    goto done;
  }
  int dup = -1, same_source = -1;
  for (i = 0; i < m.ndeps; i++) {
    const dmr_dep* d = &m.deps[i];
    if (strcmp(d->name, name) == 0) dup = i;
    if (is_path ? (d->kind == DEP_PATH && d->path && strcmp(d->path, spec) == 0)
                : (d->kind == DEP_GIT && d->git && strcmp(d->git, url) == 0)) same_source = i;
  }
  if (dup >= 0) {
    fprintf(stderr, "error: '%s' is already in %s (line %d)%s\n", name, DMR_MANIFEST, m.deps[dup].line,
            same_source == dup ? "." : ", with another source; use --name for a different name.");
  } else if (same_source >= 0) {
    fprintf(stderr, "error: this source is already in %s as '%s' (line %d).\n", DMR_MANIFEST,
            m.deps[same_source].name, m.deps[same_source].line);
  }
  manifest_free(&m);
  if (dup >= 0 || same_source >= 0) goto done;

  if (is_path) {
    line = xfmt("%s = { path = \"%s\" }", name, spec);
  } else if (tag) {
    line = xfmt("%s = { git = \"%s\", tag = \"%s\" }", name, url, tag);
  } else if (branch) {
    line = xfmt("%s = { git = \"%s\", branch = \"%s\" }", name, url, branch);
  } else if (rev) {
    line = xfmt("%s = { git = \"%s\", rev = \"%s\" }", name, url, rev);
  } else {
    line = xfmt("%s = { git = \"%s\" }", name, url);
  }

  size_t len = 0;
  text = fs_read(DMR_MANIFEST, &len);
  if (!text) { fprintf(stderr, "error: cannot read %s.\n", DMR_MANIFEST); goto done; }
  has_manifest_copy = 1;

  if (insert_dependency(text, line) != 0) goto done;
  printf("added %s\n", line);
  rc = 0;

  if (then_install) {
    rc = cmd_install(0);
    if (rc != 0 && has_manifest_copy) {
      /* a dependency that can not be installed must not stay in the manifest */
      if (fs_write(DMR_MANIFEST, text, len) == 0) {
        fprintf(stderr, "%s restored: '%s' was not added.\n", DMR_MANIFEST, name);
      } else {
        fprintf(stderr, "error: could not restore %s; remove the line of '%s' by hand.\n", DMR_MANIFEST, name);
      }
    }
  }

done:
  free(text); free(line); free(name); free(url); free(base);
  free(shorthand_tag);
  return rc;
}
