/* dmr - drax manager & resolver
 *
 * install / update: reads package.toml of the current directory, downloads
 * the dependencies into ./lib (the default library directory of Drax, see
 * docs/imports.md) and writes package.lock.
 */
#ifndef __DMR_INSTALL
#define __DMR_INSTALL

#define DMR_MANIFEST "package.toml"
#define DMR_LOCKFILE "package.lock"
#define DMR_LIBDIR   "lib"

/* update = 0: use the revisions of package.lock when they exist.
 * update = 1: ignore the lock and resolve tags/branches again.
 * returns the exit code of the process. */
int cmd_install(int update);

#endif
