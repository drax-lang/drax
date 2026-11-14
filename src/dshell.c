#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dshell.h"

#define BUFFER_SIZE 2048
#define BSLL_CHAR "> "

static char buffer[BUFFER_SIZE];

char* b_read_content() {
  fputs(BSLL_CHAR, stdout);
  if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) return NULL;

  size_t sz = strlen(buffer);
  char* tmp = (char*) malloc((sz + 1) * sizeof(char));
  memcpy(tmp, buffer, sz + 1);

  return tmp;
}
