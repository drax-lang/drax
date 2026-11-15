/* dmr - drax manager & resolver */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "dfs.h"

void* xrealloc(void* p, size_t n) {
  void* r = realloc(p, n ? n : 1);
  if (r == NULL) {
    fprintf(stderr, "dmr: out of memory\n");
    exit(1);
  }
  return r;
}

char* xstrdup(const char* s) {
  size_t n = strlen(s);
  char* r = (char*) xrealloc(NULL, n + 1);
  memcpy(r, s, n + 1);
  return r;
}

char* xfmt(const char* fmt, ...) {
  va_list ap, ap2;
  va_start(ap, fmt);
  va_copy(ap2, ap);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  char* r = (char*) xrealloc(NULL, (size_t) n + 1);
  vsnprintf(r, (size_t) n + 1, fmt, ap2);
  va_end(ap2);
  return r;
}

int fs_exists(const char* p) {
  struct stat st;
  return lstat(p, &st) == 0;
}

int fs_is_dir(const char* p) {
  struct stat st;
  return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

int fs_is_symlink(const char* p) {
  struct stat st;
  return lstat(p, &st) == 0 && S_ISLNK(st.st_mode);
}

char* fs_read(const char* p, size_t* len) {
  FILE* f = fopen(p, "rb");
  if (!f) return NULL;

  size_t cap = 4096, n = 0;
  char* buf = (char*) xrealloc(NULL, cap);
  size_t r;
  while ((r = fread(buf + n, 1, cap - n - 1, f)) > 0) {
    n += r;
    if (n + 1 >= cap) {
      cap *= 2;
      buf = (char*) xrealloc(buf, cap);
    }
  }
  fclose(f);
  buf[n] = 0;
  if (len) *len = n;
  return buf;
}

int fs_write(const char* p, const char* data, size_t len) {
  FILE* f = fopen(p, "wb");
  if (!f) return 1;
  size_t w = fwrite(data, 1, len, f);
  int c = fclose(f);
  return (w == len && c == 0) ? 0 : 1;
}

int fs_mkdir_p(const char* p) {
  char* tmp = xstrdup(p);
  size_t n = strlen(tmp);
  size_t i;

  if (n > 1 && tmp[n - 1] == '/') tmp[--n] = 0;

  for (i = 1; i < n; i++) {
    if (tmp[i] != '/') continue;
    tmp[i] = 0;
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) { free(tmp); return 1; }
    tmp[i] = '/';
  }
  int r = (mkdir(tmp, 0755) != 0 && errno != EEXIST) ? 1 : 0;
  free(tmp);
  return r;
}

char* fs_join(const char* a, const char* b) {
  if (b[0] == '/') return xstrdup(b);
  size_t n = strlen(a);
  if (n > 0 && a[n - 1] == '/') return xfmt("%s%s", a, b);
  return xfmt("%s/%s", a, b);
}

char* fs_realpath(const char* p) {
  char buf[PATH_MAX];
  if (realpath(p, buf) == NULL) return NULL;
  return xstrdup(buf);
}

/* "from" and "to" are absolute and normalized (realpath) */
static int split_segments(const char* p, char*** out) {
  char* copy = xstrdup(p);
  char** segs = NULL;
  int n = 0;
  char* save = NULL;
  char* t;
  for (t = strtok_r(copy, "/", &save); t != NULL; t = strtok_r(NULL, "/", &save)) {
    segs = (char**) xrealloc(segs, sizeof(char*) * (size_t) (n + 1));
    segs[n++] = xstrdup(t);
  }
  free(copy);
  *out = segs;
  return n;
}

char* fs_relpath(const char* from, const char* to) {
  char** fs_;
  char** ts;
  int nf = split_segments(from, &fs_);
  int nt = split_segments(to, &ts);
  int c = 0, i;

  while (c < nf && c < nt && strcmp(fs_[c], ts[c]) == 0) c++;

  size_t cap = 3 * (size_t) (nf - c) + strlen(to) + 4;
  char* r = (char*) xrealloc(NULL, cap);
  r[0] = 0;
  for (i = c; i < nf; i++) strcat(r, "../");
  for (i = c; i < nt; i++) {
    strcat(r, ts[i]);
    if (i + 1 < nt) strcat(r, "/");
  }
  if (r[0] == 0) strcpy(r, ".");

  for (i = 0; i < nf; i++) free(fs_[i]);
  for (i = 0; i < nt; i++) free(ts[i]);
  free(fs_);
  free(ts);
  return r;
}

static int rm_cb(const char* path, const struct stat* st, int flag, struct FTW* ftw) {
  (void) st; (void) flag; (void) ftw;
  return remove(path);
}

int fs_rm_tree(const char* p) {
  struct stat st;
  if (lstat(p, &st) != 0) return 0;
  /* a symlink is only unlinked: its target is never touched */
  if (S_ISLNK(st.st_mode) || !S_ISDIR(st.st_mode)) return unlink(p) == 0 ? 0 : 1;
  return nftw(p, rm_cb, 32, FTW_DEPTH | FTW_PHYS) == 0 ? 0 : 1;
}

int proc_run(char* const argv[], char** out) {
  int fds[2] = { -1, -1 };

  if (out != NULL) {
    *out = NULL;
    if (pipe(fds) != 0) return -1;
  }

  fflush(stdout);
  fflush(stderr);
  pid_t pid = fork();
  if (pid < 0) return -1;

  if (pid == 0) {
    if (out != NULL) {
      dup2(fds[1], STDOUT_FILENO);
      close(fds[0]);
      close(fds[1]);
    }
    execvp(argv[0], argv);
    _exit(127);
  }

  if (out != NULL) {
    close(fds[1]);
    size_t cap = 256, n = 0;
    char* buf = (char*) xrealloc(NULL, cap);
    ssize_t r;
    while ((r = read(fds[0], buf + n, cap - n - 1)) > 0) {
      n += (size_t) r;
      if (n + 1 >= cap) {
        cap *= 2;
        buf = (char*) xrealloc(buf, cap);
      }
    }
    close(fds[0]);
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) n--;
    buf[n] = 0;
    *out = buf;
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) return -1;
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  return -1;
}
