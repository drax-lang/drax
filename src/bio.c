#include <stdio.h>
#include <stdlib.h>
#include "bio.h"

int get_file_content(char* name, char** content)
{
  char * buffer = NULL;
  long length;
  FILE * f = fopen (name, "rb");

  if (f)
  {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = (char *) calloc (length + 1, sizeof(char));
    if (buffer)
    {
      fread (buffer, 1, length, f);
    }
    // fclose (f);
  }

  if (buffer)
  {
    buffer[length + 1] = 0;
    *content = buffer;
    return 0;
  }

  return 1;
}
