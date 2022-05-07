#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 2048
#define BSLL_CHAR "> "

static char buffer[BUFFER_SIZE];

void breg_history(char* content) {
    // fputs(content, stdout);
}

char* b_read_content() {
  fputs(BSLL_CHAR, stdout);
  fgets(buffer, BUFFER_SIZE, stdin);

  size_t sz = strlen(buffer);
  char* tmp = malloc(sz + 1);

  for (size_t i = 0; i < sz; i++) {
      tmp[i] = buffer[i];
  }
  
  tmp[sz  + 1] = '\0';

//   breg_history(tmp);
  return tmp;
}
