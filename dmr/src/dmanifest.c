/* dmr - drax manager & resolver */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dmanifest.h"
#include "dfs.h"

int valid_name(const char* s) {
  size_t n = strlen(s), i;
  if (n == 0 || n > 64 || s[0] == '-') return 0;
  for (i = 0; i < n; i++) {
    if (!(isalnum((unsigned char) s[i]) || s[i] == '_' || s[i] == '-')) return 0;
  }
  return 1;
}

int valid_ref(const char* s) {
  size_t i;
  if (s[0] == 0 || s[0] == '-') return 0;
  for (i = 0; s[i]; i++) {
    if ((unsigned char) s[i] < 0x20 || s[i] == 0x7f) return 0;
  }
  return 1;
}

typedef struct cur {
  const char* p;
  int line;
  char* err;
  int errsz;
} cur;

static int fail(cur* c, const char* msg) {
  snprintf(c->err, (size_t) c->errsz, "line %d: %s", c->line, msg);
  return 1;
}

static void skip_ws(cur* c) {
  while (*c->p == ' ' || *c->p == '\t') c->p++;
}

/* "basic" (with escapes) or 'literal' string. result is malloc'd */
static int parse_string(cur* c, char** out) {
  char q = *c->p;
  ++c->p;
  size_t cap = 32, n = 0;
  char* buf = (char*) xrealloc(NULL, cap);

  while (*c->p && *c->p != q) {
    char ch = *c->p;
    if (q == '"' && ch == '\\') {
      c->p++;
      switch (*c->p) {
        case 'n': ch = '\n'; break;
        case 't': ch = '\t'; break;
        case '"': ch = '"';  break;
        case '\\': ch = '\\'; break;
        default: free(buf); return fail(c, "invalid escape in string.");
      }
    }
    if (n + 2 >= cap) { cap *= 2; buf = (char*) xrealloc(buf, cap); }
    buf[n++] = ch;
    c->p++;
  }
  if (*c->p != q) { free(buf); return fail(c, "unterminated string."); }
  c->p++;
  buf[n] = 0;
  *out = buf;
  return 0;
}

static int parse_bare_key(cur* c, char** out) {
  const char* s = c->p;
  while (isalnum((unsigned char) *c->p) || *c->p == '_' || *c->p == '-') c->p++;
  if (c->p == s) return fail(c, "expected a key.");
  size_t n = (size_t) (c->p - s);
  char* k = (char*) xrealloc(NULL, n + 1);
  memcpy(k, s, n);
  k[n] = 0;
  *out = k;
  return 0;
}

static void set_field(char** field, char* v) {
  free(*field);
  *field = v;
}

static int parse_inline_table(cur* c, dmr_dep* d) {
  c->p++; /* { */
  for (;;) {
    skip_ws(c);
    if (*c->p == '}') { c->p++; return 0; }

    char* key = NULL;
    char* val = NULL;
    if (parse_bare_key(c, &key)) return 1;
    skip_ws(c);
    if (*c->p != '=') { free(key); return fail(c, "expected '=' after the key."); }
    c->p++;
    skip_ws(c);
    if (*c->p != '"' && *c->p != '\'') { free(key); return fail(c, "the value must be a string."); }
    if (parse_string(c, &val)) { free(key); return 1; }

    char** field = NULL;
    if (strcmp(key, "git") == 0) field = &d->git;
    else if (strcmp(key, "tag") == 0) field = &d->tag;
    else if (strcmp(key, "branch") == 0) field = &d->branch;
    else if (strcmp(key, "rev") == 0) field = &d->rev;
    else if (strcmp(key, "registry") == 0) field = &d->registry;
    else if (strcmp(key, "version") == 0) field = &d->version;
    else if (strcmp(key, "path") == 0) field = &d->path;

    if (field == NULL) {
      char msg[160];
      snprintf(msg, sizeof(msg), "unknown dependency key '%.60s'.", key);
      free(key); free(val);
      return fail(c, msg);
    }
    set_field(field, val);
    free(key);

    skip_ws(c);
    if (*c->p == ',') { c->p++; continue; }
    if (*c->p == '}') { c->p++; return 0; }
    return fail(c, "expected ',' or '}' (inline tables must be on one line).");
  }
}

static int validate_dep(cur* c, dmr_dep* d, int simple) {
  if (!valid_name(d->name)) {
    return fail(c, "invalid dependency name (use letters, digits, '_' and '-').");
  }

  if (simple) {
    d->kind = DEP_REGISTRY_DEFAULT;
    return 0;
  }

  int nsrc = (d->git != NULL) + (d->registry != NULL) + (d->path != NULL);
  if (nsrc != 1) {
    return fail(c, "a dependency needs exactly one of 'git', 'registry' or 'path'.");
  }

  if (d->git) {
    d->kind = DEP_GIT;
    int nref = (d->tag != NULL) + (d->branch != NULL) + (d->rev != NULL);
    if (nref > 1) return fail(c, "use only one of 'tag', 'branch' or 'rev'.");
    if (!valid_ref(d->git)) return fail(c, "invalid git url.");
    if (d->tag && !valid_ref(d->tag)) return fail(c, "invalid tag.");
    if (d->branch && !valid_ref(d->branch)) return fail(c, "invalid branch.");
    if (d->rev && !valid_ref(d->rev)) return fail(c, "invalid rev.");
    if (d->version || d->registry) return fail(c, "'version' is not used by git dependencies.");
  } else if (d->registry) {
    d->kind = DEP_REGISTRY_CUSTOM;
    if (!d->version) return fail(c, "a registry dependency needs a 'version'.");
    if (d->tag || d->branch || d->rev) return fail(c, "'tag', 'branch' and 'rev' are only for git.");
  } else {
    d->kind = DEP_PATH;
    if (d->path[0] == 0) return fail(c, "empty path.");
    if (d->version || d->tag || d->branch || d->rev) return fail(c, "a path dependency has no version or ref.");
  }
  return 0;
}

