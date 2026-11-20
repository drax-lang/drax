/* drax Lang
 *
 * Permissions of a program ("safe mode").
 *
 * By default a program may do everything (like any script). With --safe
 * (or dperm_set_safe for a program that embeds the VM) the table starts
 * empty: only what is allowed with --allow-* works, and a function that
 * needs another permission fails with a message that says which flag to use.
 *
 * The table protects against the Drax code (a script or a package) that
 * does what it should not. It does not isolate native code: an extension
 * that was allowed runs with all the rights of the process.
 */

#ifndef __DPERM
#define __DPERM

#include <stddef.h>
#include "dtypes.h"

typedef enum d_perm {
  D_PERM_RUN    = 1,   /* Os.cmd, Os.cmd_with_status, Os.system      --allow-run    */
  D_PERM_NET    = 2,   /* TCPServer.new                              --allow-net    */
  D_PERM_ENV    = 4,   /* Os.get_env                                 --allow-env    */
  D_PERM_FS     = 8,   /* Os.mkdir, Os.pmkdir                        --allow-fs     */
  D_PERM_NATIVE = 16   /* Native.load                                --allow-native */
} d_perm;

#define D_PERM_ALL (D_PERM_RUN | D_PERM_NET | D_PERM_ENV | D_PERM_FS | D_PERM_NATIVE)

typedef struct d_perms {
  int safe;            /* 0 (default): everything is allowed */
  unsigned allowed;    /* in safe mode: the permissions that were granted */
  char** native;       /* in safe mode: the extensions that may be loaded, when D_PERM_NATIVE is not granted for all */
  int native_count;
  int native_cap;
} d_perms;

void dperm_init(d_perms* p);

void dperm_free(d_perms* p);

/* embedding: the same as --safe and --allow-* */
void dperm_set_safe(d_perms* p, int safe);

void dperm_allow(d_perms* p, d_perm what);

/* allows one extension (a path, compared after it is resolved). Returns 0 when out of memory */
int dperm_allow_native_path(d_perms* p, const char* path);

/* 1 when the program may do 'what' */
int dperm_allows(const d_perms* p, d_perm what);

/**
 * A command line option. Returns 1 when 'arg' is a permission option (and
 * it was applied), 0 when it is not, -1 when it is one but it is wrong
 * ('err' receives the message).
 */
int dperm_parse_option(d_perms* p, const char* arg, char* err, size_t err_size);

/* what the options are, for --help */
const char* dperm_options_help(void);

/**
 * The message of a denied call: the function, what it needs and the flag
 * that allows it. 'detail' is optional (the extension that was refused).
 */
void dperm_denied_text(d_perm what, const char* fn, const char* detail, char* buf, size_t size);

#endif
