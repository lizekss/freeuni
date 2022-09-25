#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(2, "wrong number of arguments for sleep\n");
        exit(1);
    }
    int num = atoi(argv[1]);
    sleep(num);
    exit(0);
}