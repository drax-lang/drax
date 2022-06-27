#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bshell.h"

#define BUFFER_SIZE 2048
#define BSLL_CHAR "> "

static char buffer[BUFFER_SIZE];

char* b_read_content() {
  fputs(BSLL_CHAR, stdout);
  fgets(buffer, BUFFER_SIZE, stdin);

  size_t sz = strlen(buffer + 1);
  char* tmp = (char*) malloc(sz * sizeof(char));

  for (size_t i = 0; i < sz; i++) {
      tmp[i] = buffer[i];
  }
  
  tmp[sz] = '\0';

  return tmp;
}
