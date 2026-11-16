/* dmr - drax manager & resolver
 *
 * add: adds a dependency to package.toml.
 *
 *   dmr add user/repo[@tag]        GitHub shorthand
 *   dmr add <git url>              https://..., git@host:..., file://...
 *   dmr add ./dir | ../dir | /abs  local path
 *
 * options: --name n  --tag t  --branch b  --rev r
 */
#ifndef __DMR_ADD
#define __DMR_ADD

/* then_install: run `install` after the manifest was changed.
 * returns the exit code of the process. */
int cmd_add(int argc, char** argv, int then_install);

#endif