static char* trim_line(char* s) {
  char* e;
  while (*s == ' ' || *s == '\t' || *s == '\r') s++;
  e = s + strlen(s);
  while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r')) *--e = 0;
  return s;
}

/* cuts the comment, ignoring '#' inside strings */
static void cut_comment(char* s) {
  char q = 0;
  for (; *s; s++) {
    if (q) {
      if (*s == '\\' && q == '"' && s[1]) s++;
      else if (*s == q) q = 0;
    } else if (*s == '"' || *s == '\'') {
      q = *s;
    } else if (*s == '#') {
      *s = 0;
      return;
    }
  }
}

int manifest_load(const char* path, dmr_manifest* m, char* err, int errsz) {
  memset(m, 0, sizeof(*m));

  char* text = fs_read(path, NULL);
  if (text == NULL) {
    snprintf(err, (size_t) errsz, "cannot read '%s'.", path);
    return 1;
  }

  enum { S_NONE, S_PACKAGE, S_DEPS, S_OTHER } section = S_NONE;
  cur c = { NULL, 0, err, errsz };
  char* raw;
  int cap = 0;
  int rc = 0;

  /* strtok_r skips empty lines, so the lines are counted by hand */
  char* p = text;
  while (*p && rc == 0) {
    char* eol = strchr(p, '\n');
    if (eol) *eol = 0;
    c.line++;
    raw = p;
    p = eol ? eol + 1 : p + strlen(p);

    cut_comment(raw);
    char* line = trim_line(raw);
    if (*line == 0) continue;

    if (line[0] == '[') {
      size_t n = strlen(line);
      if (line[n - 1] != ']') { rc = fail(&c, "invalid section header."); break; }
      line[n - 1] = 0;
      const char* sec = line + 1;
      if (strcmp(sec, "package") == 0) section = S_PACKAGE;
      else if (strcmp(sec, "dependencies") == 0) section = S_DEPS;
      else section = S_OTHER;
      continue;
    }

    if (section == S_NONE || section == S_OTHER) continue;

    c.p = line;
    char* key = NULL;
    if (parse_bare_key(&c, &key)) { rc = 1; break; }
    skip_ws(&c);
    if (*c.p != '=') { free(key); rc = fail(&c, "expected '='."); break; }
    c.p++;
    skip_ws(&c);

    if (section == S_PACKAGE) {
      char* val = NULL;
      if (*c.p != '"' && *c.p != '\'') { free(key); rc = fail(&c, "the value must be a string."); break; }
      if (parse_string(&c, &val)) { free(key); rc = 1; break; }
      if (strcmp(key, "name") == 0) set_field(&m->name, val);
      else if (strcmp(key, "version") == 0) set_field(&m->version, val);
      else free(val);
      free(key);
      continue;
    }

    /* [dependencies] */
    if (m->ndeps >= cap) {
      cap = cap == 0 ? 8 : cap * 2;
      m->deps = (dmr_dep*) xrealloc(m->deps, sizeof(dmr_dep) * (size_t) cap);
    }
    dmr_dep* d = &m->deps[m->ndeps];
    memset(d, 0, sizeof(*d));
    d->name = key;
    d->line = c.line;
    m->ndeps++;

    int simple = 0;
    if (*c.p == '"' || *c.p == '\'') {
      simple = 1;
      if (parse_string(&c, &d->version)) { rc = 1; break; }
    } else if (*c.p == '{') {
      if (parse_inline_table(&c, d)) { rc = 1; break; }
    } else {
      rc = fail(&c, "the value must be a version string or an inline table.");
      break;
    }

    skip_ws(&c);
    if (*c.p != 0) { rc = fail(&c, "unexpected text after the value."); break; }

    if (validate_dep(&c, d, simple)) { rc = 1; break; }

    int i;
    for (i = 0; i < m->ndeps - 1; i++) {
      if (strcmp(m->deps[i].name, d->name) == 0) {
        rc = fail(&c, "duplicated dependency.");
        break;
      }
    }
  }

  free(text);
  if (rc) manifest_free(m);
  return rc;
}

void manifest_free(dmr_manifest* m) {
  int i;
  for (i = 0; i < m->ndeps; i++) {
    dmr_dep* d = &m->deps[i];
    free(d->name); free(d->version); free(d->registry); free(d->git);
    free(d->tag); free(d->branch); free(d->rev); free(d->path);
  }
  free(m->deps);
  free(m->name);
  free(m->version);
  memset(m, 0, sizeof(*m));
}
