#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>

#include "dio.h"
#include "dstring.h"

char* b_pwd() {
  #ifdef _WIN32

  #include <direct.h>
  #define MAX_PATH_SIZE 255

  char* relative_path = (char*) malloc(sizeof(char) * MAX_PATH_SIZE);
  _getcwd(relative_path, MAX_PATH_SIZE);
  return relative_path;
  #else
  long size;
  char *relative_path;

  size = pathconf("./", _PC_PATH_MAX);
  if ((relative_path = (char *) malloc((size_t) size)) != NULL)
      getcwd(relative_path, (size_t) size);

  return relative_path;
  #endif
}

char* normalize_path(char* bp, char* path) {
  char* nbp = NULL;

  if (NULL != bp) {
    nbp = strndup(bp, strlen(bp) + 1);
    nbp = dirname(nbp);
  }

  char* full_path;
  if (path[0] == '/') {
    full_path = malloc((strlen(path) * sizeof(char)) + 1);
    strcpy(full_path, path);
    return full_path;
  }

  int unp = NULL == nbp;
  char* r_path = unp ? b_pwd() : nbp;
  size_t rs = strlen(r_path);
  size_t cs = strlen(path);
  full_path = (char*) malloc(sizeof(char) * (rs + cs + 2));
 
  size_t i = 0;
  for (i = 0; i < rs; i++) { full_path[i] = r_path[i]; }
  full_path[rs] = '/';
  for (i = 0; i < cs; i++) { full_path[i + rs + 1] = path[i]; }
  full_path[rs + cs + 1] = '\0';
  if (unp) free(r_path);
  return full_path;
}

int get_file_content(char* bp, char* name, char** content) {
  char* filename = normalize_path(bp, name);
  
  char* buffer = NULL;
  long length;
  FILE* f = fopen(filename, "rb");

  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = (char*) calloc(length + 1, sizeof(char));
    if (buffer) {
      fread(buffer, 1, length, f);
    }
    fclose (f);
  }

  free(filename);
  if (buffer) {
    buffer[length] = 0;
    *content = buffer;
    return 0;
  }

  return 1;
}
