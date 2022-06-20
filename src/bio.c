#include <stdio.h>

int get_file_content(char* name, char** content)
{
  char * buffer = 0;
  long length;
  FILE * f = fopen (name, "rb");

  if (f)
  {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = (char *) malloc (length);
    if (buffer)
    {
      fread (buffer, 1, length, f);
    }
    // fclose (f);
  }

  if (buffer)
  {
    *content = buffer;
    return 0;
  }

  return 1;
}
