/* dmr - drax manager & resolver */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#include "dgraph.h"
#include "dlang.h"
#include "dfs.h"

#define GRAPH_VERSION 1

/* ------------------------------------------------------------------ */
/* scanner                                                             */
/* ------------------------------------------------------------------ */

typedef enum { T_ID, T_STR, T_NUM, T_SYM, T_EOF } tk_kind;

typedef struct tok {
  tk_kind kind;
  char* text;
  int line;
} tok;

typedef struct toks {
  tok* v;
  int n, cap;
} toks;

static void push_tok(toks* t, tk_kind k, const char* s, size_t len, int line) {
  if (t->n >= t->cap) {
    t->cap = t->cap == 0 ? 256 : t->cap * 2;
    t->v = (tok*) xrealloc(t->v, sizeof(tok) * (size_t) t->cap);
  }
  char* x = (char*) xrealloc(NULL, len + 1);
  memcpy(x, s, len);
  x[len] = 0;
  t->v[t->n].kind = k;
  t->v[t->n].text = x;
  t->v[t->n].line = line;
  t->n++;
}

static int id_start(char c) { return isalpha((unsigned char) c) || c == '_' || c == '?'; }
static int id_part(char c) { return id_start(c) || isdigit((unsigned char) c); }

static void scan(const char* s, toks* t) {
  int line = 1;
  const char* p = s;

  while (*p) {
    char c = *p;

    if (c == '\n') { line++; p++; continue; }
    if (c == ' ' || c == '\t' || c == '\r') { p++; continue; }
    if (c == '#') { while (*p && *p != '\n') p++; continue; }

    if (c == '"' && p[1] == '"' && p[2] == '"') {
      /* multiline string: up to the next """ */
      int start_line = line;
      const char* b = p + 3;
      p += 3;
      while (*p && !(p[0] == '"' && p[1] == '"' && p[2] == '"')) { if (*p == '\n') line++; p++; }
      push_tok(t, T_STR, b, (size_t) (p - b), start_line);
      if (*p) p += 3;
      continue;
    }

    if (c == '"' || c == '\'') {
      int start_line = line;
      const char* b = ++p;
      while (*p && *p != c) {
        if (*p == '\\' && p[1]) p++;
        if (*p == '\n') line++;
        p++;
      }
      push_tok(t, T_STR, b, (size_t) (p - b), start_line);
      if (*p) p++;
      continue;
    }

    if (isdigit((unsigned char) c)) {
      const char* b = p;
      while (isalnum((unsigned char) *p) || *p == '_') p++;
      if (*p == '.' && isdigit((unsigned char) p[1])) {
        p++;
        while (isdigit((unsigned char) *p)) p++;
      }
      push_tok(t, T_NUM, b, (size_t) (p - b), line);
      continue;
    }

    if (id_start(c)) {
      const char* b = p;
      while (id_part(*p)) p++;
      push_tok(t, T_ID, b, (size_t) (p - b), line);
      continue;
    }

    static const char* two[] = { "|>", "++", "==", "!=", "<=", ">=", "::", "<<", ">>", 0 };
    int k, matched = 0;
    for (k = 0; two[k]; k++) {
      if (p[0] == two[k][0] && p[1] == two[k][1]) {
        push_tok(t, T_SYM, p, 2, line);
        p += 2;
        matched = 1;
        break;
      }
    }
    if (matched) continue;

    push_tok(t, T_SYM, p, 1, line);
    p++;
  }
  push_tok(t, T_EOF, "", 0, line);
}

static void free_toks(toks* t) {
  int i;
  for (i = 0; i < t->n; i++) free(t->v[i].text);
  free(t->v);
  memset(t, 0, sizeof(*t));
}

/* ------------------------------------------------------------------ */
/* model                                                               */
/* ------------------------------------------------------------------ */

#define GROW(arr, count, cap) \
  do { \
    if ((count) >= (cap)) { \
      (cap) = (cap) == 0 ? 16 : (cap) * 2; \
      (arr) = xrealloc((arr), sizeof(*(arr)) * (size_t) (cap)); \
    } \
  } while (0)

typedef struct g_fn {
  int id, file, parent;
  char* name;                 /* NULL: anonymous */
  int arity;
  char* params;               /* "a,b" */
  int line0, line1;
} g_fn;

