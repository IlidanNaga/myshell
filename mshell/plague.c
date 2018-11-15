#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/wait.h>

int main (int argc, char **argv) {

    int pid, i = 0, status;
    int fd[2];

    if (argc < 3)
        exit(1);

    while (++i < argc) {

        pipe(fd);
        pid = fork();

        if (pid == 0) {

            if (i + 1 != argc) {

                dup2(fd[1], 1);
            }
            close(fd[1]);
            close(fd[0]);
            execlp(argv[i], argv[i], NULL);
            exit(1);
        }
        dup2(fd[0],0);
        close(fd[1]);
        close(fd[0]);
    }

    while (wait(NULL) != -1);

    return 0;

}