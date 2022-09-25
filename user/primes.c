#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

void sieve(int left[]) {
    close(left[1]);
    int n, m, right[2];
    if (read(left[0], (char**)&n, sizeof(int)) > 0) {
        pipe(right);
        printf("prime %d\n", n);
        int pid = fork();
        if (pid == 0) {
            sieve(right);
        } else if (pid > 0) {
            while (read(left[0], (char**)&m, sizeof(int)) > 0) {
                if (m % n != 0) {
                    write(right[1], (char**)&m, sizeof(int));
                }
            }
            close(left[0]);
            close(right[1]);
            wait((int*)0);
        } else {
            fprintf(2, "fork failed\n");
            exit(1);
        }
    } else close(left[0]);

    //exit(0);
}


int main(int argc, char *argv[]) {
    int p[2];
    pipe(p);
    int pid = fork();
    if (pid > 0) {
        for (int i = 2; i <= 35; i++)
            write(p[1], (char**)&i, sizeof(int));
        close(p[1]);
        wait((int*)0);
        exit(0);
    } else if (pid == 0) {
        sieve(p);
        exit(0);
    } else {
        fprintf(2, "fork failed\n");
        exit(1);
    }
    printf("done");
    exit(0);
}