typedef struct g_call {
  int file, from;             /* from: function id, 0 = file scope */
  char* callee;               /* "name" or "obj.key" */
  int arity, line;
} g_call;

typedef struct g_def { int file; char* name; int line; } g_def;
typedef struct g_imp { int file; char* spec; int line; } g_imp;
typedef struct g_alias { int file; char* name; char* spec; } g_alias;
typedef struct g_exp { int file; char* keys; } g_exp;
typedef struct g_file { char* path; char* real; int ok; } g_file;

static g_file* files;   static int nfiles, cfiles;
static g_fn* fns;       static int nfns, cfns;
static g_call* calls;   static int ncalls, ccalls;
static g_def* defs;     static int ndefs, cdefs;
static g_imp* imps;     static int nimps, cimps;
static g_alias* als;    static int nals, cals;
static g_exp* exps;     static int nexps, cexps;

static void append_word(char** buf, const char* w, char sep) {
  size_t a = *buf ? strlen(*buf) : 0, b = strlen(w);
  *buf = (char*) xrealloc(*buf, a + b + 2);
  if (a > 0) (*buf)[a++] = sep;
  memcpy(*buf + a, w, b + 1);
}

/* ------------------------------------------------------------------ */
/* structure                                                           */
/* ------------------------------------------------------------------ */

typedef enum { B_FUN, B_IF, B_DO } blk_kind;
typedef struct blk { blk_kind kind; int fn; int seen_do; } blk;

static int is_sym(const toks* t, int i, const char* s) {
  return i >= 0 && i < t->n && t->v[i].kind == T_SYM && strcmp(t->v[i].text, s) == 0;
}

static int is_id(const toks* t, int i) {
  return i >= 0 && i < t->n && t->v[i].kind == T_ID;
}

/* arguments of the call that opens at tokens[open] == '(' */
static int count_args(const toks* t, int open) {
  int depth = 0, commas = 0, any = 0, k;
  for (k = open; k < t->n && t->v[k].kind != T_EOF; k++) {
    if (t->v[k].kind == T_SYM) {
      const char* s = t->v[k].text;
      if (s[0] == '(' || s[0] == '[' || s[0] == '{') { depth++; if (depth > 1) any = 1; continue; }
      if (s[0] == ')' || s[0] == ']' || s[0] == '}') { depth--; if (depth == 0) break; continue; }
      if (depth == 1 && s[0] == ',' && s[1] == 0) { commas++; continue; }
    }
    if (depth >= 1 && k != open) any = 1;
  }
  return any ? commas + 1 : 0;
}

static void add_call(int file, int from, const char* callee, int arity, int line) {
  GROW(calls, ncalls, ccalls);
  g_call* c = &calls[ncalls++];
  c->file = file;
  c->from = from;
  c->callee = xstrdup(callee);
  c->arity = arity;
  c->line = line;
}

