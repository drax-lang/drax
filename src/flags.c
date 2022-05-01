/* Beorn Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef _BEORN_FLAGS
#define _BEORN_FLAGS

#define BEORN_VM_VERSION  "v0.0.1-dev"
#define BEORN_LIB_VERSION "LC-0.0.0-dev"

void initial_info() {
  printf("Beorn (%s) (%s)\n", BEORN_VM_VERSION, BEORN_LIB_VERSION);
}

void version_app() {
  printf("Beorn Land (%s) (%s)\n", BEORN_VM_VERSION, BEORN_LIB_VERSION);
}

#endif
