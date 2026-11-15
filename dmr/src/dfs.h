/* dmr - drax manager & resolver
 *
 * file system and process helpers. dmr is standalone: it does not link any
 * source of the Drax interpreter.
 */
#ifndef __DMR_FS
#define __DMR_FS

#include <stddef.h>

/* memory: all of them abort on out of memory */
char* xstrdup(const char* s);
void* xrealloc(void* p, size_t n);
char* xfmt(const char* fmt, ...);

/* paths */
int fs_exists(const char* p);          /* lstat: a broken symlink exists */
int fs_is_dir(const char* p);          /* follows symlinks */
int fs_is_symlink(const char* p);
char* fs_read(const char* p, size_t* len);   /* NULL when it can not be read */
int fs_write(const char* p, const char* data, size_t len);
int fs_mkdir_p(const char* p);
char* fs_join(const char* a, const char* b);
char* fs_realpath(const char* p);      /* NULL when it does not exist */
char* fs_relpath(const char* from_dir, const char* to);  /* both absolute */

/* removes a file, a symlink (never follows it) or a whole tree */
int fs_rm_tree(const char* p);

/* process: argv without a shell (the values come from a package file,
 * so they must never be interpreted by a shell).
 * returns the exit code, -1 when it can not be started.
 * out != NULL captures stdout (NUL terminated, trailing newline cut). */
int proc_run(char* const argv[], char** out);

#endif
