#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dperm.h"
#include "dvm.h"
#include "ddefs.h"

void dperm_init(d_perms* p) {
  p->safe = 0;
  p->allowed = 0;
  p->native = NULL;
  p->native_count = 0;
  p->native_cap = 0;
}

void dperm_free(d_perms* p) {
  int i;
  for (i = 0; i < p->native_count; i++) free(p->native[i]);
  free(p->native);
  dperm_init(p);
}

void dperm_set_safe(d_perms* p, int safe) {
  p->safe = safe ? 1 : 0;
}

void dperm_allow(d_perms* p, d_perm what) {
  p->allowed |= (unsigned) what;
}

int dperm_allow_native_path(d_perms* p, const char* path) {
  if (p->native_count >= p->native_cap) {
    int cap = p->native_cap == 0 ? 4 : p->native_cap * 2;
    char** n = (char**) realloc(p->native, sizeof(char*) * (size_t) cap);
    if (n == NULL) return 0;
    p->native = n;
    p->native_cap = cap;
  }

  size_t len = strlen(path);
  char* copy = (char*) malloc(len + 1);
  if (copy == NULL) return 0;
  memcpy(copy, path, len + 1);

  p->native[p->native_count++] = copy;
  return 1;
}

int dperm_allows(const d_perms* p, d_perm what) {
  return !p->safe || (p->allowed & (unsigned) what) != 0;
}

typedef struct perm_option {
  const char* name;  /* after --allow- */
  d_perm what;
  const char* help;
} perm_option;

static const perm_option perm_options[] = {
  { "run",    D_PERM_RUN,    "run system commands (Os.cmd, Os.cmd_with_status, Os.system)" },
  { "net",    D_PERM_NET,    "use the network (TCPServer.new)" },
  { "env",    D_PERM_ENV,    "read environment variables (Os.get_env)" },
  { "fs",     D_PERM_FS,     "change the file system (Os.mkdir, Os.pmkdir)" },
  { "native", D_PERM_NATIVE, "load native extensions (Native.load); --allow-native=a.so,b.so allows only those" },
};

#define N_PERM_OPTIONS ((int) (sizeof(perm_options) / sizeof(perm_options[0])))

static const perm_option* find_perm(d_perm what) {
  int i;
  for (i = 0; i < N_PERM_OPTIONS; i++) {
    if (perm_options[i].what == what) return &perm_options[i];
  }
  return NULL;
}

static void set_err(char* err, size_t size, const char* fmt, const char* a) {
  if (err != NULL && size > 0) snprintf(err, size, fmt, a);
}

/* adds the paths of "a.so,b.so" */
static int add_native_list(d_perms* p, const char* list, char* err, size_t err_size) {
  const char* s = list;
  int added = 0;

  while (*s != '\0') {
    const char* e = strchr(s, ',');
    size_t n = e == NULL ? strlen(s) : (size_t) (e - s);

    if (n > 0) {
      char* item = (char*) malloc(n + 1);
      if (item == NULL) return -1;
      memcpy(item, s, n);
      item[n] = '\0';
      int ok = dperm_allow_native_path(p, item);
      free(item);
      if (!ok) return -1;
      added++;
    }

    if (e == NULL) break;
    s = e + 1;
  }

  if (!added) {
    set_err(err, err_size,
      "drax: option '%s' needs at least one path (example: --allow-native=./greet.so,./other.so)", "--allow-native=");
    return -1;
  }
  return 0;
}

int dperm_parse_option(d_perms* p, const char* arg, char* err, size_t err_size) {
  if (strcmp(arg, "--safe") == 0) {
    p->safe = 1;
    return 1;
  }

  if (strcmp(arg, "--allow-all") == 0) {
    p->allowed |= D_PERM_ALL;
    return 1;
  }

  if (strncmp(arg, "--allow-", 8) != 0) return 0;

  const char* name = arg + 8;
  const char* eq = strchr(name, '=');
  size_t nlen = eq == NULL ? strlen(name) : (size_t) (eq - name);
  int i;

  for (i = 0; i < N_PERM_OPTIONS; i++) {
    const perm_option* o = &perm_options[i];
    if (strlen(o->name) != nlen || strncmp(o->name, name, nlen) != 0) continue;

    if (eq == NULL) {
      p->allowed |= (unsigned) o->what;
      return 1;
    }

    if (o->what != D_PERM_NATIVE) {
      char opt[64];
      snprintf(opt, sizeof(opt), "--allow-%s", o->name);
      set_err(err, err_size, "drax: option '%s' does not take a value (use it as it is: it allows everything of that kind)", opt);
      return -1;
    }

    return add_native_list(p, eq + 1, err, err_size) == 0 ? 1 : -1;
  }

  {
    char list[256];
    size_t used = 0;
    list[0] = '\0';
    for (i = 0; i < N_PERM_OPTIONS; i++) {
      int n = snprintf(list + used, sizeof(list) - used, "%s--allow-%s", i == 0 ? "" : ", ", perm_options[i].name);
      if (n < 0 || (size_t) n >= sizeof(list) - used) break;
      used += (size_t) n;
    }

    if (err != NULL && err_size > 0) {
      snprintf(err, err_size, "drax: unknown option '%.60s'. The permissions are: %s, --allow-all (see drax --help)", arg, list);
    }
  }
  return -1;
}

const char* dperm_options_help(void) {
  return
  "  --safe                 start with no permissions: only what --allow-* grants works\n"
  "  --allow-run            allow running system commands (Os.cmd, Os.cmd_with_status, Os.system)\n"
  "  --allow-net            allow using the network (TCPServer.new)\n"
  "  --allow-env            allow reading environment variables (Os.get_env)\n"
  "  --allow-fs             allow changing the file system (Os.mkdir, Os.pmkdir)\n"
  "  --allow-native         allow loading any native extension (Native.load)\n"
  "  --allow-native=a,b     allow only the extensions at those paths (a.so,b.so)\n"
  "  --allow-all            allow everything (--safe --allow-all is the same as not using --safe)\n";
}

void dperm_denied_text(d_perm what, const char* fn, const char* detail, char* buf, size_t size) {
  const perm_option* o = find_perm(what);
  const char* name = o != NULL ? o->name : "?";

  const char* does = "do that";
  switch (what) {
    case D_PERM_RUN:    does = "run system commands"; break;
    case D_PERM_NET:    does = "use the network"; break;
    case D_PERM_ENV:    does = "read environment variables"; break;
    case D_PERM_FS:     does = "change the file system"; break;
    case D_PERM_NATIVE: does = "load native extensions"; break;
  }

  if (what == D_PERM_NATIVE && detail != NULL) {
    snprintf(buf, size,
      "%s: permission denied. This program runs in safe mode and is not allowed to load the native extension '%s'. "
      "To allow it, start drax with --allow-native=%s (or --allow-native to allow every extension).",
      fn, detail, detail);
    return;
  }

  snprintf(buf, size,
    "%s: permission denied. This program runs in safe mode and is not allowed to %s. "
    "To allow it, start drax with --allow-%s (example: drax --safe --allow-%s script.dx).",
    fn, does, name, name);
}

drax_value dperm_deny(d_vm* vm, int* stat, d_perm what, const char* fn, const char* detail) {
  char msg[8192]; /* a path can be long: the flag that fixes it is at the end */
  dperm_denied_text(what, fn, detail, msg, sizeof(msg));
  DX_ERROR_FN(stat);
  return DS_VAL(new_derror(vm, msg));
}
