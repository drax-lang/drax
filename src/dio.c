#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "dio.h"

char* b_pwd() {
  #ifdef _WIN32

  #include <direct.h>
  #define MAX_PATH_SIZE 255

  char* relative_path = (char*) malloc(sizeof(char) * MAX_FATH_SIZE);
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

char* normalize_path(char* path) {
  if (path[0] == '/') return path;

  char* r_path = b_pwd();
  size_t rs = strlen(r_path);
  size_t cs = strlen(path);
  char* full_path = (char*) malloc(sizeof(char) * (rs + cs + 2));
 
  for (size_t i = 0; i < rs; i++) { full_path[i] = r_path[i]; }
  full_path[rs] = '/';
  for (size_t i = 0; i < cs; i++) { full_path[i + rs + 1] = path[i]; }
  full_path[rs + cs + 1] = '\0';
 
  return full_path;
}

int get_file_content(char* name, char** content) {
  char* filename = normalize_path(name);

  char * buffer = NULL;
  long length;
  FILE * f = fopen (filename, "rb");

  if (f) {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = (char *) calloc(length + 1, sizeof(char));
    if (buffer) {
      fread (buffer, 1, length, f);
    }
    fclose (f);
  }

  if (buffer) {
    buffer[length] = 0;
    *content = buffer;
    return 0;
  }

  return 1;
}