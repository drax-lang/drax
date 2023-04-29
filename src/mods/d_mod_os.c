#include "d_mod_os.h"

#define BUF_SIZE 1024

int d_popen(const char *command, char *output, int output_size) {
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
        execl("/bin/sh", "sh", "-c", command, NULL);
        _exit(127);
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

        int status;
        waitpid(pid, &status, 0);

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return -1;
        }

        return total_bytes_read;
    }
}
