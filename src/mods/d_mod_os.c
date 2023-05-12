#include "d_mod_os.h"
#include "../dstring.h"

#define BUF_SIZE 4096

char* replace_special_char(char special, char new, char* str) {
  int i, j;
  int len = strlen(str);
  int count = 0;
  
  for (i = 0; i < len; i++) {
    if (str[i] == special) count++;
  }
  
  char* n = (char*) malloc(len + count + 1);
  
  for (i = 0, j = 0; i < len; i++, j++) {
    if (str[i] == special) {
      n[j++] = '\\';
      n[j] = new;
    }
    else {
      n[j] = str[i];
    }
  }
  n[j] = '\0';
  
  return n;
}

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 199309L
int d_command(const char* command, char* output, int output_size) {
  char* tmpc = str_format_output(command);
  FILE *pipe = popen(tmpc, "r");
  free(tmpc);
  int status;

  if (pipe == NULL) {
    printf("Fail to create pipe\n");
    return -1;
  }

  int total_bytes_read = 0;
  char buf[BUF_SIZE];

  while (fgets(buf, sizeof(buf), pipe) != NULL) {
    int bytes_to_copy = strlen(buf);
    if (total_bytes_read + bytes_to_copy >= output_size) {
        printf("Output buffer too small\n");
        pclose(pipe);
        return -1;
    }

    memcpy(output + total_bytes_read, buf, bytes_to_copy);
    total_bytes_read += bytes_to_copy;
  }
  output[total_bytes_read] = '\0';
  status = pclose(pipe);

  return WEXITSTATUS(status);
}
#else
int d_command(const char* command, char* output, int output_size) {
  int pipe_fd[2];
  pid_t pid;

  if (pipe(pipe_fd) == -1) {
    return -1;
  }

  pid = fork();
  if (pid == -1) {
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    return -1;
  }

  if (pid == 0) {
    close(pipe_fd[0]);
    dup2(pipe_fd[1], STDOUT_FILENO);
    close(pipe_fd[1]);
    char* tmpc = str_format_output(command);
    execl("/bin/sh", "sh", "-c", tmpc, NULL);
    free(tmpc);
    return -1;
  } else {
    close(pipe_fd[1]);
    int total_bytes_read = 0;
    int bytes_read;
    char buf[BUF_SIZE];

    while ((bytes_read = read(pipe_fd[0], buf, BUF_SIZE)) > 0) {
      if (total_bytes_read + bytes_read > output_size) {
        errno = EOVERFLOW;
        return -1;
      }
      memcpy(output + total_bytes_read, buf, bytes_read);
      total_bytes_read += bytes_read;
    }
    output[total_bytes_read] = '\0';

    int status;
    waitpid(pid, &status, 0);

    return WEXITSTATUS(status);
  }
}
#endif
