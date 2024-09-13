#include "d_mod_os.h"
#include "../dstring.h"

#define BUF_SIZE 4096

static char get_eq_char(char c) {
  switch (c) {
    case '\n':
      return 'n';

    case '\r':
      return 'r';

    case '\\':
      return '\\';

    case '\0':
      return '0';

    case '\t':
      return 't';

  default:
    return 0;
  }

  return 0;
}

char* replace_special_char(char* str) {
  int i, j;
  int len = strlen(str);
  int count = 0;
  
  for (i = 0; i < len; i++) {
    if (str[i] == '\n') count++;
    if (str[i] == '\r') count++;
    if (str[i] == '\\') count++;
    if (str[i] == '\0') count++;
    if (str[i] == '\t') count++;
  }
  
  char* n = (char*) malloc(len + count + 1);
  
  for (i = 0, j = 0; i < len; i++, j++) {
    char r = get_eq_char(str[i]);
    if (r != 0) {
      n[j++] = '\\';
      n[j] = r;
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
  #ifndef _WIN32
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
  #else
    HANDLE hPipeRead, hPipeWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0)) {
      return -1;
    }

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hPipeWrite;
    si.hStdError = hPipeWrite;
    si.hStdInput = NULL;

    char cmdLine[BUF_SIZE];
    snprintf(cmdLine, BUF_SIZE, "cmd.exe /c %s", command);

    PROCESS_INFORMATION pi;
    if (!CreateProcess(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
      CloseHandle(hPipeWrite);
      CloseHandle(hPipeRead);
      return -1;
    }

    CloseHandle(hPipeWrite);

    DWORD total_bytes_read = 0;
    DWORD bytes_read;
    char buf[BUF_SIZE];

    while (ReadFile(hPipeRead, buf, BUF_SIZE, &bytes_read, NULL) && bytes_read > 0) {
      if (total_bytes_read + bytes_read > (DWORD)output_size) {
        CloseHandle(hPipeRead);
        return -1;
      }
      memcpy(output + total_bytes_read, buf, bytes_read);
      total_bytes_read += bytes_read;
    }
    output[total_bytes_read] = '\0';

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hPipeRead);

    return exit_code;
  #endif
}
#endif
