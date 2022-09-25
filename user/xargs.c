#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(2, "wrong number of arguments for xargs\n");
    }

    char buf[1024];
    int len = 0;
    char *new_argv[MAXARG];
    for (int i = 1; i < argc; i++) {
        new_argv[i - 1] = argv[i];
    }
    int r;
    for (;;) {
        // read line into buffer
        while (len <= 1024 && (r = read(0, &buf[len++], 1)) == 1) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
                new_argv[argc - 1] = buf; 
                new_argv[argc] = 0;
                len = 0;
                break;
            }   
        }
        if (r <= 0)
            break;
        // fork-exec
        int p = fork();
        if (p == 0) {
            exec(new_argv[0], new_argv);
            fprintf(2, "exec failed\n");
            exit(1);
        } else if (p > 0) {
            wait((int*) 0);
        } else {
            fprintf(2, "fork failed\n");
            exit(1);
        }
    }
    exit(0);
    
}