/* returns 1 when the structure is balanced */
static int analyze(int file, const toks* t) {
  blk stack[512];
  int depth = 0, unbalanced = 0, i;

  for (i = 0; i < t->n && t->v[i].kind != T_EOF; i++) {
    const tok* tk = &t->v[i];

    if (tk->kind == T_SYM) {
      /* &(...) : call of the function being defined */
      if (tk->text[0] == '&' && tk->text[1] == 0 && is_sym(t, i + 1, "(")) {
        int k, fn = 0;
        for (k = depth - 1; k >= 0; k--) {
          if (stack[k].kind == B_FUN && fns[stack[k].fn - 1].name) { fn = stack[k].fn; break; }
        }
        if (fn) add_call(file, fn, fns[fn - 1].name, count_args(t, i + 1) + (is_sym(t, i - 1, "|>") ? 1 : 0), tk->line);
      }
      continue;
    }
    if (tk->kind != T_ID) continue;

    const char* w = tk->text;

    if (strcmp(w, "fun") == 0) {
      int j = i + 1;
      const char* name = NULL;
      if (is_id(t, j) && is_sym(t, j + 1, "(")) { name = t->v[j].text; j++; }

      char* params = NULL;
      int arity = 0;
      if (is_sym(t, j, "(")) {
        int k = j + 1;
        while (k < t->n && t->v[k].kind != T_EOF && !is_sym(t, k, ")")) {
          if (t->v[k].kind == T_ID) { append_word(&params, t->v[k].text, ','); arity++; }
          k++;
        }
        i = k;
      }

      int parent = 0, b;
      for (b = depth - 1; b >= 0; b--) if (stack[b].kind == B_FUN) { parent = stack[b].fn; break; }

      GROW(fns, nfns, cfns);
      g_fn* f = &fns[nfns++];
      f->id = nfns;
      f->file = file;
      f->parent = parent;
      f->name = name ? xstrdup(name) : NULL;
      f->arity = arity;
      f->params = params;
      f->line0 = tk->line;
      f->line1 = tk->line;

      if (depth < 512) { stack[depth].kind = B_FUN; stack[depth].fn = f->id; stack[depth].seen_do = 0; depth++; }
      continue;
    }

    if (strcmp(w, "if") == 0) {
      if (depth < 512) { stack[depth].kind = B_IF; stack[depth].fn = 0; stack[depth].seen_do = 0; depth++; }
      continue;
    }

    if (strcmp(w, "do") == 0) {
      if (depth > 0 && !stack[depth - 1].seen_do && stack[depth - 1].kind != B_DO) {
        stack[depth - 1].seen_do = 1;
      } else if (depth < 512) {
        stack[depth].kind = B_DO; stack[depth].fn = 0; stack[depth].seen_do = 1; depth++;
      }
      continue;
    }

    if (strcmp(w, "end") == 0) {
      if (depth == 0) { unbalanced = 1; continue; }
      depth--;
      if (stack[depth].kind == B_FUN) fns[stack[depth].fn - 1].line1 = tk->line;
      continue;
    }

    int cur = 0, b;
    for (b = depth - 1; b >= 0; b--) if (stack[b].kind == B_FUN) { cur = stack[b].fn; break; }

    if (strcmp(w, "import") == 0 && is_sym(t, i + 1, "(") && t->v[i + 2].kind == T_STR) {
      GROW(imps, nimps, cimps);
      imps[nimps].file = file;
      imps[nimps].spec = xstrdup(t->v[i + 2].text);
      imps[nimps].line = tk->line;
      nimps++;

      if (cur == 0 && is_sym(t, i - 1, "=") && is_id(t, i - 2) && !is_sym(t, i - 3, ".")) {
        GROW(als, nals, cals);
        als[nals].file = file;
        als[nals].name = xstrdup(t->v[i - 2].text);
        als[nals].spec = xstrdup(t->v[i + 2].text);
        nals++;
      }
      continue;
    }

    if (strcmp(w, "export") == 0 && is_sym(t, i + 1, "(") && is_sym(t, i + 2, "{")) {
      char* keys = NULL;
      int k, d = 0;
      for (k = i + 2; k < t->n && t->v[k].kind != T_EOF; k++) {
        if (t->v[k].kind == T_SYM) {
          char ch = t->v[k].text[0];
          if (t->v[k].text[1] == 0 && (ch == '(' || ch == '[' || ch == '{')) d++;
          else if (t->v[k].text[1] == 0 && (ch == ')' || ch == ']' || ch == '}')) { d--; if (d == 0) break; }
        }
        if (d == 1 && (t->v[k].kind == T_ID || t->v[k].kind == T_STR) &&
            (is_sym(t, k - 1, "{") || is_sym(t, k - 1, ",")) && is_sym(t, k + 1, ":")) {
          append_word(&keys, t->v[k].text, ' ');
        }
      }
      GROW(exps, nexps, cexps);
      exps[nexps].file = file;
      exps[nexps].keys = keys ? keys : xstrdup("");
      nexps++;
      continue;
    }

    if (dlang_is_keyword(w)) continue;

    /* name(...) or obj.name(...) */
    if (is_sym(t, i + 1, "(")) {
      int start = i;
      char callee[512];
      if (is_sym(t, i - 1, ".")) {
        const char* obj = "?";
        if (is_id(t, i - 2) && !is_sym(t, i - 3, ".")) obj = t->v[i - 2].text;
        snprintf(callee, sizeof(callee), "%s.%s", obj, w);
        while (is_sym(t, start - 1, ".") && is_id(t, start - 2)) start -= 2;
      } else {
        snprintf(callee, sizeof(callee), "%s", w);
      }
      add_call(file, cur, callee, count_args(t, i + 1) + (is_sym(t, start - 1, "|>") ? 1 : 0), tk->line);
      continue;
    }

    /* name = ...   (global variable, not '==') */
    if (cur == 0 && is_sym(t, i + 1, "=") && !is_sym(t, i - 1, ".")) {
      GROW(defs, ndefs, cdefs);
      defs[ndefs].file = file;
      defs[ndefs].name = xstrdup(w);
      defs[ndefs].line = tk->line;
      ndefs++;
    }
  }

  return !(unbalanced || depth != 0);
}

