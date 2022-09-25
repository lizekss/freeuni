#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[]) {
    char buff[1];
    int p1[2];
    int p2[2];

    if (pipe(p1) < 0 || pipe(p2) < 0) {
        fprintf(2, "pipe failed\n");
        exit(1);
    }
    
    int p = fork();
    if (p == 0) {
        read(p1[0], buff, 1);
        close(p1[0]);
        printf("%d: received ping\n", getpid());
        write(p2[1], buff, 1);
        close(p2[1]);
        exit(0);
    } else if (p > 0) {
        write(p1[1], buff, 1);
        close(p1[1]);
        read(p2[0], buff, 1);
        close(p2[0]);
        printf("%d: received pong\n", getpid());
        exit(0);
    } else {
        fprintf(2, "fork failed\n");
        exit(1);
    }
}