/* ------------------------------------------------------------------ */
/* resolution                                                          */
/* ------------------------------------------------------------------ */

static int find_file_by_real(const char* real) {
  int i;
  for (i = 0; i < nfiles; i++) {
    if (files[i].real && strcmp(files[i].real, real) == 0) return i;
  }
  return -1;
}

static int is_reg(const char* p) {
  struct stat st;
  return stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

/* a file, the same with ".dx", or a package directory (its main.dx):
 * the same order of the interpreter */
static int try_real(const char* path, char* out) {
  char tmp[2 * PATH_MAX + 16];
  size_t n = strlen(path);
  int dir = 0;

  if (realpath(path, out) != NULL) {
    if (is_reg(out)) return 1;
    dir = fs_is_dir(out);
  }
  if (n < 3 || strcmp(path + n - 3, ".dx") != 0) {
    snprintf(tmp, sizeof(tmp), "%s.dx", path);
    if (realpath(tmp, out) != NULL && is_reg(out)) return 1;
  }
  if (dir) {
    snprintf(tmp, sizeof(tmp), "%s/main.dx", path);
    if (realpath(tmp, out) != NULL && is_reg(out)) return 1;
  }
  return 0;
}

/* same rules as the import of the interpreter, see docs/imports.md */
static int resolve_import(int from_file, const char* spec) {
  char dir[PATH_MAX];
  char cand[2 * PATH_MAX + 16];
  char out[PATH_MAX];
  char cwd[PATH_MAX];

  snprintf(dir, sizeof(dir), "%s", files[from_file].real);
  char* slash = strrchr(dir, '/');
  if (slash) *slash = 0;

  int explicit_path = spec[0] == '/' ||
    (spec[0] == '.' && (spec[1] == '/' || (spec[1] == '.' && spec[2] == '/')));

  if (explicit_path) {
    if (spec[0] == '/') snprintf(cand, sizeof(cand), "%s", spec);
    else snprintf(cand, sizeof(cand), "%s/%s", dir, spec);
    return try_real(cand, out) ? find_file_by_real(out) : -1;
  }

  /* default lib/ directory: next to the importer, then on the current dir */
  snprintf(cand, sizeof(cand), "%s/lib/%s", dir, spec);
  if (try_real(cand, out)) return find_file_by_real(out);
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    snprintf(cand, sizeof(cand), "%s/lib/%s", cwd, spec);
    if (try_real(cand, out)) return find_file_by_real(out);
  }
  return -1;
}

static int find_alias(int file, const char* name) {
  int i;
  for (i = nals - 1; i >= 0; i--) {
    if (als[i].file == file && strcmp(als[i].name, name) == 0) return i;
  }
  return -1;
}

static int find_fn(int file, const char* name, int arity) {
  int i;
  for (i = 0; i < nfns; i++) {
    if (fns[i].file == file && fns[i].name && fns[i].arity == arity && strcmp(fns[i].name, name) == 0) {
      return fns[i].id;
    }
  }
  return 0;
}

/* f<id> | b:name/n | m:Mod.fn/n | F<id>.key/n | ?name/n */
static void resolve_call(const g_call* c, char* out, size_t sz) {
  char* dot = strchr(c->callee, '.');

  if (dot == NULL) {
    int id = find_fn(c->file, c->callee, c->arity);
    if (id) snprintf(out, sz, "f%d", id);
    else if (dlang_has_builtin(c->callee, c->arity)) snprintf(out, sz, "b:%s/%d", c->callee, c->arity);
    else snprintf(out, sz, "?%s/%d", c->callee, c->arity);
    return;
  }

  char obj[256];
  size_t ol = (size_t) (dot - c->callee);
  if (ol >= sizeof(obj)) ol = sizeof(obj) - 1;
  memcpy(obj, c->callee, ol);
  obj[ol] = 0;
  const char* key = dot + 1;

  if (dlang_has_module_fn(obj, key, c->arity)) {
    snprintf(out, sz, "m:%s.%s/%d", obj, key, c->arity);
    return;
  }

  int a = find_alias(c->file, obj);
  if (a >= 0) {
    int target = resolve_import(c->file, als[a].spec);
    if (target >= 0) { snprintf(out, sz, "F%d.%s/%d", target + 1, key, c->arity); return; }
  }
  snprintf(out, sz, "?%s/%d", c->callee, c->arity);
}

static void print_target(FILE* o, int file, const char* spec) {
  int t = resolve_import(file, spec);
  if (t >= 0) fprintf(o, "F%d", t + 1);
  else fprintf(o, "\"%s\"", spec);
}

/* ------------------------------------------------------------------ */
/* output                                                              */
/* ------------------------------------------------------------------ */

static void print_fn_list(FILE* o, const char* const* list) {
  int i = 0;
  while (list[i]) {
    const char* s = list[i];
    const char* slash = strchr(s, '/');
    size_t nlen = (size_t) (slash - s);
    fprintf(o, " %s", s);
    int j = i + 1;
    while (list[j] && strncmp(list[j], s, nlen) == 0 && list[j][nlen] == '/') {
      fprintf(o, ",%s", list[j] + nlen + 1);
      j++;
    }
    i = j;
  }
  fprintf(o, "\n");
}

static void emit_lang(FILE* o) {
  int i;
  fprintf(o, "#dlang %d\nkw", GRAPH_VERSION);
  for (i = 0; DLANG_KEYWORDS[i]; i++) fprintf(o, " %s", DLANG_KEYWORDS[i]);
  fprintf(o, "\n");

  for (i = 0; DLANG_OPERATORS[i].level; i++) {
    fprintf(o, "op.%s %s\n", DLANG_OPERATORS[i].level, DLANG_OPERATORS[i].ops);
  }
  fprintf(o, "fn");
  print_fn_list(o, DLANG_BUILTINS);
  for (i = 0; DLANG_MODULES[i].name; i++) {
    fprintf(o, "mod %s", DLANG_MODULES[i].name);
    print_fn_list(o, DLANG_MODULES[i].fns);
  }
}

static void emit_cpg(FILE* o) {
  int i, k;
  fprintf(o, "#dcpg %d\n", GRAPH_VERSION);

  for (i = 0; i < nfiles; i++) {
    fprintf(o, "F%d %s%s\n", i + 1, files[i].path, files[i].ok ? "" : " !parse-error");

    for (k = 0; k < nfns; k++) {
      g_fn* f = &fns[k];
      if (f->file != i) continue;
      fprintf(o, "f%d F%d %s/%d %s %d-%d", f->id, i + 1, f->name ? f->name : "_",
              f->arity, f->params ? f->params : "-", f->line0, f->line1);
      if (f->parent) fprintf(o, " ^f%d", f->parent);
      fprintf(o, "\n");
    }
    for (k = 0; k < ndefs; k++) {
      if (defs[k].file == i) fprintf(o, "g F%d %s %d\n", i + 1, defs[k].name, defs[k].line);
    }
    for (k = 0; k < nimps; k++) {
      if (imps[k].file != i) continue;
      fprintf(o, "i F%d ", i + 1);
      print_target(o, i, imps[k].spec);
      fprintf(o, " %d\n", imps[k].line);
    }
    for (k = 0; k < nals; k++) {
      if (als[k].file != i) continue;
      fprintf(o, "a F%d %s ", i + 1, als[k].name);
      print_target(o, i, als[k].spec);
      fprintf(o, "\n");
    }
    for (k = 0; k < nexps; k++) {
      if (exps[k].file == i) fprintf(o, "e F%d %s\n", i + 1, exps[k].keys);
    }
    for (k = 0; k < ncalls; k++) {
      g_call* c = &calls[k];
      if (c->file != i) continue;
      char target[600];
      resolve_call(c, target, sizeof(target));
      if (c->from) fprintf(o, "c f%d %s %d\n", c->from, target, c->line);
      else fprintf(o, "c F%d %s %d\n", i + 1, target, c->line);
    }
  }
}

/* ------------------------------------------------------------------ */
/* driver                                                              */
/* ------------------------------------------------------------------ */

static char** inputs;
static int ninputs, cinputs;

static int cmp_str(const void* a, const void* b) {
  return strcmp(*(const char* const*) a, *(const char* const*) b);
}

static void add_input(const char* path) {
  GROW(inputs, ninputs, cinputs);
  inputs[ninputs++] = xstrdup(path);
}

/* directories already walked (by real path): a symlink can make a cycle */
static char** seen_dirs;
static int nseen, cseen;

static int seen_dir(const char* real) {
  int i;
  for (i = 0; i < nseen; i++) if (strcmp(seen_dirs[i], real) == 0) return 1;
  GROW(seen_dirs, nseen, cseen);
  seen_dirs[nseen++] = xstrdup(real);
  return 0;
}

static void walk(const char* path) {
  struct stat st;
  if (stat(path, &st) != 0) {
    fprintf(stderr, "dmr graph: '%s' not found.\n", path);
    return;
  }
  if (S_ISREG(st.st_mode)) { add_input(path); return; }
  if (!S_ISDIR(st.st_mode)) return;

  char* real = fs_realpath(path);
  if (real == NULL || seen_dir(real)) { free(real); return; }
  free(real);

  DIR* d = opendir(path);
  if (!d) return;

  char** names = NULL;
  int n = 0, cap = 0, i;
  struct dirent* e;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.') continue; /* .git, .drax-cpg, ... */
    GROW(names, n, cap);
    names[n++] = xstrdup(e->d_name);
  }
  closedir(d);
  qsort(names, (size_t) n, sizeof(names[0]), cmp_str);

  for (i = 0; i < n; i++) {
    char* sub = fs_join(path, names[i]);
    size_t l = strlen(names[i]);
    /* symlinks to directories (path dependencies) are walked too */
    if (stat(sub, &st) == 0) {
      if (S_ISDIR(st.st_mode)) walk(sub);
      else if (l > 3 && strcmp(names[i] + l - 3, ".dx") == 0) add_input(sub);
    }
    free(sub);
    free(names[i]);
  }
  free(names);
}

static int cmp_files(const void* a, const void* b) {
  return strcmp(((const g_file*) a)->path, ((const g_file*) b)->path);
}

int cmd_graph(int npaths, char** paths) {
  int i;
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) cwd[0] = 0;

  if (npaths == 0) walk(".");
  for (i = 0; i < npaths; i++) walk(paths[i]);

  if (ninputs == 0) {
    fprintf(stderr, "dmr graph: no .dx file found.\n");
    return 1;
  }

  /* unique files, sorted by the displayed path: the output is deterministic */
  for (i = 0; i < ninputs; i++) {
    char* real = fs_realpath(inputs[i]);
    if (real == NULL) continue;
    if (find_file_by_real(real) >= 0) { free(real); continue; }

    GROW(files, nfiles, cfiles);
    char* shown = fs_relpath(cwd, real);
    files[nfiles].real = real;
    /* outside of the current directory: the absolute path is not portable */
    files[nfiles].path = strncmp(shown, "../", 3) == 0 ? xstrdup(real) : shown;
    if (files[nfiles].path != shown) free(shown);
    files[nfiles].ok = 0;
    nfiles++;
  }
  qsort(files, (size_t) nfiles, sizeof(g_file), cmp_files);

  for (i = 0; i < nfiles; i++) {
    char* text = fs_read(files[i].real, NULL);
    if (text == NULL) continue;
    toks t;
    memset(&t, 0, sizeof(t));
    scan(text, &t);
    files[i].ok = analyze(i, &t);
    free_toks(&t);
    free(text);
  }

  if (!fs_is_dir(DMR_GRAPH_DIR) && fs_mkdir_p(DMR_GRAPH_DIR) != 0) {
    fprintf(stderr, "dmr graph: cannot create '%s'.\n", DMR_GRAPH_DIR);
    return 1;
  }

  FILE* lang = fopen(DMR_GRAPH_DIR "/lang.dgraph", "w");
  FILE* cpg = fopen(DMR_GRAPH_DIR "/cpg.dgraph", "w");
  if (!lang || !cpg) {
    fprintf(stderr, "dmr graph: cannot write on '%s'.\n", DMR_GRAPH_DIR);
    return 1;
  }
  emit_lang(lang);
  emit_cpg(cpg);
  fclose(lang);
  fclose(cpg);

  fprintf(stderr, "dmr graph: %d file(s), %d function(s) -> %s/\n", nfiles, nfns, DMR_GRAPH_DIR);
  return 0;
}
